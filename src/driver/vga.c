#include <driver/vga.h>
#include <stdint.h>
#include <kernel/ports.h>

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;

static void vga_update_cursor(void)
{
        uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

        outb(0x3D4, 0x0F);
        outb(0x3D5, (uint8_t)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_scroll(void)
{
        int y, x;

        for (y = 1; y < VGA_HEIGHT; y++) {
                for (x = 0; x < VGA_WIDTH; x++) {
                        vga_buffer[(y - 1) * VGA_WIDTH + x] =
                                vga_buffer[y * VGA_WIDTH + x];
                }
        }
        for (x = 0; x < VGA_WIDTH; x++)
                vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 0x0F00 | ' ';
}

void vga_putchar(char c)
{
        if (c == '\n') {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= VGA_HEIGHT) {
                        vga_scroll();
                        cursor_y = VGA_HEIGHT - 1;
                }
                vga_update_cursor();
                return;
        }

        if (c == '\r') {
                cursor_x = 0;
                vga_update_cursor();
                return;
        }

        if (c == '\b') {
                if (cursor_x > 0) {
                        cursor_x--;
                } else if (cursor_y > 0) {
                        cursor_y--;
                        cursor_x = VGA_WIDTH - 1;
                }
                vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | ' ';
                vga_update_cursor();
                return;
        }

        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | c;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= VGA_HEIGHT) {
                        vga_scroll();
                        cursor_y = VGA_HEIGHT - 1;
                }
        }
        vga_update_cursor();
}

void vga_write(const char *s)
{
        while (*s)
                vga_putchar(*s++);
}

void vga_clear(void)
{
        int i;

        for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
                vga_buffer[i] = 0x0F00 | ' ';
        cursor_x = 0;
        cursor_y = 0;
        vga_update_cursor();
}

void vga_init(void)
{
        vga_clear();
}
