#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <stdint.h>

#define PTE_PRESENT      0x001
#define PTE_RW           0x002
#define PTE_USER         0x004
#define PTE_WRITETHROUGH 0x008
#define PTE_CACHE_DISABLE 0x010
#define PTE_ACCESSED     0x020
#define PTE_DIRTY        0x040
#define PTE_PAT          0x080
#define PTE_GLOBAL       0x100
#define PTE_PS           0x080
#define PAGE_SIZE 4096
#define NUM_PAGES (64 * 1024 * 1024 / PAGE_SIZE)
unsigned long paging_init(unsigned long mem_start, unsigned long mem_end);
uint32_t alloc_page_frame(void);
void free_page_frame(uint32_t phys_addr);
void map_page(void *virt, void *phys, uint32_t flags);
void unmap_page(void *virt);
int page_present(void *virt);
uint32_t paging_create_task_pd(void);
void load_cr3(uint32_t pd);

#endif
