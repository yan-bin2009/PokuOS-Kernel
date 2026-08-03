#include <fs/tar.h>
#include <kernel/kstring.h>

uint32_t tar_octal(const char *s, size_t len)
{
        uint32_t val = 0;
        size_t i;

        for (i = 0; i < len && s[i]; i++)
        {
                if (s[i] >= '0' && s[i] <= '7')
                        val = (val << 3) | (s[i] - '0');
                else
                        break;
        }
        return val;
}

static void tar_skip_name(char *name)
{
        while (name[0] == '.' && name[1] == '/')
                name += 2;
}

int tar_next_entry(tar_block_read_t read_fn, void *ctx, uint32_t *blk,
                   struct tar_entry *entry)
{
        uint8_t hdr[TAR_BLOCK_SIZE];
        uint32_t size;
        uint32_t data_off;
        uint32_t padding;
        size_t i;

        if (!read_fn || !blk || !entry)
                return -1;

        if (read_fn(ctx, *blk, 1, hdr) != 0)
                return -1;

        if (memcmp(hdr + 257, TAR_MAGIC, 5) != 0)
                return -1;
        if (hdr[0] == '\0')
                return -1;

        for (i = 0; i < 99; i++)
        {
                if (hdr[i] == '\0')
                        break;
                entry->name[i] = hdr[i];
        }
        entry->name[i] = '\0';

        size = tar_octal((const char *)hdr + 124, 12);
        entry->type = hdr[156];
        entry->size = size;

        data_off = (*blk + 1) * TAR_BLOCK_SIZE;
        entry->offset = data_off;

        padding = (TAR_BLOCK_SIZE - (size % TAR_BLOCK_SIZE)) % TAR_BLOCK_SIZE;
        *blk = data_off / TAR_BLOCK_SIZE + (size + padding) / TAR_BLOCK_SIZE;

        return 0;
}

int tar_find_entry(tar_block_read_t read_fn, void *ctx, const char *path,
                   struct tar_entry *entry)
{
        struct tar_entry e;
        uint32_t blk = 0;

        while (tar_next_entry(read_fn, ctx, &blk, &e) == 0)
        {
                tar_skip_name(e.name);
                if (strcmp(e.name, path) == 0)
                {
                        *entry = e;
                        return 0;
                }
        }
        return -1;
}
