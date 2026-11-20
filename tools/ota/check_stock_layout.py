#!/usr/bin/env python3
"""Compare a candidate OTA layout against an exact stock firmware ZIP.

This is an offline check, not proof that installation or boot will succeed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile


def read_bundle(path):
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP integrity failure')
        manifest_name, = [n for n in archive.namelist() if n.split('/')[-1] == 'manifest.json']
        manifest = json.loads(archive.read(manifest_name))
        prefix = manifest_name[:-len('manifest.json')]
        payloads = {}
        for name, part in manifest['parts'].items():
            if 'src' not in part:
                continue
            data = archive.read(prefix + part['src'])
            if len(data) != part['size']:
                raise ValueError(f'{path}: {name} size mismatch')
            if hashlib.sha256(data).hexdigest() != part['cs_sha256']:
                raise ValueError(f'{path}: {name} SHA-256 mismatch')
            payloads[name] = data
    return manifest, payloads


def partitions(data):
    result = {}
    for offset in range(0, len(data), 32):
        if data[offset:offset + 2] != b'\xaa\x50':
            break
        _, kind, subtype, addr, size, label, flags = struct.unpack('<HBBII16sI', data[offset:offset + 32])
        name = label.rstrip(b'\0').decode('ascii')
        if name in result:
            raise ValueError(f'Duplicate partition: {name}')
        result[name] = (kind, subtype, addr, size, flags)
    if not result:
        raise ValueError('Empty partition table')
    ordered = sorted(result.values(), key=lambda p: p[2])
    for previous, current in zip(ordered, ordered[1:]):
        if previous[2] + previous[3] > current[2]:
            raise ValueError('Overlapping partitions')
    return result


def check(candidate, stock):
    cm, cp = read_bundle(candidate)
    sm, sp = read_bundle(stock)
    for field in ('name', 'platform'):
        if cm[field] != sm[field]:
            raise ValueError(f'Wrong {field}')
    if cm['parts']['pt']['addr'] != sm['parts']['pt']['addr']:
        raise ValueError('Partition table address differs')
    actual, expected = partitions(cp['pt']), partitions(sp['pt'])
    if actual != expected:
        differences = [name for name in actual.keys() | expected.keys()
                       if actual.get(name) != expected.get(name)]
        raise ValueError('Partition layout differs: ' + ', '.join(sorted(differences)))
    if cm['parts']['boot'].get('update') is not False:
        raise ValueError('Candidate must preserve the stock bootloader')
    for name in ('app', 'fs'):
        part = cm['parts'][name]
        if part['ptn'] != sm['parts'][name]['ptn']:
            raise ValueError(f'{name}: wrong target partition')
        slot_size = actual[part['ptn']][3]
        if part['size'] > slot_size:
            raise ValueError(f'{name}: image exceeds partition')
        if name == 'fs' and (part['size'] != slot_size or part['fs_size'] != slot_size):
            raise ValueError('Filesystem size differs from partition')
    print(f'PASS: {cm["name"]} {cm["version"]}: layout matches stock {sm["version"]}; '
          'part sizes/checksums verified, stock bootloader preserved')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('stock', type=Path)
    args = parser.parse_args()
    try:
        check(args.candidate, args.stock)
    except (ValueError, KeyError, zipfile.BadZipFile) as error:
        parser.exit(1, f'FAIL: {error}\n')
