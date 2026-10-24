#!/usr/bin/env python3
"""Compile and exercise the production HTTP asset allowlist."""
from pathlib import Path
import subprocess,tempfile
s=Path('src/shelly_main.cpp').read_text();a=s.index('static void HTTPHandler(');b=s.index('\nmgos::hap::Accessory::IdentifyCB',a)
h=r'''
#include <cassert>
#include <cstring>
#include <strings.h>
#include <string>
#define MG_EV_HTTP_REQUEST 1
#define MG_F_SEND_AND_CLOSE 2
#define MG_SOCK_STRINGIFY_IP 0
struct mg_str { const char *p; size_t len; };
mg_str mg_mk_str(const char *s) { return {s,strlen(s)}; }
int mg_vcmp(mg_str *a,const char *b) { return a->len==strlen(b)?strncmp(a->p,b,a->len):1; }
int mg_vcasecmp(mg_str *a,const char *b) { return a->len==strlen(b)?strncasecmp(a->p,b,a->len):1; }
struct http_message { mg_str method,uri; };
struct mg_connection { int flags=0,sa=0; };
std::string file,type,headers;int error=0;
void mg_http_send_error(mg_connection *,int code,void *) { error=code; }
void mg_sock_addr_to_str(int *,char *out,size_t,int) { strcpy(out,"127.0.0.1"); }
void ReportClientRequest(const char *) {}
void mg_http_serve_file(mg_connection *,http_message *,const char *f,mg_str t,mg_str h) {file=f;type.assign(t.p,t.len);headers.assign(h.p,h.len);}
'''+s[a:b]+r'''
void Request(const char *path,const char *method="GET") {file.clear();type.clear();headers.clear();error=0;mg_connection c;http_message hm{mg_mk_str(method),mg_mk_str(path)};HTTPHandler(&c,1,&hm,nullptr);}
int main(){
 Request("/garage.html");assert(file=="garage.html.gz"&&type=="text/html"&&headers.find("gzip")!=std::string::npos);
 for(const char *p:{"/garage-house-day.webp","/garage-house-night.webp","/garage-interior-car.webp","/garage-interior-empty.webp","/garage-interior-car-night.webp","/garage-interior-empty-night.webp","/garage-car.webp","/garage-car-night.webp","/garage-door-night.webp","/garage-door.webp"}){Request(p);assert(type=="image/webp"&&headers.find("gzip")==std::string::npos&&error==0);}
 Request("/garage-icon-closed.svg");assert(type=="image/svg+xml");
 Request("/conf9.json");assert(error==404&&file.empty());Request("/../conf9.json");assert(error==404);Request("/garage-house-day.webp","POST");assert(error==404);
 Request("/");assert(file=="index.html.gz"&&headers.find("gzip")!=std::string::npos);
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.cpp').write_text(h);subprocess.run(['clang++','-std=c++14','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS: production HTTP allowlist, gzip HTML, WebP/SVG MIME types, rejected traversal and non-GET')
