#!/usr/bin/env python3
"""Check publication of real model images and rejection before metadata changes.

Run after building both targets with the same version. All output is confined to
a temporary project; no hosting or live device is contacted.
"""
import io
import json
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

root = Path(__file__).resolve().parents[1]
models = {'ShellyPlus1': ('Plus1', 'esp32'), 'Shelly1Gen3': ('S1G3', 'esp32c3')}


def project(path):
    for name in ['tools/ota/prepare.py', 'tools/ota/landing.html',
                 'tools/ota/check_stock_layout.py', 'tools/ota/update/index.php',
                 'tools/ota/update/installer.js', 'tools/ota/update/compatibility.json']:
        dest = path / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / name, dest)
    (path / 'docs').mkdir()
    for guide in (root / 'docs').glob('*.md'):
        shutil.copy2(guide, path / 'docs' / guide.name)
    for model in models:
        dest = path / f'build_{model}'
        dest.mkdir()
        shutil.copy2(root / f'build_{model}/fw.zip', dest / 'fw.zip')


def mutate_manifest(path, change):
    result = io.BytesIO()
    with zipfile.ZipFile(path) as old, zipfile.ZipFile(result, 'w') as new:
        for info in old.infolist():
            data = old.read(info.filename)
            if info.filename == 'manifest.json':
                manifest = json.loads(data)
                change(manifest)
                data = json.dumps(manifest).encode()
            new.writestr(info, data)
    path.write_bytes(result.getvalue())


def prepare(path, *args):
    return subprocess.run([sys.executable, str(path / 'tools/ota/prepare.py'), *args],
                          capture_output=True, text=True)


with tempfile.TemporaryDirectory(prefix='garage-ota-models-') as temp:
    base = Path(temp)
    valid = base / 'valid'
    project(valid)
    run = prepare(valid)
    assert run.returncode == 0, run.stderr
    out = valid / 'releases/hosting/garage-blu'
    release = json.loads((out / 'release.json').read_text())
    entries = json.loads((out / 'update.json').read_text())
    channel = entries[0][1]
    version = next(iter(release.values()))['version']
    if '_blu.' in version:
        legacy = next(cfg for pattern,cfg in entries if re.search(pattern,'2.16.1-blu56'))
        canonical = next(cfg for pattern,cfg in entries if re.search(pattern,version))
        assert legacy['version'] == version.replace('_blu.','-blu')
        assert canonical['version'] == version
        assert legacy['urls'] == {'ShellyPlus1': canonical['urls']['ShellyPlus1']}
        assert 'Shelly1Gen3' in legacy['incompatible_models']
        old_gen3 = next(cfg for pattern,cfg in entries if re.search(pattern,'2.16.1_blu.78'))
        assert 'Shelly1Gen3' not in old_gen3['urls']
        assert 'Shelly1Gen3' in old_gen3['incompatible_models']
        assert re.fullmatch(r'\d+\.\d+\.\d+-?[a-z]*\d*',legacy['version'])

    assert set(release) == set(models) == set(channel['urls'])
    assert json.loads((out / 'install.json').read_text()) == release
    assert (out / 'update/index.php').is_file()
    assert len({r['version'] for r in release.values()}) == 1
    for model, (app, platform) in models.items():
        name = release[model]['url'].rsplit('/', 1)[1]
        assert (out / name).read_bytes() == (root / f'build_{model}/fw.zip').read_bytes()
        assert channel['urls'][model] == release[model]['url']
        with zipfile.ZipFile(out / name) as z:
            manifest = json.loads(z.read('manifest.json'))
        assert (manifest['name'], manifest['platform']) == (app, platform)
    # Explicit model selection remains available; default publication keeps both.
    assert prepare(valid, '--model', 'ShellyPlus1').returncode == 0
    assert set(json.loads((out / 'update.json').read_text())[0][1]['urls']) == {'ShellyPlus1'}

    for case, change in [
        ('mixed_versions', lambda m: m.update(version=m['version'] + '-different')),
        ('wrong_platform', lambda m: m.update(platform='esp32')),
        ('wrong_ota_model', lambda m: m.update(shelly_hk_model='ShellyPlus1')),
        ('bad_part_hash', lambda m: m['parts']['app'].update(cs_sha256='0' * 64)),
    ]:
        p = base / case
        project(p)
        out = p / 'releases/hosting/garage-blu'
        out.mkdir(parents=True)
        sentinel = {'update.json': b'prior channel', 'release.json': b'prior metadata'}
        for name, data in sentinel.items():
            (out / name).write_bytes(data)
        mutate_manifest(p / 'build_Shelly1Gen3/fw.zip', change)
        assert prepare(p).returncode != 0, case
        assert {f.name: f.read_bytes() for f in out.iterdir()} == sentinel, case

print('PASS: default dual-model OTA mapping, exact model ZIPs, explicit subset and rejection of mixed versions/platform/hash before writes')
