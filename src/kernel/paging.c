#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>
#include <vm/vm.h>

#define PAGE_SIZE 4096

#define PHYS_MEM_SIZE (16 * 1024 * 1024)
#define NUM_PAGES (PHYS_MEM_SIZE / PAGE_SIZE)
#define PAGE_START_INDEX 1024

int pressure_triggered = 0;

struct vm_page vm_page_array[NUM_PAGES];
static TAILQ_HEAD(free_page_list, vm_page) vm_page_freeq;
uint32_t page_directory[1024] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_table_0[1024] __attribute__((aligned(PAGE_SIZE)));
static uint32_t page_table_physmap[4][1024] __attribute__((aligned(PAGE_SIZE)));

#define PHYS_MAP_BASE 0xC0000000

static uint32_t _alloc_phys_page(void)
{
        struct vm_page *pg;

        pg = TAILQ_FIRST(&vm_page_freeq);
        if (!pg)
                return 0;
        TAILQ_REMOVE(&vm_page_freeq, pg, pageq);
        pg->flags &= ~PG_FREE;
        return pg->phys_addr;
}

static void _free_phys_page(uint32_t phys_addr)
{
        uint32_t idx;
        struct vm_page *pg;

        idx = phys_addr >> 12;
        if (idx >= PAGE_START_INDEX && idx < NUM_PAGES)
        {
                pg = &vm_page_array[idx];
                pg->flags |= PG_FREE;
                TAILQ_INSERT_HEAD(&vm_page_freeq, pg, pageq);
        }
}

uint32_t free_pages_count(void)
{
        struct vm_page *pg;
        uint32_t count = 0;

        TAILQ_FOREACH(pg, &vm_page_freeq, pageq)
                count++;
        return count;
}

uint32_t alloc_page_frame(void)
{
        uint32_t phys;
        uint32_t idx;

        phys = _alloc_phys_page();
        if (phys)
        {
                idx = phys >> 12;
                if (idx < NUM_PAGES)
                {
                        vm_page_array[idx].ref_count = 1;
                }
                if (phys < 0x400000)
                {
                        serial_write("[ALLOC LOW] phys=");
                        serial_write_hex(phys);
                        serial_write("\n");
                }
                return phys;
        }

        vm_handle_pressure();

        phys = _alloc_phys_page();
        if (phys)
        {
                idx = phys >> 12;
                if (idx < NUM_PAGES)
                {
                        vm_page_array[idx].ref_count = 1;
                }
                return phys;
        }

        serial_write("[PAGE] alloc failed after pressure\n");
        return 0;
}

uint32_t paging_create_task_pd(void)
{
        uint32_t pd_phys;
        uint32_t *new_pd;
        uint32_t *kernel_pd;
        int i;

        pd_phys = alloc_page_frame();
        if (!pd_phys)
                return 0;

        map_page((void *)0xE0002000, (void *)pd_phys, PTE_PRESENT | PTE_RW);
        new_pd = (uint32_t *)0xE0002000;
        kernel_pd = (uint32_t *)0xFFFFF000;

        for (i = 0; i < 1024; i++)
        {
                if (i == 0 || i >= 768)
                        new_pd[i] = kernel_pd[i];
                else
                        new_pd[i] = 0;
        }
        new_pd[1023] = pd_phys | PTE_PRESENT | PTE_RW;

        unmap_page((void *)0xE0002000);
        return pd_phys;
}

void load_cr3(uint32_t pd)
{
        __asm__ volatile("mov %0, %%cr3" : : "r"(pd) : "memory");
}

void free_page_frame(uint32_t phys)
{
        uint32_t idx;

        idx = phys >> 12;
        if (idx < NUM_PAGES)
        {
                if (vm_page_array[idx].ref_count > 0)
                {
                        vm_page_array[idx].ref_count--;
                }
                if (vm_page_array[idx].ref_count > 0)
                {
                        return;
                }
        }

        _free_phys_page(phys);
}

void map_page(void *virt, void *phys, uint32_t flags)
{
        uint32_t vaddr = (uint32_t)virt;
        uint32_t paddr = (uint32_t)phys;
        uint32_t pd_idx = vaddr >> 22;
        uint32_t pt_idx = (vaddr >> 12) & 0x3FF;
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t *pt;

        if (!(pd[pd_idx] & PTE_PRESENT))
        {
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
        __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
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
        __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

unsigned long paging_init(unsigned long mem_start, unsigned long mem_end)
{
        uint32_t i;

        for (i = 0; i < 1024; i++)
        {
                page_directory[i] = 0;
                page_table_0[i] = 0;
        }

        for (i = 0; i < 1024; i++)
                page_table_0[i] = (i * PAGE_SIZE) | PTE_PRESENT | PTE_RW;

        TAILQ_INIT(&vm_page_freeq);
        for (i = 0; i < NUM_PAGES; i++)
        {
                vm_page_array[i].phys_addr = i * PAGE_SIZE;
                vm_page_array[i].ref_count = 0;
                vm_page_array[i].flags = PG_FREE;
                vm_page_array[i].object = NULL;
                if (i >= PAGE_START_INDEX)
                        TAILQ_INSERT_TAIL(&vm_page_freeq, &vm_page_array[i], pageq);
        }

        page_directory[0] = ((uint32_t)page_table_0) | PTE_PRESENT | PTE_RW;
        page_directory[1023] = ((uint32_t)page_directory) | PTE_PRESENT | PTE_RW;

        for (i = 0; i < 4; i++)
        {
                uint32_t j;

                page_directory[768 + i] =
                    ((uint32_t)page_table_physmap[i]) | PTE_PRESENT | PTE_RW;
                for (j = 0; j < 1024; j++)
                        page_table_physmap[i][j] =
                            ((i * 1024 + j) * PAGE_SIZE) | PTE_PRESENT | PTE_RW;
        }

        __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory) : "memory");

        uint32_t cr0;
        __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 |= 0x80000000;
        __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

        return PAGE_SIZE * ((mem_start + PAGE_SIZE - 1) / PAGE_SIZE);
}