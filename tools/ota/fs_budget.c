/* Read-only LittleFS2 allocation inspector. Device data is never written. */
#include "lfs.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 4096

struct image {
  const unsigned char *data;
  size_t size;
  unsigned char *used;
  size_t used_blocks;
};

static int read_block(const struct lfs_config *cfg, lfs_block_t block,
                      lfs_off_t off, void *buffer, lfs_size_t size) {
  struct image *img = cfg->context;
  const size_t start = (size_t) block * cfg->block_size + off;
  if (block >= cfg->block_count || off > cfg->block_size ||
      size > cfg->block_size - off || start > img->size ||
      size > img->size - start) return LFS_ERR_IO;
  memcpy(buffer, img->data + start, size);
  return 0;
}

static int reject_program(const struct lfs_config *cfg, lfs_block_t block,
                          lfs_off_t off, const void *data, lfs_size_t size) {
  (void) cfg; (void) block; (void) off; (void) data; (void) size;
  return LFS_ERR_IO;
}

static int reject_erase(const struct lfs_config *cfg, lfs_block_t block) {
  (void) cfg; (void) block;
  return LFS_ERR_IO;
}

static int sync_image(const struct lfs_config *cfg) {
  (void) cfg;
  return 0;
}

static int visit_block(void *context, lfs_block_t block) {
  struct image *img = context;
  if (block >= img->size / BLOCK_SIZE) return LFS_ERR_CORRUPT;
  if (!img->used[block]) {
    img->used[block] = 1;
    img->used_blocks++;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  FILE *file = fopen(argv[1], "rb");
  if (!file) return 2;
  if (fseek(file, 0, SEEK_END) || ftell(file) <= 0) {
    fclose(file); return 2;
  }
  const size_t size = (size_t) ftell(file);
  if (size % BLOCK_SIZE || size < 4 * BLOCK_SIZE || fseek(file, 0, SEEK_SET)) {
    fclose(file); return 2;
  }
  unsigned char *data = malloc(size);
  unsigned char *used = calloc(size / BLOCK_SIZE, 1);
  if (!data || !used || fread(data, 1, size, file) != size) {
    fclose(file); free(data); free(used); return 2;
  }
  fclose(file);
  struct image img = {data, size, used, 0};
  struct lfs_config cfg = {0};
  cfg.context = &img;
  cfg.read = read_block;
  cfg.prog = reject_program;
  cfg.erase = reject_erase;
  cfg.sync = sync_image;
  cfg.read_size = 128;
  cfg.prog_size = 128;
  cfg.block_size = BLOCK_SIZE;
  cfg.block_count = size / BLOCK_SIZE;
  cfg.cache_size = 128;
  cfg.lookahead_size = 32;
  cfg.block_cycles = 512;
  lfs_t lfs = {0};
  int result = lfs_mount(&lfs, &cfg);
  if (result == 0) {
    result = lfs_fs_traverse(&lfs, visit_block, &img);
    const int unmount_result = lfs_unmount(&lfs);
    if (result == 0) result = unmount_result;
  }
  if (result == 0) {
    printf("{\"size\":%zu,\"used\":%zu,\"free\":%zu,\"block_size\":%u}\n",
           size, img.used_blocks * BLOCK_SIZE,
           size - img.used_blocks * BLOCK_SIZE, BLOCK_SIZE);
  } else {
    fprintf(stderr, "LittleFS2 image check failed (%d)\n", result);
  }
  free(data); free(used);
  return result == 0 ? 0 : 1;
}
