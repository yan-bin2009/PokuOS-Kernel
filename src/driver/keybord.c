#include <driver/keybord.h>
#include <kernel/ports.h>

#define BUFFER_SIZE 256
#define KEY_RELEASE 0xF0

static volatile char key_buffer[BUFFER_SIZE];
static volatile int head = 0;
static volatile int tail = 0;
static volatile int key_state[128] = {0};

static const unsigned char scancode_to_ascii[128] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        '-', '=', 8, 9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g',
        'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',
        'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char shift_symbols[] = ")!@#$%^&*(";

static void push_to_buffer(char c)
{
        int next = (head + 1) % BUFFER_SIZE;

        if (next != tail) {
                key_buffer[head] = c;
                head = next;
        }
}

char getchar(void)
{
        char c;

        if (tail == head)
                return 0;
        c = key_buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        return c;
}

void keybord_handler(void *frame)
{
        unsigned char scancode = inb(0x60);
        static int extended = 0;
        static int break_pending = 0;

        if (scancode == 0xE0) {
                extended = 1;
                outb(0x20, 0x20);
                return;
        }

        if (extended) {
                extended = 0;
                if (scancode == KEY_RELEASE) {
                        break_pending = 1;
                        outb(0x20, 0x20);
                        return;
                }
                if (break_pending) {
                        break_pending = 0;
                        outb(0x20, 0x20);
                        return;
                }
                switch (scancode) {
                case 0x48: push_to_buffer(KEY_UP); break;
                case 0x4B: push_to_buffer(KEY_LEFT); break;
                case 0x4D: push_to_buffer(KEY_RIGHT); break;
                case 0x50: push_to_buffer(KEY_DOWN); break;
                default: break;
                }
                outb(0x20, 0x20);
                return;
        }

        if (scancode == KEY_RELEASE) {
                break_pending = 1;
                outb(0x20, 0x20);
                return;
        }

        if (break_pending) {
                if (scancode < 128)
                        key_state[scancode] = 0;
                break_pending = 0;
                outb(0x20, 0x20);
                return;
        }

        if (scancode >= 128) {
                outb(0x20, 0x20);
                return;
        }

        key_state[scancode] = 1;

        if (scancode == 0x48 || scancode == 0x4B || scancode == 0x4D || scancode == 0x50) {
                switch (scancode) {
                case 0x48: push_to_buffer(KEY_UP); break;
                case 0x4B: push_to_buffer(KEY_LEFT); break;
                case 0x4D: push_to_buffer(KEY_RIGHT); break;
                case 0x50: push_to_buffer(KEY_DOWN); break;
                }
                outb(0x20, 0x20);
                return;
        }

        if (scancode == 0x2A || scancode == 0x36) {
                outb(0x20, 0x20);
                return;
        }

        {
                char c = scancode_to_ascii[scancode];
                int shift = key_state[0x2A] || key_state[0x36];

                if (c) {
                        if (shift && c >= 'a' && c <= 'z') {
                                c -= 32;
                        } else if (shift && c >= '1' && c <= '9') {
                                c = shift_symbols[c - '1'];
                        } else if (shift && c == '0') {
                                c = ')';
                        }
                        push_to_buffer(c);
                }
        }

        outb(0x20, 0x20);
}

void keybord_init(void)
{
        outb(0x20, 0x11);
        outb(0xA0, 0x11);
        outb(0x21, 0x20);
        outb(0xA1, 0x28);
        outb(0x21, 0x04);
        outb(0xA1, 0x02);
        outb(0x21, 0x01);
        outb(0xA1, 0x01);

        outb(0x21, 0xFC);
        outb(0xA1, 0xFF);
}
