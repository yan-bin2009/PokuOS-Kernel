#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H 
#include <stddef.h> 

/* initialize the kernel heap allocator
* Must be called after paging is enabled
*/
void heap_init(void);

/*
 * Allocate a block of memory of at least 'size' bytes
 * Returns a kernel virtual address, or NULL on failure.
 */ 
void *kmalloc(size_t size);

void kfree(void *ptr);

#endif
