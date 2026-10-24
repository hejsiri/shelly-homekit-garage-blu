#!/usr/bin/env python3
"""Prepare OTA downloads for Plus 1 and Shelly 1 Gen3 builds."""
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
                    help='Model to publish; repeat to select models. Default: both models.')
models = list(dict.fromkeys(parser.parse_args().model or ['ShellyPlus1', 'Shelly1Gen3']))
targets = {
    'ShellyPlus1': ('Plus1', 'esp32', 0x190000, 0x60000),
    'Shelly1Gen3': ('S1G3', 'esp32c3', 0x2B0000, 0xD0000),
}
releases = {}
prepared = []
# Validate every selected package before writing release files or metadata.
for model in models:
    app, platform, app_slot, fs_size = targets[model]
    source = root / f'build_{model}/fw.zip'
    with zipfile.ZipFile(source) as archive:
        assert archive.testzip() is None, 'ZIP integrity failure'
        assert archive.namelist()[0] == 'manifest.json', 'Manifest must be first'
        assert archive.getinfo('manifest.json').compress_type == zipfile.ZIP_STORED
        manifest = json.loads(archive.read('manifest.json'))
        assert manifest['name'] == app, 'Wrong device image'
        assert manifest['platform'] == platform, 'Wrong device platform'
        assert manifest['shelly_hk_model'] == model, 'Wrong OTA model identifier'
        for part in manifest['parts'].values():
            if 'src' not in part:
                continue
            data = archive.read(part['src'])
            assert len(data) == part['size'], 'Part size mismatch'
            assert hashlib.sha256(data).hexdigest() == part['cs_sha256'], 'Part checksum mismatch'
        assert manifest['parts']['app']['size'] <= app_slot, 'Application exceeds model slot'
        assert manifest['parts']['fs']['size'] == fs_size, 'Wrong filesystem size for model'
    version = manifest['version']
    assert version.startswith('2.16.1-blu'), 'Unexpected channel version'
    assert all(c.isalnum() or c in '.-' for c in version)
    prepared.append((model, manifest, source.read_bytes()))
assert len({manifest['version'] for _, manifest, _ in prepared}) == 1, 'Selected builds must have the same version'
for model, manifest, payload in prepared:
    version = manifest['version']
    name = f'shelly-homekit-garage-blu-{model}-{version}.zip'
    (out / name).write_bytes(payload)
    checksum = hashlib.sha256(payload).hexdigest()
    (out / (name + '.sha256')).write_text(f'{checksum}  {name}\n')
    releases[model] = {'model': model, 'version': version,
        'build_id': manifest.get('build_id'), 'url': base + name,
        'size': len(payload), 'sha256': checksum, 'hardware_tested': False}
    (out / f'release-{model}.json').write_text(json.dumps(releases[model], indent=2) + '\n')
channel = [['.*', {'version': version, 'rel_notes': base,
    'urls': {model: r['url'] for model, r in releases.items()}}]]
(out / 'update.json').write_text(json.dumps(channel, indent=2) + '\n')
(out / 'release.json').write_text(json.dumps(releases, indent=2) + '\n')
downloads = ''.join(
    f'<li><strong>{html.escape(model)}</strong>: <a href="{r["url"]}">ZIP {html.escape(r["version"])} ({r["size"]:,} bajtów)</a> · '
    f'<a href="{r["url"]}.sha256">SHA-256</a></li>' for model, r in releases.items())
for guide in ['AKTUALIZACJA.md', 'KONFIGURACJA.md', 'ROZWIAZYWANIE_PROBLEMOW.md', 'HISTORIA_ZMIAN.md', 'SHELLY_1_GEN3.md']:
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
gen3 = releases.get('Shelly1Gen3')
gen3_download = (
    f'<a href="{gen3["url"]}">ZIP {html.escape(gen3["version"])} dla Shelly 1 Gen3</a> · '
    f'<a href="{gen3["url"]}.sha256">SHA-256</a>' if gen3 else
    'Brak paczki Gen3 w tym zestawie.')
stock_update_examples = ''.join(
    f'<h4>{html.escape(model_names[model])}</h4>'
    f'<pre>http://ADRES_IP/rpc/Shelly.Update?url={html.escape(quote(r["url"], safe=""))}</pre>'
    for model, r in releases.items())
metadata_rows = ''.join(
    f'<tr><th scope="row">{html.escape(model_names[model])}</th>'
    f'<td>{html.escape(r["build_id"] or "")}</td><td>{r["size"]:,}</td>'
    f'<td><code>{html.escape(r["sha256"])}</code></td></tr>'
    for model, r in releases.items())
target_notice = ('Publikowane modele: ' + ', '.join(model_names[m] for m in models) +
                 '. Wybierz ZIP dla dokładnego modelu. Paczki Plus1 i S1G3 nie są zamienne.')
landing = Template((Path(__file__).parent / 'landing.html').read_text())
(out / 'index.html').write_text(landing.substitute(
    version=html.escape(version),
    repo=repo,
    published_models=html.escape(', '.join(model_names[m] for m in models)),
    target_notice=html.escape(target_notice),
    downloads=downloads,
    metadata_rows=metadata_rows,
    gen3_download=gen3_download,
    stock_update_examples=stock_update_examples,
    plus1_status='Bieżąca paczka.' if 'ShellyPlus1' in releases else 'Brak paczki w tym zestawie.',
    gen3_status='Bieżąca paczka ESP32-C3. Test na fizycznym Gen3 pozostaje do wykonania.' if 'Shelly1Gen3' in releases else
                'Kod zachowany; brak paczki w tym zestawie.',
))
print(json.dumps({'directory': str(out), 'releases': releases}, indent=2))
