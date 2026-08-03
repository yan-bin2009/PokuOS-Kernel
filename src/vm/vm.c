/*
 * 这个是仿制freebsd的vm虚拟内存机制
 * 大爱freebsd
 * 感谢freebsd的馈赠
 */
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <stdint.h>
#include <sys/queue.h>
#include <vm/vm.h>

#define VM_PAGE_SIZE 4096

extern int pressure_triggered;

struct vm_object *vm_object_alloc(uint32_t pages)
{
        struct vm_object *obj;

        obj = (struct vm_object *)kmalloc(sizeof(struct vm_object));
        if (!obj)
                return NULL;
        obj->ref_count = 1;
        obj->size = pages;
        obj->flags = OBJ_ANON;
        obj->shadow = NULL;
        TAILQ_INIT(&obj->memq);
        return obj;
}

static uint32_t vm_count_user_pages(void)
{
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t i, j, n = 0;

        for (i = 1; i < 768; i++)
        {
                uint32_t *pt;

                if (!(pd[i] & 1))
                        continue;
                pt = (uint32_t *)(0xFFC00000 + (i << 12));
                for (j = 0; j < 1024; j++)
                {
                        if (pt[j] & 1)
                                n++;
                }
        }
        return n;
}

static void vm_scan_user_pages(struct vm_object *obj)
{
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t i, j;

        for (i = 1; i < 768; i++)
        {
                uint32_t *pt;

                if (!(pd[i] & 1))
                        continue;
                pt = (uint32_t *)(0xFFC00000 + (i << 12));
                for (j = 0; j < 1024; j++)
                {
                        struct vm_page *pg;

                        if (!(pt[j] & 1))
                                continue;
                        pg = &vm_page_array[pt[j] >> 12];
                        pg->object = obj;
                        pg->ref_count = 1;
                        pg->flags = PG_BUSY;
                        TAILQ_INSERT_TAIL(&obj->memq, pg, listq);
                }
        }
}

struct vm_map *vm_map_create(void)
{
        struct vm_map *map;
        uint32_t n;
        uint32_t cr3;

        n = vm_count_user_pages();
        map = (struct vm_map *)kmalloc(sizeof(struct vm_map));
        if (!map)
                return NULL;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        map->page_directory = cr3;
        map->user_object = vm_object_alloc(n);
        if (!map->user_object)
        {
                kfree(map);
                return NULL;
        }
        vm_scan_user_pages(map->user_object);
        return map;
}

void vm_object_reference(struct vm_object *obj)
{
        if (obj)
                obj->ref_count++;
}

void vm_copy_phys(uint32_t old_phys, uint32_t new_phys)
{
        uint32_t *old_v;
        uint32_t *new_v;
        uint32_t i;

        map_page((void *)0xE0000000, (void *)old_phys, PTE_PRESENT | PTE_RW);
        map_page((void *)0xE0001000, (void *)new_phys, PTE_PRESENT | PTE_RW);
        old_v = (uint32_t *)0xE0000000;
        new_v = (uint32_t *)0xE0001000;
        for (i = 0; i < 1024; i++)
                new_v[i] = old_v[i];
        unmap_page((void *)0xE0000000);
        unmap_page((void *)0xE0001000);
}

int vm_fault_cow(struct vm_map *map, uint32_t vaddr)
{
        uint32_t *pd;
        uint32_t *pt;
        uint32_t pd_idx;
        uint32_t pt_idx;
        uint32_t pte;
        uint32_t old_phys;
        uint32_t new_phys;
        struct vm_page *old_pg;
        static uint32_t cow_count;

        if (!map)
                return -1;
        pd_idx = vaddr >> 22;
        pt_idx = (vaddr >> 12) & 0x3ff;
        pd = (uint32_t *)0xFFFFF000;
        if (!(pd[pd_idx] & PTE_PRESENT))
                return -1;
        pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
        pte = pt[pt_idx];
        if (!(pte & PTE_PRESENT))
                return -1;
        if (!(pte & PTE_USER))
                return -1;
        if (pte & PTE_RW)
                return 0;
        old_phys = pte & 0xFFFFF000;
        old_pg = &vm_page_array[old_phys >> 12];
        if (old_pg->ref_count == 1)
        {
                pt[pt_idx] |= PTE_RW;
                __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
                return 0;
        }
        new_phys = alloc_page_frame();
        if (!new_phys)
                return -1;
        vm_copy_phys(old_phys, new_phys);
        free_page_frame(old_phys);
        pt[pt_idx] = new_phys | (pte & 0xFFF) | PTE_RW;
        __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
        cow_count++;
        serial_write("COW #");
        serial_write_hex(cow_count);
        serial_write(" at ");
        serial_write_hex(vaddr);
        serial_write(" old=");
        serial_write_hex(old_phys);
        serial_write(" new=");
        serial_write_hex(new_phys);
        serial_write(" pt=");
        serial_write_hex(pt[pt_idx]);
        serial_write("\n");
        return 0;
}

int vm_fault(struct vm_map *map, uint32_t vaddr, uint32_t error_code)
{
        if (map && (error_code & 4) && (error_code & 1) && (error_code & 2))
                return vm_fault_cow(map, vaddr);
        return -1;
}

void vm_protect_readonly(struct vm_map *map)
{
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t i, j;

        for (i = 1; i < 768; i++)
        {
                uint32_t *pt;

                if (!(pd[i] & 1))
                        continue;
                pt = (uint32_t *)(0xFFC00000 + (i << 12));
                for (j = 0; j < 1024; j++)
                {
                        uint32_t phys;

                        if (!(pt[j] & 1) || !(pt[j] & PTE_USER))
                                continue;
                        pt[j] &= ~PTE_RW;
                        phys = pt[j] & 0xFFFFF000;
                        vm_page_array[phys >> 12].ref_count = 2;
                        __asm__ volatile("invlpg (%0)" : : "r"((uint32_t)(i << 22 | j << 12)) : "memory");
                }
        }
}

struct vm_object *vm_object_shadow(struct vm_object *backing)
{
        struct vm_object *obj;

        obj = vm_object_alloc(0);
        if (!obj)
                return NULL;
        obj->shadow = backing;
        obj->flags = OBJ_COW;
        if (backing)
                backing->ref_count++;
        return obj;
}

void vm_object_deallocate(struct vm_object *obj)
{
        struct vm_object *next;

        if (!obj)
                return;

        while (1)
        {
                struct vm_page *pg;

                if (obj->ref_count == 0)
                        return;
                if (--obj->ref_count > 0)
                        return;
                obj->flags |= OBJ_DEAD;
                while (!TAILQ_EMPTY(&obj->memq))
                {
                        pg = TAILQ_FIRST(&obj->memq);
                        TAILQ_REMOVE(&obj->memq, pg, listq);
                        free_page_frame(pg->phys_addr);
                }
                next = obj->shadow;
                obj->shadow = NULL;
                kfree(obj);
                if (!next)
                        return;
                obj = next;
        }
}

void vm_handle_pressure(void)
{
        task_t *current = get_current_task();
        if (!current)
                return;

        uint32_t free = free_pages_count();

        if (free < FREE_PAGES_CRITICAL && current->tier == TIER_USER) {
                current->tier = TIER_CRITICAL;
                serial_write("[PRESSURE] CRITICAL triggered, pid=");
                serial_write_hex(current->pid);
                serial_write(" free_pages=");
                serial_write_hex(free);
                serial_write("\n");
                pressure_triggered = 1;
        } else if (free < FREE_PAGES_WARN) {
                serial_write("[PRESSURE] WARN: low memory, free_pages=");
                serial_write_hex(free);
                serial_write("\n");
        }
}

void vm_release_pressure(void)
{
        task_t *current = get_current_task();
        if (!current)
                return;

        uint32_t free = free_pages_count();

        if (pressure_triggered && current->tier == TIER_CRITICAL && free >= FREE_PAGES_WARN) {
                current->tier = TIER_USER;
                pressure_triggered = 0;
                serial_write("[PRESSURE] released, pid=");
                serial_write_hex(current->pid);
                serial_write(" free_pages=");
                serial_write_hex(free);
                serial_write("\n");
        }
}

void vm_init(void)
{
}