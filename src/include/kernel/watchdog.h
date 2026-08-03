#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <kernel/tier.h>

void watchdog_init(void);
void watchdog_tick(void);
int watchdog_register(const char *path, tier_t tier, task_t *initial_task);
void watchdog_check(void);

#endif
