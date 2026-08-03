#ifndef _FS_PAGE_CACHE_H
#define _FS_PAGE_CACHE_H

#include <fs/fs_types.h>
#include <stdint.h>

struct inode;

struct page_cache_entry
{
        uint32_t pgoff;
        uint32_t phys;
        uint32_t valid;
        tier_t tier;
        uint32_t last_access;
};

int readpage(struct inode *inode, uint32_t pgoff, void *buf, tier_t tier);

#endif
