#ifndef _KERNEL_TIER_H
#define _KERNEL_TIER_H

#include <stdint.h>

typedef enum {
        TIER_KERNEL,
        TIER_SYSTEM,
        TIER_USER,
        TIER_CRITICAL
} tier_t;

#define TIER_MAX 3

#define FREE_PAGES_CRITICAL 64
#define FREE_PAGES_WARN     128

extern int pressure_triggered;

uint32_t free_pages_count(void);
void vm_handle_pressure(void);
void vm_release_pressure(void);

static inline int tier_is_user(tier_t tier)
{
        return tier == TIER_USER || tier == TIER_CRITICAL;
}

static inline int tier_is_critical(tier_t tier)
{
        return tier == TIER_CRITICAL;
}

static inline int tier_to_queue_idx(tier_t tier)
{
        switch (tier) {
        case TIER_KERNEL:   return 0;
        case TIER_SYSTEM:   return 1;
        case TIER_CRITICAL: return 1;
        case TIER_USER:     return 2;
        }
        return 2;
}

#endif