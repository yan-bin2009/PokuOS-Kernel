#include <driver/vga.h>

#define VGA_ADDR 0xB8000
#define COLS 80
#define ROWS 25
#define VGA_SIZE (COLS * ROWS)

volatile unsigned short* vga_buffer = (volatile unsigned short*)VGA_ADDR;
static int cursor_x = 0;
static int cursor_y = 0;

static void scroll() {
        for (int i = 0; i < (ROWS - 1) * COLS; i++) {
                vga_buffer[i] = vga_buffer[i + COLS];
        }
        for (int i = (ROWS - 1) * COLS; i < ROWS * COLS; i++) {
                vga_buffer[i] = (0x0F << 8) | ' ';
        }
        cursor_y = ROWS - 1;
}

void vga_clear() {
        for (int i = 0; i < VGA_SIZE; i++) {
                vga_buffer[i] = (0x0F << 8) | ' ';
        }
        cursor_x = 0;
        cursor_y = 0;
}

void vga_putchar(char c) {
        if (c == '\n') {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= ROWS) scroll();
                return;
        }
        if (c == '\b') {
                if (cursor_x > 0) {
                        cursor_x--;
                        int index = cursor_y * COLS + cursor_x;
                        vga_buffer[index] = (0x0F << 8) | ' ';
                }
                return;
        }
        int index = cursor_y * COLS + cursor_x;
        vga_buffer[index] = (0x0F << 8) | c;
        cursor_x++;
        if (cursor_x >= COLS) {
                cursor_x = 0;
                cursor_y++;
                if (cursor_y >= ROWS) scroll();
        }
}

void vga_write(const char* str) {
        for (int i = 0; str[i] != '\0'; i++) {
                vga_putchar(str[i]);
        }
}

void vga_init() {
        //vga_clear();    这个没啥用啊，但是保守一点，先保留
}
