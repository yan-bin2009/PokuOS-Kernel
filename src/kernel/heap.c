#include <driver/vga.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/serial.h>
#include <stddef.h>
#include <stdint.h>

#define HEAP_VIRT_START 0xD0000000
#define HEAP_INIT_PAGES 64
#define PAGE_SIZE 4096

#define BLOCK_FLAG_FREE 0x1
#define BLOCK_FLAG_PREV_FREE 0x2
#define BLOCK_SIZE_MASK (~(size_t)0x3)
#define MIN_BLOCK_SIZE 16

struct block_header
{
        size_t size;
};

struct free_block
{
        struct block_header header;
        struct free_block *next;
};

static struct free_block *free_list;
static uintptr_t heap_brk = HEAP_VIRT_START;
static uintptr_t heap_end = HEAP_VIRT_START;

static size_t *block_footer(struct block_header *block);

static int expand_heap(uintptr_t vaddr)
{
        uint32_t phys = alloc_page_frame();

        if (!phys)
                return -1;
        map_page((void *)vaddr, (void *)phys, PTE_PRESENT | PTE_RW);
        return 0;
}

void heap_init(void)
{
        uint32_t i;

        for (i = 0; i < HEAP_INIT_PAGES; i++)
                expand_heap(heap_brk + i * PAGE_SIZE);

        free_list = (struct free_block *)HEAP_VIRT_START;
        free_list->header.size = (HEAP_INIT_PAGES * PAGE_SIZE) | BLOCK_FLAG_FREE;
        free_list->next = NULL;
        *block_footer((struct block_header *)free_list) =
            HEAP_INIT_PAGES * PAGE_SIZE;

        heap_brk += HEAP_INIT_PAGES * PAGE_SIZE;
        heap_end = heap_brk;
}

static size_t block_size(const struct block_header *block)
{
        return block->size & BLOCK_SIZE_MASK;
}

static int block_is_free(const struct block_header *block)
{
        return block->size & BLOCK_FLAG_FREE;
}

static void *block_payload(struct block_header *block)
{
        return (void *)((uintptr_t)block + sizeof(struct block_header));
}

static struct block_header *block_after(struct block_header *block)
{
        return (struct block_header *)((uintptr_t)block + block_size(block));
}

static size_t *block_footer(struct block_header *block)
{
        return (size_t *)((uintptr_t)block + block_size(block) - sizeof(size_t));
}

static int block_in_heap(const struct block_header *block)
{
        return (uintptr_t)block >= HEAP_VIRT_START && (uintptr_t)block + sizeof(struct block_header) <= heap_end;
}

static void insert_free(struct free_block *block)
{
        struct free_block *prev, *curr;

        prev = NULL;
        curr = free_list;
        while (curr && (uintptr_t)curr < (uintptr_t)block)
        {
                prev = curr;
                curr = curr->next;
        }
        block->next = curr;
        if (prev)
                prev->next = block;
        else
                free_list = block;
}

static void remove_free(struct free_block *block)
{
        struct free_block *prev, *curr;

        prev = NULL;
        curr = free_list;
        while (curr)
        {
                if (curr == block)
                {
                        if (prev)
                                prev->next = curr->next;
                        else
                                free_list = curr->next;
                        return;
                }
                prev = curr;
                curr = curr->next;
        }
}

static int grow_heap(size_t need_bytes)
{
        struct block_header *prev;
        size_t prev_size;
        size_t pages;
        size_t i;

        if (need_bytes == 0)
                return -1;
        if (heap_brk >= 0xFE000000)
                return -1;

        pages = (need_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

        for (i = 0; i < pages; i++)
        {
                if (expand_heap(heap_brk + i * PAGE_SIZE) != 0)
                        return -1;
        }

        /* 若前一块 free，合并扩展区，保持 free list 连续 */
        if (heap_brk > HEAP_VIRT_START)
        {
                prev_size = *(size_t *)((uintptr_t)heap_brk - sizeof(size_t));
                prev = (struct block_header *)((uintptr_t)heap_brk - prev_size);
                if (block_in_heap(prev) &&
                    (uintptr_t)prev + prev_size == (uintptr_t)heap_brk &&
                    block_is_free(prev))
                {
                        remove_free((struct free_block *)prev);
                        prev->size = (prev_size + pages * PAGE_SIZE) |
                                     BLOCK_FLAG_FREE |
                                     (prev->size & BLOCK_FLAG_PREV_FREE);
                        heap_brk += pages * PAGE_SIZE;
                        heap_end = heap_brk;
                        *block_footer(prev) = block_size(prev);
                        ((struct free_block *)prev)->next = NULL;
                        insert_free((struct free_block *)prev);
                        return 0;
                }
        }

        {
                struct block_header *newblock;

                newblock = (struct block_header *)heap_brk;
                newblock->size = (pages * PAGE_SIZE) | BLOCK_FLAG_FREE;
                *block_footer(newblock) = pages * PAGE_SIZE;
                ((struct free_block *)newblock)->next = NULL;
                heap_brk += pages * PAGE_SIZE;
                heap_end = heap_brk;
                insert_free((struct free_block *)newblock);
        }
        return 0;
}

void *kmalloc(size_t size)
{
        struct free_block *fb;
        struct block_header *block;
        size_t total_size;
        size_t remaining;

        if (!size)
                return NULL;

        size = (size + 3) & ~(size_t)0x03;
        total_size = size + sizeof(struct block_header) + sizeof(size_t);
        if (total_size < MIN_BLOCK_SIZE)
                total_size = MIN_BLOCK_SIZE;

        for (fb = free_list; fb; fb = fb->next)
        {
                if (block_size(&fb->header) >= total_size)
                        break;
        }
        if (!fb)
        {
                if (grow_heap(total_size) != 0)
                        return NULL;
                for (fb = free_list; fb; fb = fb->next)
                {
                        if (block_size(&fb->header) >= total_size)
                                break;
                }
                if (!fb)
                        return NULL;
        }

        block = &fb->header;
        remove_free(fb);

        remaining = block_size(block) - total_size;
        if (remaining >= MIN_BLOCK_SIZE)
        {
                struct block_header *split;
                struct block_header *after_split;

                split = (struct block_header *)((uintptr_t)block + total_size);
                split->size = remaining | BLOCK_FLAG_FREE;
                after_split = block_after(split);
                if (block_in_heap(after_split))
                        after_split->size |= BLOCK_FLAG_PREV_FREE;
                *block_footer(split) = remaining;
                insert_free((struct free_block *)split);
                block->size = total_size | (block->size & BLOCK_FLAG_PREV_FREE);
        }

        block->size &= ~BLOCK_FLAG_FREE;
        *block_footer(block) = block_size(block);

        {
                struct block_header *next;

                next = block_after(block);
                if (block_in_heap(next))
                        next->size &= ~BLOCK_FLAG_PREV_FREE;
        }

        return block_payload(block);
}

void kfree(void *ptr)
{
        struct block_header *block;
        struct block_header *next;

        if (!ptr)
                return;

        block = (struct block_header *)((uintptr_t)ptr - sizeof(struct block_header));
        if (!block_in_heap(block) || block_size(block) == 0)
                return;

        if (block->size & BLOCK_FLAG_PREV_FREE)
        {
                size_t prev_size;
                struct block_header *prev;

                prev_size = *(size_t *)((uintptr_t)block - sizeof(size_t));
                prev = (struct block_header *)((uintptr_t)block - prev_size);
                if (block_in_heap(prev) && (uintptr_t)prev + prev_size == (uintptr_t)block)
                {
                        remove_free((struct free_block *)prev);
                        prev->size = (prev_size + block_size(block)) | BLOCK_FLAG_FREE | (prev->size & BLOCK_FLAG_PREV_FREE);
                        block = prev;
                }
        }

        next = block_after(block);
        if (block_in_heap(next) && block_is_free(next))
        {
                remove_free((struct free_block *)next);
                block->size = (block_size(block) + block_size(next)) | BLOCK_FLAG_FREE | (block->size & BLOCK_FLAG_PREV_FREE);
        }

        next = block_after(block);
        if (block_in_heap(next))
                next->size |= BLOCK_FLAG_PREV_FREE;

        block->size |= BLOCK_FLAG_FREE;
        *block_footer(block) = block_size(block);
        ((struct free_block *)block)->next = NULL;
        insert_free((struct free_block *)block);
}

static uint32_t heap_free_block_count(void)
{
        struct free_block *fb;
        uint32_t n = 0;

        for (fb = free_list; fb; fb = fb->next)
                n++;
        return n;
}

static uint32_t heap_largest_free(void)
{
        struct free_block *fb;
        uint32_t max = 0;

        for (fb = free_list; fb; fb = fb->next)
        {
                uint32_t s = block_size(&fb->header);

                if (s > max)
                        max = s;
        }
        return max;
}

void heap_selftest(void)
{
        void *a, *b, *c, *d, *e, *big;
        uint32_t before, after;

        serial_write("[heap] selftest start\n");

        a = kmalloc(64);
        b = kmalloc(256);
        c = kmalloc(64);
        d = kmalloc(512);
        e = kmalloc(128);
        serial_write("[heap] 5 blocks allocated\n");

        kfree(b);
        kfree(d);
        serial_write("[heap] freed b,d (external frag created)\n");
        before = heap_free_block_count();
        serial_write("[heap] free blocks=");
        serial_write_hex(before);
        serial_write("\n");

        big = kmalloc(600);
        serial_write(big ? "[heap] big 600 alloc ok\n"
                         : "[heap] big 600 alloc FAILED\n");

        kfree(a);
        serial_write("[heap] freed a\n");
        kfree(c);
        serial_write("[heap] freed c\n");
        kfree(e);
        serial_write("[heap] freed e\n");
        kfree(big);
        serial_write("[heap] freed big\n");
        after = heap_free_block_count();
        serial_write("[heap] after full free blocks=");
        serial_write_hex(after);
        serial_write(" largest=");
        serial_write_hex(heap_largest_free());
        serial_write("\n");

        if (heap_free_block_count() == 1)
                serial_write("[heap] COALESCE OK\n");
        else
                serial_write("[heap] COALESCE WARN: fragmented\n");

        serial_write("[heap] selftest done\n");
}
