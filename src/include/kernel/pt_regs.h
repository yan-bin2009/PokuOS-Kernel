#ifndef _KERNEL_PT_REGS_H
#define _KERNEL_PT_REGS_H

#include <stdint.h>

/* 与 syscall_entry.asm 的 pusha 压栈顺序一致：offset 0 是 EDI */
struct pt_regs
{
        uint32_t edi;
        uint32_t esi;
        uint32_t ebp;
        uint32_t esp;
        uint32_t ebx;
        uint32_t edx;
        uint32_t ecx;
        uint32_t eax;
};

#endif
