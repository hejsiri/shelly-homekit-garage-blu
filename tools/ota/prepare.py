#!/usr/bin/env python3
"""Prepare OTA downloads for Plus 1 and Shelly 1 Gen3 builds."""
import argparse
import hashlib
import html
import json
import re
from pathlib import Path
from string import Template
import zipfile
from urllib.parse import quote
from check_stock_layout import partitions

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
    'Shelly1Gen3': ('S1G3', 'esp32c3', 0x300000, 0xE0000),
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
        table = partitions(archive.read(manifest['parts']['pt']['src']))
        app0, fs0, app1, fs1 = ((0x20000, 0x320000, 0x400000, 0x700000)
                              if model == 'Shelly1Gen3' else
                              (0x10000, 0x1A0000, 0x200000, 0x390000))
        for name, address, size in [('app_0', app0, app_slot), ('app_1', app1, app_slot),
                                    ('fs_0', fs0, fs_size), ('fs_1', fs1, fs_size)]:
            assert table[name][2:4] == (address, size), f'Wrong {name} layout for model'
    version = manifest['version']
    assert re.fullmatch(r'2\.16\.1(?:-blu[0-9]+|_blu\.[0-9]+)', version), 'Unexpected channel version'
    assert all(c.isalnum() or c in '.-_'  for c in version)
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
urls = {model: r['url'] for model, r in releases.items()}
message = (f'Ten Gen3 ma starszy układ partycji. Bezpośrednia aktualizacja do {version} '
           'jest zablokowana. Procedura przejścia: ' + base + 'update/')
fallback = {'version': version, 'rel_notes': base,
            'urls': {model: url for model, url in urls.items() if model != 'Shelly1Gen3'},
            'incompatible_models': {'Shelly1Gen3': message}}
# Only the new Gen3 layout may receive the current Gen3 ZIP through HomeKit OTA.
# The stock installer gates exact compatible stock versions, with one ZIP per model.
channel = [[r'^2\.16\.1_blu\.(?:79|[89][0-9]|[1-9][0-9]{2,})$',
            {'version': version, 'rel_notes': base, 'urls': urls}], ['.*', fallback]]
# Old embedded panels cannot parse underscores or a dotted variant sequence.
# Advertise a legacy alias only to their version strings; ZIPs/manifests and
# canonical release metadata always retain the requested version format.
if '_blu.' in version:
    legacy = version.replace('_blu.', '-blu')
    channel.insert(1, [r'^[^_]+$', {**fallback, 'version': legacy}])
(out / 'update.json').write_text(json.dumps(channel, indent=2) + '\n')
(out / 'release.json').write_text(json.dumps(releases, indent=2) + '\n')
(out / 'install.json').write_text(json.dumps(releases, indent=2) + '\n')
(out / 'update').mkdir(exist_ok=True)
for resource in ('index.php', 'installer.js', 'compatibility.json'):
    (out / 'update' / resource).write_bytes((Path(__file__).parent / 'update' / resource).read_bytes())
downloads = ''.join(
    f'<li><strong>{html.escape(model)}</strong>: <a href="{r["url"]}">ZIP {html.escape(r["version"])} ({r["size"]:,} bajtów)</a> · '
    f'<a href="{r["url"]}.sha256">SHA-256</a></li>' for model, r in releases.items())
for guide in ['AKTUALIZACJA.md', 'KONFIGURACJA.md', 'ROZWIAZYWANIE_PROBLEMOW.md', 'HISTORIA_ZMIAN.md', 'SHELLY_1_GEN3.md', 'INSTALACJA_JEDEN_ADRES.md']:
    (out / guide).write_bytes((root / 'docs' / guide).read_bytes())
for guide in ['BINDKEY_BLE.md', 'BLU_DEVICE_PHOTOS.json', 'QINGPING.md', 'XIAOMI_SMOKE.md']:
    if (root / 'docs' / guide).exists():
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
