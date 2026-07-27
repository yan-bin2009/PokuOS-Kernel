#include <kernel/heap.h>
#include <kernel/paging.h>
#include <driver/vga.h>
#include <stddef.h>
#include <stdint.h>

#define HEAP_VIRT_START	0xD0000000
#define HEAP_INIT_PAGES	64
#define PAGE_SIZE	4096

struct block_header {
	size_t size;
	struct block_header *next;
};

static struct block_header *free_list;
static uintptr_t heap_brk = HEAP_VIRT_START;

static void expand_heap(uintptr_t vaddr)
{
	uint32_t phys = alloc_page_frame();
	if (!phys) {
		vga_write("[HEAP] out of physical memory\n");
		return;
	}
	map_page((void *)vaddr, (void *)phys, PTE_PRESENT | PTE_RW);
}

void heap_init(void)
{
	uint32_t i;

	for (i = 0; i < HEAP_INIT_PAGES; i++)
		expand_heap(heap_brk + i * PAGE_SIZE);

	free_list = (struct block_header *)HEAP_VIRT_START;
	free_list->size = HEAP_INIT_PAGES * PAGE_SIZE;
	free_list->next = NULL;

	heap_brk += HEAP_INIT_PAGES * PAGE_SIZE;

	vga_write("[HEAP] initialized at 0xD0000000, 256 KB\n");
}

void *kmalloc(size_t size)
{
	struct block_header *prev, *curr;
	size_t total_size;
	size_t remaining;

	if (!size)
		return NULL;

	size = (size + 3) & ~0x03;	/* 4-byte align */
	total_size = size + sizeof(struct block_header);

	prev = NULL;
	curr = free_list;

	while (curr) {
		if (curr->size >= total_size)
			break;
		prev = curr;
		curr = curr->next;
	}

	if (!curr) {
		vga_write("[HEAP] kmalloc: out of free blocks\n");
		return NULL;
	}

	remaining = curr->size - total_size;
	if (remaining > sizeof(struct block_header)) {
		struct block_header *new_block;

		new_block = (struct block_header *)((uintptr_t)curr + total_size);
		new_block->size = remaining;
		new_block->next = curr->next;

		curr->size = total_size;

		if (prev)
			prev->next = new_block;
		else
			free_list = new_block;
	} else {
		if (prev)
			prev->next = curr->next;
		else
			free_list = curr->next;
	}

	return (void *)((uintptr_t)curr + sizeof(struct block_header));
}

void kfree(void *ptr)
{
	struct block_header *block;

	if (!ptr)
		return;

	block = (struct block_header *)((uintptr_t)ptr - sizeof(struct block_header));

	/* Insert at head (simple, O(1)) */
	block->next = free_list;
	free_list = block;
}
