#ifndef _KERNEL_TRAP_H
#define _KERNEL_TRAP_H

#include <stdint.h>

struct trap_frame
{
        uint32_t eip;
        uint32_t cs;
        uint32_t eflags;
        uint32_t esp;
        uint32_t ss;
};

void exception_trap(void *frame, uint32_t error_code, uint32_t vector);

extern void (*exc_handlers[32])(void);

#define TRAP_DEFINE_NE(name) \
        void __attribute__((interrupt)) name(void *frame)
#define TRAP_DEFINE_EC(name) \
        void __attribute__((interrupt)) name(void *frame, uint32_t error_code)

TRAP_DEFINE_NE(exc_vector00);
TRAP_DEFINE_NE(exc_vector01);
TRAP_DEFINE_NE(exc_vector02);
TRAP_DEFINE_NE(exc_vector03);
TRAP_DEFINE_NE(exc_vector04);
TRAP_DEFINE_NE(exc_vector05);
TRAP_DEFINE_NE(exc_vector06);
TRAP_DEFINE_NE(exc_vector07);
TRAP_DEFINE_EC(exc_vector08);
TRAP_DEFINE_NE(exc_vector09);
TRAP_DEFINE_EC(exc_vector10);
TRAP_DEFINE_EC(exc_vector11);
TRAP_DEFINE_EC(exc_vector12);
TRAP_DEFINE_EC(exc_vector13);
TRAP_DEFINE_EC(exc_vector14);
TRAP_DEFINE_NE(exc_vector15);
TRAP_DEFINE_NE(exc_vector16);
TRAP_DEFINE_EC(exc_vector17);
TRAP_DEFINE_NE(exc_vector18);
TRAP_DEFINE_NE(exc_vector19);
TRAP_DEFINE_NE(exc_vector20);
TRAP_DEFINE_NE(exc_vector21);
TRAP_DEFINE_NE(exc_vector22);
TRAP_DEFINE_NE(exc_vector23);
TRAP_DEFINE_NE(exc_vector24);
TRAP_DEFINE_NE(exc_vector25);
TRAP_DEFINE_NE(exc_vector26);
TRAP_DEFINE_NE(exc_vector27);
TRAP_DEFINE_NE(exc_vector28);
TRAP_DEFINE_NE(exc_vector29);
TRAP_DEFINE_NE(exc_vector30);
TRAP_DEFINE_NE(exc_vector31);

#endif
