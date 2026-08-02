#ifndef _VM_VM_H
#define _VM_VM_H

#include <stdint.h>
#include <sys/queue.h>

#define PHYS_PAGE_COUNT (1024 * 1024)

#define OBJ_ANON        0x01
#define OBJ_COW         0x02
#define OBJ_DEAD        0x04

#define PG_BUSY         0x01
#define PG_FREE         0x02

struct vm_object;

struct vm_page
{
        TAILQ_ENTRY(vm_page) pageq;
        TAILQ_ENTRY(vm_page) listq;
        struct vm_object *object;
        uint32_t phys_addr;
        uint32_t ref_count;
        uint32_t flags;
};

extern struct vm_page vm_page_array[];

struct vm_object
{
        TAILQ_HEAD(, vm_page) memq;
        struct vm_object *shadow;
        uint32_t ref_count;
        uint32_t size;
        uint32_t flags;
};

struct vm_map
{
        struct vm_object *user_object;
        uint32_t page_directory;
};

struct vm_object *vm_object_alloc(uint32_t pages);

struct vm_map *vm_map_create(void);

void vm_object_reference(struct vm_object *obj);

void vm_object_deallocate(struct vm_object *obj);

struct vm_object *vm_object_shadow(struct vm_object *backing);

int vm_fault_cow(struct vm_map *map, uint32_t vaddr);

int vm_fault(struct vm_map *map, uint32_t vaddr, uint32_t error_code);

void vm_protect_readonly(struct vm_map *map);

void vm_init(void);

#endif
