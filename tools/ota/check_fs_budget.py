#!/usr/bin/env python3
"""Require free LittleFS blocks for configuration, HomeKit keys and updates."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LFS = Path.home() / '.mos/work/shelly-garage-blu/deps/vfs-fs-lfs/littlefs'


def inspect_image(image: bytes, lfs_source: Path = DEFAULT_LFS) -> dict:
    for name in ('lfs.c', 'lfs_util.c', 'lfs.h'):
        if not (lfs_source / name).is_file():
            raise ValueError(f'Missing LittleFS source: {lfs_source / name}')
    with tempfile.TemporaryDirectory(prefix='garage-fs-budget-') as folder:
        work = Path(folder)
        image_file = work / 'fs.img'
        image_file.write_bytes(image)
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c99',
                        '-DLFS_NO_DEBUG', '-DLFS_NO_WARN', '-DLFS_NO_ERROR',
                        '-I' + str(lfs_source), str(ROOT / 'tools/ota/fs_budget.c'),
                        str(lfs_source / 'lfs.c'), str(lfs_source / 'lfs_util.c'),
                        '-o', str(work / 'inspect')], check=True)
        result = subprocess.run([str(work / 'inspect'), str(image_file)],
                                check=True, capture_output=True, text=True)
        return json.loads(result.stdout)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('firmware', type=Path, help='Firmware ZIP or LittleFS2 fs.img')
    parser.add_argument('--lfs-source', type=Path,
                        default=Path(os.environ.get('SHELLY_LFS_SOURCE', DEFAULT_LFS)))
    parser.add_argument('--min-free', type=int, default=64 * 1024)
    args = parser.parse_args()
    if args.min_free < 0:
        parser.error('--min-free must be non-negative')
    if args.firmware.suffix == '.zip':
        with zipfile.ZipFile(args.firmware) as archive:
            manifest = json.loads(archive.read('manifest.json'))
            part = manifest['parts']['fs']
            image = archive.read(part['src'])
            if len(image) != part['size'] or hashlib.sha256(image).hexdigest() != part['cs_sha256']:
                raise ValueError('Filesystem image does not match manifest')
    else:
        image = args.firmware.read_bytes()
    info = inspect_image(image, args.lfs_source)
    print(f'LittleFS: {info["used"]} used, {info["free"]} free of {info["size"]} bytes; '
          f'required reserve {args.min_free} bytes')
    if info['free'] < args.min_free:
        raise SystemExit('Firmware filesystem leaves too little space for persistent configuration')


if __name__ == '__main__':
    main()
