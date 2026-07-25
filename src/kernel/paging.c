#include <kernel/paging.h>
#include <driver/vga.h>

#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

static unsigned int page_directory[1024] __attribute__((aligned(PAGE_SIZE)));
static unsigned int page_table_0[1024] __attribute__((aligned(PAGE_SIZE)));

unsigned long paging_init(unsigned long mem_start, unsigned long mem_end) {
        unsigned int i;

        for (i = 0; i < 1024; i++) {
                page_directory[i] = 0;
                page_table_0[i] = 0;
        }

        for (i = 0; i < 1024; i++) {
                page_table_0[i] = (i * PAGE_SIZE) | 0x03;
        }

        page_directory[0] = ((unsigned int)page_table_0) | 0x03;

        __asm__ volatile ("mov %0, %%cr3" : : "r" (page_directory) : "memory");

        unsigned int cr0;
        __asm__ volatile ("mov %%cr0, %0" : "=r" (cr0));
        cr0 |= 0x80000000;
        __asm__ volatile ("mov %0, %%cr0" : : "r" (cr0));

        return PAGE_ALIGN(mem_start);
}
