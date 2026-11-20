#!/usr/bin/env python3
"""Exercise the fresh-image reserve against actual LittleFS2 allocation."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fs_budget', ROOT / 'tools/ota/check_fs_budget.py')
budget = importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)
lfs = Path(os.environ.get('SHELLY_LFS_SOURCE', budget.DEFAULT_LFS))
fixture = r'''
#define main inspector_entry
#include "INSPECTOR_SOURCE"
#undef main
#include <assert.h>

static int write_block(const struct lfs_config *cfg, lfs_block_t block,
                       lfs_off_t off, const void *data, lfs_size_t size) {
  struct image *img=cfg->context;
  assert(block<cfg->block_count && off+size<=cfg->block_size);
  memcpy((unsigned char*)img->data+block*cfg->block_size+off,data,size);return 0;
}
static int erase_block(const struct lfs_config *cfg,lfs_block_t block) {
  struct image *img=cfg->context;assert(block<cfg->block_count);
  memset((unsigned char*)img->data+block*cfg->block_size,255,cfg->block_size);return 0;
}
int main(int argc,char **argv){
  assert(argc==3);
  unsigned char data[393216];memset(data,255,sizeof(data));
  struct image image={data,sizeof(data),NULL,0};
  struct lfs_config cfg={0};cfg.context=&image;cfg.read=read_block;
  cfg.prog=write_block;cfg.erase=erase_block;cfg.sync=sync_image;
  cfg.read_size=128;cfg.prog_size=128;cfg.block_size=4096;cfg.block_count=96;
  cfg.cache_size=128;cfg.lookahead_size=32;cfg.block_cycles=512;
  lfs_t fs={0};assert(lfs_format(&fs,&cfg)==0);assert(lfs_mount(&fs,&cfg)==0);
  if(strcmp(argv[2],"full")==0){
    lfs_file_t file={0};assert(lfs_file_open(&fs,&file,"payload",LFS_O_WRONLY|LFS_O_CREAT)==0);
    unsigned char chunk[1000];memset(chunk,42,sizeof(chunk));
    for(int i=0;i<350;i++)assert(lfs_file_write(&fs,&file,chunk,sizeof(chunk))==sizeof(chunk));
    assert(lfs_file_close(&fs,&file)==0);
  }
  assert(lfs_unmount(&fs)==0);
  FILE *out=fopen(argv[1],"wb");assert(out);assert(fwrite(data,1,sizeof(data),out)==sizeof(data));assert(fclose(out)==0);
}
'''.replace('INSPECTOR_SOURCE', str(ROOT / 'tools/ota/fs_budget.c'))

with tempfile.TemporaryDirectory(prefix='garage-fs-budget-test-') as folder:
    p = Path(folder)
    (p / 'fixture.c').write_text(fixture)
    subprocess.run(['cc', '-std=c99', '-DLFS_NO_DEBUG', '-DLFS_NO_WARN', '-DLFS_NO_ERROR',
                    '-I' + str(lfs), str(p / 'fixture.c'), str(lfs / 'lfs.c'),
                    str(lfs / 'lfs_util.c'), '-o', str(p / 'fixture')], check=True)
    for kind in ['empty', 'full']:
        subprocess.run([str(p / 'fixture'), str(p / (kind + '.img')), kind], check=True)
    empty = (p / 'empty.img').read_bytes()
    full = (p / 'full.img').read_bytes()
    assert budget.inspect_image(empty, lfs)['free'] >= 65536
    assert 0 < budget.inspect_image(full, lfs)['free'] < 65536

    for name, expected in [('empty.img', 0), ('full.img', 1)]:
        before = hashlib.sha256((p / name).read_bytes()).hexdigest()
        result = subprocess.run(['python3', str(ROOT / 'tools/ota/check_fs_budget.py'), str(p / name)],
                                capture_output=True, text=True)
        assert result.returncode == expected, (name, result.stdout, result.stderr)
        assert hashlib.sha256((p / name).read_bytes()).hexdigest() == before

    for image in [empty[:-1], bytes(len(empty))]:
        try:
            budget.inspect_image(image, lfs)
        except subprocess.CalledProcessError:
            pass
        else:
            raise AssertionError('Corrupt/truncated image accepted')

    # A release ZIP must also match its embedded filesystem checksum.
    for valid in [True, False]:
        archive = p / ('valid.zip' if valid else 'bad-hash.zip')
        manifest = {'parts': {'fs': {'src': 'fs.img', 'size': len(empty),
                                    'cs_sha256': hashlib.sha256(empty).hexdigest() if valid else '0' * 64}}}
        with zipfile.ZipFile(archive, 'w') as z:
            z.writestr('manifest.json', json.dumps(manifest))
            z.writestr('fs.img', empty)
        result = subprocess.run(['python3', str(ROOT / 'tools/ota/check_fs_budget.py'), str(archive)],
                                capture_output=True, text=True)
        assert (result.returncode == 0) == valid, result.stderr

script = (ROOT / 'tools/build-blu.sh').read_text()
assert script.index('make "$model"') < script.index('check_fs_budget.py') < script.index('cp "$build_root/build_$model/fw.zip"')
print('PASS: real LittleFS2 allocation reserve, near-full rejection, read-only images, corrupt/truncated images and ZIP checksum')
