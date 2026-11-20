#!/usr/bin/env python3
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit
ROOT=Path(__file__).resolve().parent
FS=ROOT.parent.parent/'fs_src'
class Handler(SimpleHTTPRequestHandler):
    def __init__(self,*args,**kwargs):
        super().__init__(*args,directory=str(FS),**kwargs)
    def log_message(self,*args): pass
    def do_GET(self):
        request_path=urlsplit(self.path).path
        language_script='i18n.js?v='+str((FS/'i18n.js').stat().st_mtime_ns)
        asset_path=self.path.split('?',1)[0].lstrip('/')
        if asset_path in {
            'garage-icon-closed.svg', 'garage-icon-open.svg', 'garage-tesla-s-day.webp',
            'garage-tesla-s-night.webp', 'garage-v16-day.webp', 'garage-v16-door-day.webp',
            'garage-v16-door-night.webp', 'garage-v16-interior-day.webp',
            'garage-v16-interior-night.webp', 'garage-v16-night.webp'}:
            content=(FS.parent/'fs'/asset_path).read_bytes()
            self.send_response(200)
            self.send_header('Content-Type','image/webp' if asset_path.endswith('.webp') else 'image/svg+xml')
            self.send_header('Content-Length',str(len(content)))
            self.send_header('Cache-Control','no-store')
            self.end_headers();self.wfile.write(content);return
        if request_path in {'/', '/dashboard'}:
            page=(FS/'index.html').read_text().replace('src="i18n.js"', 'src="'+language_script+'"').replace('</body>', '<script src="/panel-preview.js"></script></body>')
            if request_path=='/dashboard':
                page=page.replace('</head>', '''<style>
html,body{width:100%;height:100%;margin:0!important;padding:0!important;overflow:hidden}
body>:not(#header_container):not(script){display:none!important}
#header_container{width:100%!important;max-width:none!important;margin:0!important;padding:0!important;border-radius:0!important}
#garage_dashboard{height:100svh!important}
#badges_container{display:none!important}
</style></head>''')
            content=page.encode()
        elif self.path=='/garage.html':
            content=(FS/'garage.html').read_text().replace('src="i18n.js"', 'src="'+language_script+'"').encode()
            self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8')
            self.send_header('Content-Length',str(len(content)));self.send_header('Cache-Control','no-store')
            self.end_headers();self.wfile.write(content);return
        elif self.path.split('?')[0].lstrip('/') in {
            'garage-icon-closed.svg', 'garage-icon-open.svg', 'garage-tesla-s-day.webp', 'garage-tesla-s-night.webp', 'garage-v16-day.webp', 'garage-v16-door-day.webp', 'garage-v16-door-night.webp', 'garage-v16-interior-day.webp', 'garage-v16-interior-night.webp', 'garage-v16-night.webp'}:
            filename=self.path.split('?')[0].lstrip('/')
            content=(FS.parent/'fs'/filename).read_bytes()
            self.send_response(200);self.send_header('Content-Type','image/webp' if filename.endswith('.webp') else 'image/svg+xml')
            self.send_header('Content-Length',str(len(content)));self.end_headers();self.wfile.write(content);return
        elif self.path=='/panel-preview.js':
            content=(ROOT/'panel-preview.js').read_bytes()
        else:
            return super().do_GET()
        self.send_response(200)
        self.send_header('Content-Type','text/html; charset=utf-8' if request_path in {'/', '/dashboard'} else 'application/javascript; charset=utf-8')
        self.send_header('Content-Length',str(len(content)))
        self.send_header('Cache-Control','no-store')
        self.end_headers();self.wfile.write(content)
print('Panel firmware: http://127.0.0.1:18766',flush=True)
ThreadingHTTPServer(('127.0.0.1',18766),Handler).serve_forever()
