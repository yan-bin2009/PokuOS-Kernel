#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <stdint.h>

/*
 * Page table entry flags (x86)
 */
#define PTE_PRESENT	0x001
#define PTE_RW		0x002
#define PTE_USER	0x004
#define PTE_WRITETHROUGH 0x008
#define PTE_CACHE_DISABLE 0x010
#define PTE_ACCESSED	0x020
#define PTE_DIRTY	0x040
#define PTE_PAT		0x080
#define PTE_GLOBAL	0x100
#define PTE_PS		0x080	/* Page size (for PDE) */

/*
 * Public interface
 */

/* Initialize paging: sets up identity mapping for low memory,
 * recursive page directory mapping, and enables paging.
 * Returns the aligned start of free memory (mem_start rounded up).
 */
unsigned long paging_init(unsigned long mem_start, unsigned long mem_end);

/* Allocate a physical page frame (4KB), returns physical address or 0 on failure */
uint32_t alloc_page_frame(void);

/* Free a physical page frame */
void free_page_frame(uint32_t phys_addr);

/* Map a virtual page to a physical page with given flags */
void map_page(void *virt, void *phys, uint32_t flags);

/* Optional: unmap a virtual page (clears PTE) */
void unmap_page(void *virt);

#endif
