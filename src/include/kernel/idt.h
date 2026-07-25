#ifndef IDT_H
#define IDT_H

#define IRQ1_VECTOR     33
#define SYSCALL_VECTOR  0x80

#define GATE_INTERRUPT  0x8E
#define GATE_TRAP       0x8F
#define GATE_USER       0xEE   // 中断门 DPL=3

void idt_init(void);

#endif
