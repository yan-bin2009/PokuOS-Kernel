#include <user/tss.h>
#include <driver/vga.h>

static struct tss_entry tss;

extern unsigned char gdt_tss[];

void tss_init(void)
{
        uint32_t *ptr;
        uint32_t base;
        int i;

        ptr = (uint32_t *)&tss;
        for (i = 0; i < sizeof(tss) / 4; i++)
                ptr[i] = 0;

        base = (uint32_t)&tss;
        gdt_tss[2] = base & 0xFF;
        gdt_tss[3] = (base >> 8) & 0xFF;
        gdt_tss[4] = (base >> 16) & 0xFF;
        gdt_tss[7] = (base >> 24) & 0xFF;

        tss.iomap_base = sizeof(tss);
        tss.ss0 = 0x10;

        __asm__ volatile ("ltr %%ax" : : "a" (0x28));
}

void tss_set_kernel_stack(uint32_t esp0)
{
        tss.esp0 = esp0;
}
