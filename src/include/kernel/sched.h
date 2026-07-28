#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <kernel/task.h>

void sched_init(void);

void schedule(void);

void pit_handler(void *frame);

void pit_init(void);

#endif
