#include <kernel/serial.h>
/*
 * 参考linux-0.99键盘工艺，感谢linus的馈赠！！！
 */

#include <driver/keyboard.h>
#include <kernel/ports.h>

#define BUFFER_SIZE 256

#define KG_SHIFT 0
#define NR_SHIFT 2

static volatile char key_buffer[BUFFER_SIZE];
static volatile int head = 0;
static volatile int tail = 0;

static unsigned long key_down[4] = {0, 0, 0, 0};
static int k_down[NR_SHIFT] = {0, 0};
static int shift_state = 0;
static int caps_lock = 0;
static int num_lock = 0;
static int rep = 0;
static unsigned char prev_scancode = 0;

static const unsigned char scancode_to_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', 8, 9, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g',
    'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x',
    'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const char shift_symbols[] = "!@#$%^&*(";

static void push_to_buffer(char c)
{
        int next = (head + 1) % BUFFER_SIZE;

        if (next != tail)
        {
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

static int test_bit(int nr, unsigned long *addr)
{
        return (addr[nr >> 5] >> (nr & 31)) & 1;
}

static void set_bit(int nr, unsigned long *addr)
{
        addr[nr >> 5] |= (1UL << (nr & 31));
}

static void clear_bit(int nr, unsigned long *addr)
{
        addr[nr >> 5] &= ~(1UL << (nr & 31));
}

static void do_shift(int value, int up_flag)
{
        if (rep)
                return;

        if (up_flag)
        {
                if (k_down[value])
                        k_down[value]--;
        }
        else
                k_down[value]++;

        if (k_down[value])
                shift_state |= (1 << value);
        else
                shift_state &= ~(1 << value);
}

static void caps_toggle(void)
{
        if (rep)
                return;
        caps_lock = !caps_lock;
}

static void push_arrow(int scancode)
{
        switch (scancode)
        {
        case 0x47:
                push_to_buffer(KEY_HOME);
                break;
        case 0x48:
                push_to_buffer(KEY_UP);
                break;
        case 0x49:
                push_to_buffer(KEY_PGUP);
                break;
        case 0x4B:
                push_to_buffer(KEY_LEFT);
                break;
        case 0x4D:
                push_to_buffer(KEY_RIGHT);
                break;
        case 0x4F:
                push_to_buffer(KEY_END);
                break;
        case 0x50:
                push_to_buffer(KEY_DOWN);
                break;
        case 0x51:
                push_to_buffer(KEY_PGDN);
                break;
        case 0x52:
                push_to_buffer(KEY_INS);
                break;
        case 0x53:
                push_to_buffer(KEY_DEL);
                break;
        default:
                break;
        }
}

/* 小键盘（NumLock 关闭时为导航键，开启时输入数字/符号） */
static void handle_keypad(int scancode)
{
        static const char kp_num[] = {'7', '8', '9', '-', '4', '5',
                                      '6', '+', '1', '2', '3', '0', '.'};

        if (scancode == 0x4A)
        {
                push_to_buffer('-');
                return;
        }
        if (scancode == 0x4E)
        {
                push_to_buffer('+');
                return;
        }
        if (scancode == 0x37)
        {
                push_to_buffer('*');
                return;
        }
        if (scancode >= 0x47 && scancode <= 0x53)
        {
                if (num_lock & 1)
                        push_to_buffer(kp_num[scancode - 0x47]);
                else
                        push_arrow(scancode);
        }
}

void keyboard_handler(void *frame)
{
        unsigned char scancode = inb(0x60);
        int up_flag;
        char c;

        if (scancode == 0xE0 || scancode == 0xE1)
        {
                prev_scancode = scancode;
                outb(0x20, 0x20);
                return;
        }
        up_flag = scancode & 0x80;
        scancode &= 0x7F;

        if (prev_scancode)
        {
                prev_scancode = 0;
                if (scancode == 0x2A || scancode == 0x36)
                {
                        outb(0x20, 0x20);
                        return;
                }
                if (!up_flag)
                {
                        switch (scancode)
                        {
                        case 0x1C: /* 小键盘 Enter */
                                push_to_buffer('\n');
                                break;
                        case 0x35: /* 小键盘 / */
                                push_to_buffer('/');
                                break;
                        default:
                                push_arrow(scancode);
                                break;
                        }
                }
                outb(0x20, 0x20);
                return;
        }

        if (up_flag)
        {
                clear_bit(scancode, key_down);
                rep = 0;
                if (scancode == 0x2A || scancode == 0x36)
                        do_shift(KG_SHIFT, up_flag);
                outb(0x20, 0x20);
                return;
        }

        rep = test_bit(scancode, key_down);
        set_bit(scancode, key_down);

        if (scancode == 0x2A || scancode == 0x36)
        {
                do_shift(KG_SHIFT, up_flag);
                outb(0x20, 0x20);
                return;
        }

        if (scancode == 0x3A)
        {
                if (!rep)
                        caps_toggle();
                outb(0x20, 0x20);
                return;
        }

        if (scancode == 0x45)
        {
                if (!rep)
                        num_lock = !num_lock;
                outb(0x20, 0x20);
                return;
        }

        /* 小键盘区：NumLock 决定数字或导航 */
        if (scancode == 0x37 || scancode == 0x4A ||
            (scancode >= 0x47 && scancode <= 0x53))
        {
                handle_keypad(scancode);
                outb(0x20, 0x20);
                return;
        }

        c = scancode_to_ascii[scancode];
        if (c)
        {
                if (c >= 'a' && c <= 'z')
                {
                        if ((shift_state ^ caps_lock) & 1)
                                c -= 32;
                }
                else if (shift_state & 1)
                {
                        if (c >= '1' && c <= '9')
                                c = shift_symbols[c - '1'];
                        else if (c == '0')
                                c = ')';
                        else if (c == '-')
                                c = '_';
                        else if (c == '=')
                                c = '+';
                        else if (c == '[')
                                c = '{';
                        else if (c == ']')
                                c = '}';
                        else if (c == ';')
                                c = ':';
                        else if (c == '\'')
                                c = '"';
                        else if (c == '`')
                                c = '~';
                        else if (c == '\\')
                                c = '|';
                        else if (c == ',')
                                c = '<';
                        else if (c == '.')
                                c = '>';
                        else if (c == '/')
                                c = '?';
                }
                push_to_buffer(c);
        }

        outb(0x20, 0x20);
}

void keyboard_init(void)
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
