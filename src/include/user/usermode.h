#ifndef _USER_USERMODE_H
#define _USER_USERMODE_H

#include <stdint.h>

void switch_to_user(uint32_t entry, uint32_t user_stack);

#endif
