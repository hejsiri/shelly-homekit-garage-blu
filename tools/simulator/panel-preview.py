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
