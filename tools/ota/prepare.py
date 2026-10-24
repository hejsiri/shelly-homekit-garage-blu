#!/usr/bin/env python3
"""Prepare OTA downloads for selected builds (Plus 1 by default)."""
import argparse
import hashlib
import html
import json
from pathlib import Path
from string import Template
import zipfile
from urllib.parse import quote

root = Path(__file__).resolve().parents[2]
base = 'https://hejsiri.pl/firmware/garage-blu/'
out = root / 'releases/hosting/garage-blu'
out.mkdir(parents=True, exist_ok=True)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--model', action='append', choices=['ShellyPlus1', 'Shelly1Gen3'],
                    help='Model to publish; repeat for both. Default: ShellyPlus1.')
models = list(dict.fromkeys(parser.parse_args().model or ['ShellyPlus1']))
apps = {'ShellyPlus1': 'Plus1', 'Shelly1Gen3': 'S1G3'}
releases = {}
for model in models:
    app = apps[model]
    source = root / f'build_{model}/fw.zip'
    with zipfile.ZipFile(source) as archive:
        assert archive.testzip() is None, 'ZIP integrity failure'
        assert archive.namelist()[0] == 'manifest.json', 'Manifest must be first'
        assert archive.getinfo('manifest.json').compress_type == zipfile.ZIP_STORED
        manifest = json.loads(archive.read('manifest.json'))
    assert manifest['name'] == app, 'Wrong device image'
    version = manifest['version']
    assert version.startswith('2.16.1-blu'), 'Unexpected channel version'
    assert all(c.isalnum() or c in '.-' for c in version)
    name = f'shelly-homekit-garage-blu-{model}-{version}.zip'
    payload = source.read_bytes()
    (out / name).write_bytes(payload)
    checksum = hashlib.sha256(payload).hexdigest()
    (out / (name + '.sha256')).write_text(f'{checksum}  {name}\n')
    releases[model] = {'model': model, 'version': version,
        'build_id': manifest.get('build_id'), 'url': base + name,
        'size': len(payload), 'sha256': checksum, 'hardware_tested': False}
    (out / f'release-{model}.json').write_text(json.dumps(releases[model], indent=2) + '\n')
assert len({r['version'] for r in releases.values()}) == 1, 'Selected builds must have the same version'
channel = [['.*', {'version': version, 'rel_notes': base,
    'urls': {model: r['url'] for model, r in releases.items()}}]]
(out / 'update.json').write_text(json.dumps(channel, indent=2) + '\n')
(out / 'release.json').write_text(json.dumps(releases, indent=2) + '\n')
# The first-install example must refer to an image published in this release.
example_model = models[0]
name = releases[example_model]['url'][len(base):]
downloads = ''.join(
    f'<li><strong>{html.escape(model)}</strong>: <a href="{r["url"]}">ZIP {html.escape(r["version"])} ({r["size"]:,} bajtów)</a> · '
    f'<a href="{r["url"]}.sha256">SHA-256</a></li>' for model, r in releases.items())
for guide in ['AKTUALIZACJA.md', 'KONFIGURACJA.md', 'ROZWIAZYWANIE_PROBLEMOW.md', 'HISTORIA_ZMIAN.md']:
    (out / guide).write_bytes((root / 'docs' / guide).read_bytes())
(out / '.htaccess').write_text('''Options -Indexes
AddType application/zip .zip
AddType application/json .json
<IfModule mod_headers.c>
  Header always set Access-Control-Allow-Origin "*"
  Header always set Access-Control-Allow-Methods "GET, HEAD, OPTIONS"
  Header always set Access-Control-Allow-Headers "X-Model, X-Current-Version, X-Current-Build, X-Device-ID"
  Header always set Cache-Control "no-cache"
  Header always set X-Content-Type-Options "nosniff"
</IfModule>
<IfModule mod_rewrite.c>
  RewriteEngine On
  RewriteCond %{REQUEST_METHOD} =OPTIONS
  RewriteRule ^ - [R=204,L]
</IfModule>
''')
repo = 'https://github.com/hejsiri/shelly-homekit-garage-blu'
model_names = {'ShellyPlus1': 'Shelly Plus 1', 'Shelly1Gen3': 'Shelly 1 Gen3'}
metadata_rows = ''.join(
    f'<tr><th scope="row">{html.escape(model_names[model])}</th>'
    f'<td>{html.escape(r["build_id"] or "")}</td><td>{r["size"]:,}</td>'
    f'<td><code>{html.escape(r["sha256"])}</code></td></tr>'
    for model, r in releases.items())
target_notice = ('Bieżąca paczka dotyczy wyłącznie Shelly Plus 1. Kod dla Shelly 1 Gen3 jest zachowany, '
                 'ale jego nowe paczki są wstrzymane.' if models == ['ShellyPlus1'] else
                 'Publikowane modele: ' + ', '.join(model_names[m] for m in models) + '. Paczki modeli nie są zamienne.')
landing = Template((Path(__file__).parent / 'landing.html').read_text())
(out / 'index.html').write_text(landing.substitute(
    version=html.escape(version),
    repo=repo,
    published_models=html.escape(', '.join(model_names[m] for m in models)),
    target_notice=html.escape(target_notice),
    downloads=downloads,
    metadata_rows=metadata_rows,
    example_model=html.escape(model_names[example_model]),
    update_url=html.escape(quote(base + name, safe='')),
    plus1_status='Bieżąca paczka.' if 'ShellyPlus1' in releases else 'Brak paczki w tym zestawie.',
    gen3_status='Wybrany eksperymentalny target; sprawdź metadane i testy sprzętowe.' if 'Shelly1Gen3' in releases else
                'Kod zachowany. Nowe kompilacje i testy sprzętowe wstrzymane; brak bieżącej paczki.',
))
print(json.dumps({'directory': str(out), 'releases': releases}, indent=2))
