#ifndef _KERNEL_INITRD_H
#define _KERNEL_INITRD_H

#include <stdint.h>
#include <kernel/vfs.h>

typedef struct {
        uint8_t magic;
        char name[64];
        uint32_t offset;
        uint32_t length;
} __attribute__((packed)) initrd_file_header_t;

typedef struct {
        uint32_t nfiles;
} __attribute__((packed)) initrd_header_t;

struct dentry *initialise_initrd(uint32_t location);

#endif
