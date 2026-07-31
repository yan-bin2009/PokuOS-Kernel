#include <kernel/paging.h>
#include <driver/vga.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096

#define PHYS_MEM_SIZE   (16 * 1024 * 1024)
#define NUM_PAGES       (PHYS_MEM_SIZE / PAGE_SIZE)
#define PAGE_START_INDEX 1024

static uint32_t page_directory[1024] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_table_0[1024] __attribute__((aligned(PAGE_SIZE)));
static uint8_t phys_bitmap[NUM_PAGES / 8];

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
        return 0;
}

static void _free_phys_page(uint32_t phys_addr)
{
        uint32_t idx = phys_addr / PAGE_SIZE;

        if (idx >= PAGE_START_INDEX && idx < NUM_PAGES) {
                uint32_t byte = idx / 8;
                uint32_t bit = idx % 8;
                phys_bitmap[byte] &= ~(1 << bit);
        }
}

uint32_t alloc_page_frame(void)
{
        return _alloc_phys_page();
}

void free_page_frame(uint32_t phys_addr)
{
        _free_phys_page(phys_addr);
}

void map_page(void *virt, void *phys, uint32_t flags)
{
        uint32_t vaddr = (uint32_t)virt;
        uint32_t paddr = (uint32_t)phys;
        uint32_t pd_idx = vaddr >> 22;
        uint32_t pt_idx = (vaddr >> 12) & 0x3FF;
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t *pt;

        if (!(pd[pd_idx] & PTE_PRESENT)) {
                uint32_t pt_phys = _alloc_phys_page();
                uint32_t *pt_virt;
                int i;

                if (!pt_phys)
                        return;

                pd[pd_idx] = pt_phys | PTE_PRESENT | PTE_RW | PTE_USER;
                pt_virt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                for (i = 0; i < 1024; i++)
                        pt_virt[i] = 0;
        }

        pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
        pt[pt_idx] = paddr | (flags & 0xFFF);
}

void unmap_page(void *virt)
{
        uint32_t vaddr = (uint32_t)virt;
        uint32_t pd_idx = vaddr >> 22;
        uint32_t pt_idx = (vaddr >> 12) & 0x3FF;
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t *pt;

        if (!(pd[pd_idx] & PTE_PRESENT))
                return;

        pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
        pt[pt_idx] = 0;
}

unsigned long paging_init(unsigned long mem_start, unsigned long mem_end)
{
        uint32_t i;

        for (i = 0; i < 1024; i++) {
                page_directory[i] = 0;
                page_table_0[i] = 0;
        }

        for (i = 0; i < 1024; i++)
                page_table_0[i] = (i * PAGE_SIZE) | PTE_PRESENT | PTE_RW | PTE_USER;

        page_directory[0] = ((uint32_t)page_table_0) | PTE_PRESENT | PTE_RW | PTE_USER;
        page_directory[1023] = ((uint32_t)page_directory) | PTE_PRESENT | PTE_RW;

        __asm__ volatile ("mov %0, %%cr3" : : "r" (page_directory) : "memory");

        uint32_t cr0;
        __asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
        cr0 |= 0x80000000;
        __asm__ volatile ("mov %0, %%cr0" : : "r" (cr0));

        return PAGE_SIZE * ((mem_start + PAGE_SIZE - 1) / PAGE_SIZE);
}
