#!/usr/bin/env python3
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
ROOT=Path(__file__).resolve().parent
FS=ROOT.parent.parent/'fs_src'
class Handler(SimpleHTTPRequestHandler):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,directory=str(FS),**kwargs)
    def log_message(self,*args): pass
    def do_GET(self):
        if self.path=='/':
            content=(FS/'index.html').read_text().replace('</body>', '<script src="/panel-preview.js"></script></body>').encode()
        elif self.path=='/garage.html':
            content=(FS/'garage.html').read_bytes()
            self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8')
            self.send_header('Content-Length',str(len(content)));self.send_header('Cache-Control','no-store')
            self.end_headers();self.wfile.write(content);return
        elif self.path.split('?')[0].lstrip('/') in {
            'garage-only-preview.webp','garage-tesla-s-day.webp','garage-tesla-s-night.webp','garage-v16-day.webp','garage-v16-night.webp','garage-v16-door-day.webp','garage-v16-door-night.webp','garage-v16-interior-day.webp','garage-v16-interior-night.webp','garage-house-day.webp','garage-house-night.webp','garage-interior-car.webp',
            'garage-interior-empty.webp','garage-interior-car-night.webp','garage-interior-empty-night.webp','garage-car.webp','garage-car-night.webp','garage-door-night.webp','garage-door.webp','garage-icon-open.svg','garage-icon-closed.svg'}:
            filename=self.path.split('?')[0].lstrip('/')
            content=(FS.parent/'fs'/filename).read_bytes()
            self.send_response(200);self.send_header('Content-Type','image/webp' if filename.endswith('.webp') else 'image/svg+xml')
            self.send_header('Content-Length',str(len(content)));self.end_headers();self.wfile.write(content);return
        elif self.path=='/panel-preview.js':
            content=(ROOT/'panel-preview.js').read_bytes()
        else:
            return super().do_GET()
        self.send_response(200)
        self.send_header('Content-Type','text/html; charset=utf-8' if self.path=='/' else 'application/javascript; charset=utf-8')
        self.send_header('Content-Length',str(len(content)))
        self.send_header('Cache-Control','no-store')
        self.end_headers();self.wfile.write(content)
print('Panel firmware: http://127.0.0.1:18766',flush=True)
ThreadingHTTPServer(('127.0.0.1',18766),Handler).serve_forever()
