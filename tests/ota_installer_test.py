#!/usr/bin/env python3
"""Exercise the real PHP installer over HTTP without contacting a device."""
import json
from pathlib import Path
import shutil
import socket
import subprocess
import tempfile
import time
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
BASE = 'https://hejsiri.pl/firmware/garage-blu/'


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args):
        return None


with tempfile.TemporaryDirectory(prefix='garage-installer-') as folder:
    work = Path(folder)
    (work / 'update').mkdir()
    for resource in ('index.php', 'installer.js', 'compatibility.json'):
        shutil.copy2(ROOT / 'tools/ota/update' / resource, work / 'update' / resource)
    releases = {}
    for model in ('ShellyPlus1', 'Shelly1Gen3'):
        name = f'shelly-homekit-garage-blu-{model}-2.16.1_blu.79.zip'
        (work / name).write_bytes(b'test firmware')
        releases[model] = {'url': BASE + name, 'size': len(b'test firmware')}
    metadata = work / 'install.json'
    metadata.write_text(json.dumps(releases))
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        port = sock.getsockname()[1]
    server = subprocess.Popen(['php', '-S', f'127.0.0.1:{port}', '-t', str(work)],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    opener = urllib.request.build_opener(NoRedirect)

    def request(headers=None, method='GET'):
        req = urllib.request.Request(f'http://127.0.0.1:{port}/update/', headers=headers or {}, method=method)
        try:
            response = opener.open(req, timeout=3)
        except urllib.error.HTTPError as error:
            response = error
        with response:
            return response.status, response.headers, response.read().decode()

    try:
        for attempt in range(50):
            try:
                status, headers, body = request()
                break
            except urllib.error.URLError:
                time.sleep(0.1)
        else:
            raise AssertionError('PHP server did not start')
        assert status == 200 and '1.7.6' in body and '2.2.0' in body
        assert 'pt check failed' in body and 'PT update' in body
        assert 'installer.js?v=2' in body and 'Sprawdź zgodność' in body
        assert 'Rozpocznij instalację' not in body
        for prefix, model in [('shellyplus1', 'ShellyPlus1'), ('ShellyPlus1', 'ShellyPlus1'),
                              ('shelly1g3', 'Shelly1Gen3'), ('Shelly1G3', 'Shelly1Gen3')]:
            required = '1.7.6' if model == 'ShellyPlus1' else '2.2.0'
            arch = 'esp32' if model == 'ShellyPlus1' else 'esp32c3'
            for version in (required, f'{arch} {required} 20261111-123456',
                            f'20261111-123456/{required}-g123abc'):
                status, headers, body = request({'X-MGOS-Device-ID': prefix + '-001122aabbcc 001122AABBCC',
                                                  'X-MGOS-FW-Version': version})
                assert status == 302 and headers['Location'] == releases[model]['url']
                assert headers['Cache-Control'] == 'no-store'
            for version in ('1.0.0', '2.3.0', 'future', '', required + '-beta1'):
                status, headers, body = request({'X-MGOS-Device-ID': prefix + '-001122aabbcc',
                                                  'X-MGOS-FW-Version': version})
                assert status == 412 and 'Location' not in headers and required in body
        status, headers, body = request({'X-MGOS-Device-ID': 'shelly1g3-001122aabbcc', 'X-MGOS-FW-Version': '2.2.0'}, 'HEAD')
        assert status == 302 and not body
        for bad in ('shelly1pmg3-001122aabbcc', 'shelly1minig3-001122aabbcc',
                    'shelly1g4-001122aabbcc', 'shellyplus1pm-001122aabbcc',
                    'shelly1g3-../../file', 'shelly1g3-001122aabbcc.evil'):
            status, _, body = request({'X-MGOS-Device-ID': bad})
            assert status == 404 and 'Unsupported Shelly model' in body
        assert request({'X-MGOS-FW-Version': '2.2.0'})[0] == 400
        releases['Shelly1Gen3']['url'] = 'https://example.com/firmware.zip'
        metadata.write_text(json.dumps(releases))
        valid = {'X-MGOS-Device-ID': 'shelly1g3-001122aabbcc', 'X-MGOS-FW-Version': '2.2.0'}
        assert request(valid)[0] == 503
        metadata.write_text('not json')
        assert request(valid)[0] == 503
        metadata.unlink()
        assert request(valid)[0] == 503
        (work / 'update/compatibility.json').unlink()
        assert request()[0] == 503
    finally:
        server.terminate()
        server.wait(timeout=5)

print('PASS: compatible redirects, older/newer/beta firmware blocked, HEAD, unsupported models, missing headers and invalid metadata')
