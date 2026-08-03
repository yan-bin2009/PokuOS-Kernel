#include <fs/page_cache.h>
#include <fs/vfs.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/paging.h>
#include <kernel/serial.h>
#include <stddef.h>
#include <vm/vm.h>

#define PAGE_CACHE_SIZE 64

struct page_cache_slot
{
        uint32_t ino;
        uint32_t pgoff;
        uint32_t phys;
        uint32_t valid;
        tier_t tier;
};

static struct page_cache_slot cache[PAGE_CACHE_SIZE];

static uint32_t page_cache_find(uint32_t ino, uint32_t pgoff)
{
        uint32_t i;

        for (i = 0; i < PAGE_CACHE_SIZE; i++)
        {
                if (cache[i].valid && cache[i].ino == ino && cache[i].pgoff == pgoff)
                        return i;
        }
        return PAGE_CACHE_SIZE;
}

static uint32_t page_cache_alloc_slot(void)
{
        uint32_t i;

        for (i = 0; i < PAGE_CACHE_SIZE; i++)
        {
                if (!cache[i].valid)
                        return i;
        }
        return PAGE_CACHE_SIZE;
}

int readpage(struct inode *inode, uint32_t pgoff, void *buf, tier_t tier)
{
        uint32_t slot;
        uint32_t phys;
        void *vaddr;
        int ret;

        if (!inode || !buf)
                return -1;

        slot = page_cache_find(inode->i_ino, pgoff);
        if (slot != PAGE_CACHE_SIZE)
        {
                cache[slot].tier = tier;
                vaddr = (void *)(cache[slot].phys + 0xC0000000);
                memcpy(buf, vaddr, 4096);
                return 4096;
        }

        if (!inode->i_sb || !inode->i_sb->s_op || !inode->i_sb->s_op->readpage)
                return -1;

        slot = page_cache_alloc_slot();
        if (slot == PAGE_CACHE_SIZE)
        {
                serial_write("[page_cache] full\n");
                return -1;
        }

        phys = alloc_page_frame();
        if (!phys)
                return -1;

        vaddr = (void *)(phys + 0xC0000000);
        ret = inode->i_sb->s_op->readpage(inode, pgoff, vaddr, tier);
        if (ret < 0)
        {
                free_page_frame(phys);
                return -1;
        }

        cache[slot].ino = inode->i_ino;
        cache[slot].pgoff = pgoff;
        cache[slot].phys = phys;
        cache[slot].valid = 1;
        cache[slot].tier = tier;

        memcpy(buf, vaddr, 4096);
        return 4096;
}
