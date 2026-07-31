#include <kernel/ports.h>
#include <stdint.h>

static int serial_transmit_empty(void)
{
        return inb(0x3F8 + 5) & 0x20;
}

void serial_write_char(char c)
{
        while (!serial_transmit_empty());
        outb(0x3F8, c);
}

void serial_write(const char *s)
{
        while (*s) {
                if (*s == '\n')
                        serial_write_char('\r');
                serial_write_char(*s++);
        }
}

void serial_init(void)
{
        outb(0x3F8 + 1, 0x00);
        outb(0x3F8 + 3, 0x80);
        outb(0x3F8 + 0, 0x03);
        outb(0x3F8 + 1, 0x00);
        outb(0x3F8 + 3, 0x03);
        outb(0x3F8 + 2, 0xC7);
        outb(0x3F8 + 4, 0x0B);
}

void serial_write_hex(uint32_t num)
{
        char hex[] = "0123456789ABCDEF";
        char buf[9];
        int i;

        for (i = 7; i >= 0; i--) {
                buf[i] = hex[num & 0xF];
                num >>= 4;
        }
        buf[8] = '\0';
        serial_write(buf);
}
