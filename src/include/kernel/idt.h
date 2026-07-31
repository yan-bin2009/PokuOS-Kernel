#ifndef IDT_H
#define IDT_H

#define IRQ1_VECTOR    33
#define SYSCALL_VECTOR 0x80

#define GATE_INTERRUPT 0x8E
#define GATE_TRAP      0x8F
#define GATE_USER      0xEE

void idt_init(void);
void idt_set_gate(unsigned char num, unsigned long base,
                  unsigned short sel, unsigned char flags);

#endif
