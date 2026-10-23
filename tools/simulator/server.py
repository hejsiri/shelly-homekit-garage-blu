#!/usr/bin/env python3
"""Local behavior model; uses the firmware's real BTHome decoder/freshness rules."""
import ctypes
import json
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent
lib = ctypes.CDLL(str(ROOT / 'blu-sim.dylib'))
lib.blu_receive.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_ulong, ctypes.c_double]
lib.blu_ready.argtypes = [ctypes.c_double, ctypes.c_int]
lib.blu_age.argtypes = [ctypes.c_int, ctypes.c_double]
lib.blu_age.restype = ctypes.c_double
lock = threading.Lock()

class Simulator:
    def reset(self):
        lib.blu_reset()
        self.now = 0
        self.position = 0.0
        self.direction = 0
        self.online = [True, True]
        self.conflict = False
        self.timeout = 30
        self.pulses = 0
        self.logs = []
        self.beacon()
        self.log('Start: brama zamknięta, oba czujniki nadają.')

    def log(self, text):
        self.logs.insert(0, {'time': round(self.now, 1), 'text': text})
        self.logs = self.logs[:30]

    def beacon(self):
        for role in range(2):
            if not self.online[role]:
                continue
            active = self.conflict or (self.position <= 0 if role == 0 else self.position >= 1)
            # Flags + BTHome v2 service, packet id, battery, window.
            packet = bytes([2, 1, 6, 10, 0x16, 0xd2, 0xfc, 0x44,
                            0, int(self.now) % 256, 1, 92 - role * 8, 0x2d, int(not active)])
            buf = ctypes.create_string_buffer(packet)
            if not lib.blu_receive(role, buf, len(packet), self.now):
                raise RuntimeError('Real firmware decoder rejected simulator packet')

    def ready(self):
        return bool(lib.blu_ready(self.now, self.timeout))

    def tick(self, seconds):
        if seconds < 0 or seconds > 180:
            raise ValueError('Nieprawidłowy czas')
        for _ in range(seconds):
            self.now += 1
            if self.direction:
                self.position = max(0.0, min(1.0, self.position + self.direction / 12))
                if self.position < 0.00001 or self.position > 0.99999:
                    self.position = round(self.position)
                    self.direction = 0
                    self.log('Napęd osiągnął pozycję krańcową.')
            self.beacon()

    def command(self, data):
        action = data.get('action')
        if action == 'reset':
            self.reset()
        elif action == 'tick':
            self.tick(int(data.get('seconds', 1)))
        elif action in ('open', 'close'):
            if not self.ready():
                self.log('BLOKADA: brak świeżych danych lub sprzeczne krańcówki. Bez impulsu.')
            elif self.position == (1 if action == 'open' else 0) and not self.direction:
                self.log('Brama jest już w zadanej pozycji. Bez impulsu.')
            else:
                self.pulses += 1
                self.direction = 1 if action == 'open' else -1
                self.log('Impuls przekaźnika 500 ms: ' + ('otwieranie.' if self.direction > 0 else 'zamykanie.'))
        elif action == 'sensor':
            role = int(data['role'])
            if role not in (0, 1):
                raise ValueError('Nieprawidłowy czujnik')
            self.online[role] = bool(data['online'])
            self.log(('Przywrócono' if self.online[role] else 'Wyłączono') + ' sygnał czujnika ' + ('zamknięcia.' if role == 0 else 'otwarcia.'))
            self.beacon()
        elif action == 'conflict':
            self.conflict = bool(data['enabled'])
            self.beacon()
            self.log('Sprzeczne krańcówki: ' + ('włączone.' if self.conflict else 'wyłączone.'))
        elif action == 'restart':
            lib.blu_reset()
            self.log('Restart sterownika: zapomniano odczyty, oczekiwanie na nowe beacony.')
        else:
            raise ValueError('Nieznana akcja')
        return self.status()

    def status(self):
        ready = self.ready()
        if not ready:
            state = 'Błąd / brak potwierdzonej pozycji'
        elif self.direction:
            state = 'Otwieranie' if self.direction > 0 else 'Zamykanie'
        elif lib.blu_active(0):
            state = 'Zamknięta'
        elif lib.blu_active(1):
            state = 'Otwarta'
        else:
            state = 'Zatrzymana między krańcówkami'
        return {'time': self.now, 'position': self.position, 'state': state,
                'ready': ready, 'pulses': self.pulses, 'conflict': self.conflict,
                'sensors': [{'online': self.online[i], 'seen': bool(lib.blu_seen(i)),
                             'active': bool(lib.blu_active(i)), 'age': lib.blu_age(i, self.now),
                             'battery': 92 - i * 8, 'rssi': -58 - i * 7} for i in range(2)],
                'logs': self.logs}

sim = Simulator()
sim.reset()

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def respond(self, status, body, content_type):
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == '/':
            self.respond(200, (ROOT / 'index.html').read_bytes(), 'text/html; charset=utf-8')
        elif self.path == '/api/status':
            with lock:
                body = json.dumps(sim.status()).encode()
            self.respond(200, body, 'application/json')
        else:
            self.respond(404, b'Not found', 'text/plain')

    def do_POST(self):
        if self.path != '/api/action':
            self.respond(404, b'Not found', 'text/plain')
            return
        if self.headers.get('Origin') not in (None, 'http://127.0.0.1:18765', 'http://localhost:18765'):
            self.respond(403, b'Forbidden', 'text/plain')
            return
        try:
            size = int(self.headers.get('Content-Length', 0))
            if size > 4096:
                raise ValueError('Request too large')
            data = json.loads(self.rfile.read(size))
            with lock:
                body = json.dumps(sim.command(data)).encode()
            self.respond(200, body, 'application/json')
        except (ValueError, KeyError, TypeError) as error:
            self.respond(400, json.dumps({'error': str(error)}).encode(), 'application/json')

if __name__ == '__main__':
    print('Symulator: http://127.0.0.1:18765', flush=True)
    ThreadingHTTPServer(('127.0.0.1', 18765), Handler).serve_forever()
