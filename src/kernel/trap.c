#include <driver/vga.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/trap.h>
#include <stdint.h>

static const char *const exception_names[32] = {
    "Divide Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD FP Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

static void trap_out(const char *s)
{
        serial_write(s);
        vga_write(s);
}

static void trap_hex_out(uint32_t v)
{
        char buf[9];
        char *p = buf + 8;
        int i;

        *p = '\0';
        for (i = 0; i < 8; i++)
        {
                uint32_t nib = v & 0xF;

                *--p = nib < 10 ? '0' + nib : 'A' + nib - 10;
                v >>= 4;
        }
        trap_out(p);
}

static void trap_field(const char *name, uint32_t value)
{
        trap_out(name);
        trap_out("=0x");
        trap_hex_out(value);
        trap_out("\n");
}

static uint32_t trap_read_cr2(void)
{
        uint32_t cr2;

        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        return cr2;
}

static void trap_dump_stack(uint32_t esp, int is_user)
{
        uint32_t *sp = (uint32_t *)esp;
        uint32_t limit;
        uint32_t i;

        if (is_user)
        {
                limit = 0xC0000000;
        }
        else
        {
                uint32_t page_base = esp & ~0xFFF;

                if (page_base < 0xFF000000 || page_base > 0xFFFFF000)
                        page_base = 0xFF000000;
                limit = page_base + 0x4000;
                if (limit > 0xFFFFF000)
                        limit = 0xFFFFF000;
        }
        trap_out("\nstack dump (");
        trap_out(is_user ? "user" : "kernel");
        trap_out("):\n");
        for (i = 0; i < 16; i++)
        {
                uint32_t *addr;

                addr = sp + i;
                if ((uint32_t)addr >= limit)
                        break;
                trap_out("  [0x");
                trap_hex_out((uint32_t)addr);
                trap_out("] 0x");
                trap_hex_out(*addr);
                trap_out("\n");
        }
}

static void trap_error_code_decode(uint32_t error_code)
{
        uint32_t kind;

        trap_out("\nerror_code bits: ");
        if (error_code & 1)
        {
                trap_out("present ");
        }
        else
        {
                trap_out("not-present ");
        }
        if (error_code & 2)
        {
                trap_out("write ");
        }
        else
        {
                trap_out("read ");
        }
        if (error_code & 4)
        {
                trap_out("user-mode");
        }
        else
        {
                trap_out("kernel-mode");
        }
        trap_out("\n");
        kind = (error_code & 0x1F) >> 3;
        if (kind)
        {
                trap_out("  selector index=");
                trap_hex_out(kind);
                trap_out("\n");
        }
}

void exception_trap(void *frame, uint32_t error_code, uint32_t vector)
{
        struct trap_frame *tf = (struct trap_frame *)frame;
        int is_user = (tf->cs & 3) != 0;
        int is_pf = (vector == 14);

        trap_out("\n==============================\n");
        trap_out("KERNEL EXCEPTION: ");
        trap_out(is_user ? "[USER MODE] " : "[KERNEL MODE] ");
        if (vector < 32)
        {
                trap_out(exception_names[vector]);
        }
        else
        {
                trap_out("Unknown");
        }
        trap_out("\n");
        trap_field("vector", vector);
        trap_field("eip", tf->eip);
        trap_field("cs", tf->cs);
        trap_field("eflags", tf->eflags);
        if (is_user)
        {
                trap_field("user_esp", tf->esp);
                trap_field("user_ss", tf->ss);
        }
        trap_field("error_code", error_code);
        trap_field("cr2", trap_read_cr2());
        if (is_pf)
        {
                trap_error_code_decode(error_code);
        }
        if (is_user)
        {
                trap_dump_stack(tf->esp, 1);
        }
        else
        {
                uint32_t esp;

                __asm__ volatile("mov %%esp, %0" : "=r"(esp));
                trap_dump_stack(esp, 0);
        }
        trap_out("\ncurrent_task pid=");
        if (current_task)
        {
                trap_hex_out(current_task->pid);
        }
        else
        {
                trap_out("NULL");
        }
        trap_out("\n==============================\n");

        if (is_user)
        {
                trap_out("USER task killed, returning to scheduler\n");
                task_exit(vector);
        }
        while (1)
        {
                __asm__ volatile("hlt");
        }
}

#define TRAP_DEFINE_HANDLER_NE(name, vector)              \
        void __attribute__((interrupt)) name(void *frame) \
        {                                                 \
                exception_trap(frame, 0, vector);         \
        }

#define TRAP_DEFINE_HANDLER_EC(name, vector)                                   \
        void __attribute__((interrupt)) name(void *frame, uint32_t error_code) \
        {                                                                      \
                exception_trap(frame, error_code, vector);                     \
        }

TRAP_DEFINE_HANDLER_NE(exc_vector00, 0)
TRAP_DEFINE_HANDLER_NE(exc_vector01, 1)
TRAP_DEFINE_HANDLER_NE(exc_vector02, 2)
TRAP_DEFINE_HANDLER_NE(exc_vector03, 3)
TRAP_DEFINE_HANDLER_NE(exc_vector04, 4)
TRAP_DEFINE_HANDLER_NE(exc_vector05, 5)
TRAP_DEFINE_HANDLER_NE(exc_vector06, 6)
TRAP_DEFINE_HANDLER_NE(exc_vector07, 7)
TRAP_DEFINE_HANDLER_EC(exc_vector08, 8)
TRAP_DEFINE_HANDLER_NE(exc_vector09, 9)
TRAP_DEFINE_HANDLER_EC(exc_vector10, 10)
TRAP_DEFINE_HANDLER_EC(exc_vector11, 11)
TRAP_DEFINE_HANDLER_EC(exc_vector12, 12)
TRAP_DEFINE_HANDLER_EC(exc_vector13, 13)
TRAP_DEFINE_HANDLER_EC(exc_vector14, 14)
TRAP_DEFINE_HANDLER_NE(exc_vector15, 15)
TRAP_DEFINE_HANDLER_NE(exc_vector16, 16)
TRAP_DEFINE_HANDLER_EC(exc_vector17, 17)
TRAP_DEFINE_HANDLER_NE(exc_vector18, 18)
TRAP_DEFINE_HANDLER_NE(exc_vector19, 19)
TRAP_DEFINE_HANDLER_NE(exc_vector20, 20)
TRAP_DEFINE_HANDLER_NE(exc_vector21, 21)
TRAP_DEFINE_HANDLER_NE(exc_vector22, 22)
TRAP_DEFINE_HANDLER_NE(exc_vector23, 23)
TRAP_DEFINE_HANDLER_NE(exc_vector24, 24)
TRAP_DEFINE_HANDLER_NE(exc_vector25, 25)
TRAP_DEFINE_HANDLER_NE(exc_vector26, 26)
TRAP_DEFINE_HANDLER_NE(exc_vector27, 27)
TRAP_DEFINE_HANDLER_NE(exc_vector28, 28)
TRAP_DEFINE_HANDLER_NE(exc_vector29, 29)
TRAP_DEFINE_HANDLER_NE(exc_vector30, 30)
TRAP_DEFINE_HANDLER_NE(exc_vector31, 31)

void (*exc_handlers[32])(void) = {
    (void (*)(void))exc_vector00,
    (void (*)(void))exc_vector01,
    (void (*)(void))exc_vector02,
    (void (*)(void))exc_vector03,
    (void (*)(void))exc_vector04,
    (void (*)(void))exc_vector05,
    (void (*)(void))exc_vector06,
    (void (*)(void))exc_vector07,
    (void (*)(void))exc_vector08,
    (void (*)(void))exc_vector09,
    (void (*)(void))exc_vector10,
    (void (*)(void))exc_vector11,
    (void (*)(void))exc_vector12,
    (void (*)(void))exc_vector13,
    (void (*)(void))exc_vector14,
    (void (*)(void))exc_vector15,
    (void (*)(void))exc_vector16,
    (void (*)(void))exc_vector17,
    (void (*)(void))exc_vector18,
    (void (*)(void))exc_vector19,
    (void (*)(void))exc_vector20,
    (void (*)(void))exc_vector21,
    (void (*)(void))exc_vector22,
    (void (*)(void))exc_vector23,
    (void (*)(void))exc_vector24,
    (void (*)(void))exc_vector25,
    (void (*)(void))exc_vector26,
    (void (*)(void))exc_vector27,
    (void (*)(void))exc_vector28,
    (void (*)(void))exc_vector29,
    (void (*)(void))exc_vector30,
    (void (*)(void))exc_vector31,
};
