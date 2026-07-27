#include <kernel/paging.h>
#include <driver/vga.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Memory layout
 */
#define PAGE_SIZE		4096
#define PAGE_ALIGN(addr)	(((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* Physical memory size (16 MB) */
#define PHYS_MEM_SIZE		(16 * 1024 * 1024)
#define NUM_PAGES		(PHYS_MEM_SIZE / PAGE_SIZE)

/* Start allocating physical frames from 4 MB (index 1024) */
#define PAGE_START_INDEX	1024

/*
 * Page directory and page table for identity mapping (0-4MB)
 */
static uint32_t page_directory[1024] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_table_0[1024] __attribute__((aligned(PAGE_SIZE)));

/*
 * Physical memory bitmap (1 bit per page)
 */
static uint8_t phys_bitmap[NUM_PAGES / 8];

/*
 * Internal: allocate a physical page (bitmap based)
 */
static uint32_t _alloc_phys_page(void)
{
	uint32_t i;

	for (i = PAGE_START_INDEX; i < NUM_PAGES; i++) {
		uint32_t byte = i / 8;
		uint32_t bit = i % 8;

		if (!(phys_bitmap[byte] & (1 << bit))) {
			phys_bitmap[byte] |= (1 << bit);
			return i * PAGE_SIZE;
		}
	}
	return 0;	/* out of memory */
}

/*
 * Internal: free a physical page
 */
static void _free_phys_page(uint32_t phys_addr)
{
	uint32_t idx = phys_addr / PAGE_SIZE;

	if (idx >= PAGE_START_INDEX && idx < NUM_PAGES) {
		uint32_t byte = idx / 8;
		uint32_t bit = idx % 8;
		phys_bitmap[byte] &= ~(1 << bit);
	}
}

/*
 * Public: allocate a page frame
 */
uint32_t alloc_page_frame(void)
{
	return _alloc_phys_page();
}

/*
 * Public: free a page frame
 */
void free_page_frame(uint32_t phys_addr)
{
	_free_phys_page(phys_addr);
}

/*
 * Map a virtual address to a physical address.
 * Uses recursive page directory mapping:
 *   - PDE at 0xFFFFF000
 *   - PTE for a given PD index at 0xFFC00000 + (pd_idx << 12)
 */
void map_page(void *virt, void *phys, uint32_t flags)
{
        uint32_t vaddr = (uint32_t)virt;
        uint32_t paddr = (uint32_t)phys;
        uint32_t pd_idx = vaddr >> 22;
        uint32_t pt_idx = (vaddr >> 12) & 0x3FF;
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t *pt;

        /* Check if page table exists */
        if (!(pd[pd_idx] & PTE_PRESENT)) {
                uint32_t pt_phys = _alloc_phys_page();
                if (!pt_phys) {
                        vga_write("[PAGING] map_page: out of phys memory for PT\n");
                        return;
                }
                /* Set PDE with present, RW (and optionally user) */
                pd[pd_idx] = pt_phys | PTE_PRESENT | PTE_RW;
                /* Zero the new page table via recursive mapping */
                uint32_t *pt_virt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                int i;
                for (i = 0; i < 1024; i++)
                        pt_virt[i] = 0;
        }

        /* Now set the PTE */
        pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
        pt[pt_idx] = paddr | (flags & 0xFFF);
}
/*
 * Unmap a virtual page (clear PTE)
 */
void unmap_page(void *virt)
{
	uint32_t vaddr = (uint32_t)virt;
	uint32_t pd_idx = vaddr >> 22;
	uint32_t pt_idx = (vaddr >> 12) & 0x3FF;
	uint32_t *pd = (uint32_t *)0xFFFFF000;
	uint32_t *pt;

	if (!(pd[pd_idx] & PTE_PRESENT))
		return;	/* no PT, nothing to unmap */

	pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
	pt[pt_idx] = 0;		/* clear PTE */
}

/*
 * Initialize paging:
 *   - Identity map 0-4MB (kernel space)
 *   - Set up recursive mapping (PDE[1023] points to itself)
 *   - Enable paging (CR0.PG)
 * Returns aligned mem_start.
 */
unsigned long paging_init(unsigned long mem_start, unsigned long mem_end)
{
	uint32_t i;

	/* Zero page directory and first page table */
	for (i = 0; i < 1024; i++) {
		page_directory[i] = 0;
		page_table_0[i] = 0;
	}

	/* Identity map first 4 MB (0x00000000 - 0x00400000) */
	for (i = 0; i < 1024; i++) {
		page_table_0[i] = (i * PAGE_SIZE) | PTE_PRESENT | PTE_RW;
	}

	/* Set PDE[0] to point to page_table_0 */
	page_directory[0] = ((uint32_t)page_table_0) | PTE_PRESENT | PTE_RW;

	/* Recursive mapping: PDE[1023] points to itself */
	page_directory[1023] = ((uint32_t)page_directory) | PTE_PRESENT | PTE_RW;

	/* Load page directory base (CR3) */
	__asm__ volatile ("mov %0, %%cr3" : : "r" (page_directory) : "memory");

	/* Enable paging (set CR0.PG) */
	uint32_t cr0;
	__asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
	cr0 |= 0x80000000;
	__asm__ volatile ("mov %0, %%cr0" : : "r" (cr0));

	/* Return aligned mem_start (for further memory setup) */
	return PAGE_ALIGN(mem_start);
}
