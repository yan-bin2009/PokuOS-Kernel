#ifndef _FS_TAR_H
#define _FS_TAR_H

#include <stddef.h>
#include <stdint.h>

#define TAR_BLOCK_SIZE 512
#define TAR_MAGIC "ustar"

struct tar_entry
{
        char name[100];
        uint8_t type;
        uint32_t offset;
        uint32_t size;
};

typedef int (*tar_block_read_t)(void *ctx, uint32_t blk, uint32_t nblk, void *buf);

int tar_find_entry(tar_block_read_t read_fn, void *ctx, const char *path,
                   struct tar_entry *entry);
int tar_next_entry(tar_block_read_t read_fn, void *ctx, uint32_t *blk,
                   struct tar_entry *entry);
uint32_t tar_octal(const char *s, size_t len);

#endif
