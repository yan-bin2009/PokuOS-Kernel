#include <driver/keybord.h>
#include <kernel/ports.h>

#define VGA_ADDR 0xB8000
#define COLS 80
#define ROWS 25
#define BUFFER_SIZE 256

volatile unsigned short *vga = (volatile unsigned short*)VGA_ADDR;
int cursor = 0;

static volatile char key_buffer[BUFFER_SIZE];
static volatile int head = 0;
static volatile int tail = 0;

static int key_down[128] = {0};
static int is_break = 0;

static unsigned char scancode_to_ascii[128] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,
        9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static void putchar(char c) {
        if (c == '\b') {
                if (cursor > 0) {
                        cursor--;
                        vga[cursor] = (0x0F << 8) | ' ';
                }
                return;
        }
        if (c == '\n') {
                int row = cursor / COLS;
                cursor = (row + 1) * COLS;
                if (cursor >= ROWS * COLS) cursor = 0;
                return;
        }
        if (cursor >= ROWS * COLS) cursor = 0;
        vga[cursor++] = (0x0F << 8) | c;
}

static void push_to_buffer(char c) {
        int next = (head + 1) % BUFFER_SIZE;
        if (next != tail) {
                key_buffer[head] = c;
                head = next;
        }
}

char getchar() {
        if (tail == head) return 0;
        char c = key_buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        return c;
}

void __attribute__((interrupt)) keybord_handler(void* frame) {
        unsigned char scancode = inb(0x60);

        if (scancode == 0xF0) {
                is_break = 1;
                outb(0xA0, 0x20);
                outb(0x20, 0x20);
                return;
        }

        if (is_break) {
                if (scancode < 128) key_down[scancode] = 0;
                is_break = 0;
                outb(0xA0, 0x20);
                outb(0x20, 0x20);
                return;
        }

        if (scancode >= 128) {
                outb(0xA0, 0x20);
                outb(0x20, 0x20);
                return;
        }

        key_down[scancode] = 1;

        if (scancode == 0x2A || scancode == 0x36) {
                outb(0xA0, 0x20);
                outb(0x20, 0x20);
                return;
        }

        char c = scancode_to_ascii[scancode];
        if (c) {
                if (key_down[0x2A] || key_down[0x36]) {
                        if (c >= 'a' && c <= 'z') c -= 32;
                        else if (c >= '1' && c <= '9') {
                                char shift_map[] = ")!@#$%^&*(";
                                if (c >= '1' && c <= '9') c = shift_map[c - '1'];
                        }
                }
                push_to_buffer(c);
        }

        outb(0xA0, 0x20);
        outb(0x20, 0x20);
}

void keybord_init() {
        outb(0x20, 0x11);
        outb(0xA0, 0x11);
        outb(0x21, 0x20);
        outb(0xA1, 0x28);
        outb(0x21, 0x04);
        outb(0xA1, 0x02);
        outb(0x21, 0x01);
        outb(0xA1, 0x01);

        outb(0x21, 0xFD);
        outb(0xA1, 0xFF);
}
