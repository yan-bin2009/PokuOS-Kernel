#include "keymap.hpp"
#include "syscall.hpp"

KeyEvent get_key_event()
{
        char c = sys_getchar();
        KeyEvent ev;

        ev.type = KEYEV_NONE;
        ev.ch = 0;

        if (!c)
                return ev;

        switch (c)
        {
        case 0x1B:
                ev.type = KEYEV_ESC;
                break;
        case '\n':
                ev.type = KEYEV_ENTER;
                break;
        case '\b':
                ev.type = KEYEV_BACKSPACE;
                break;
        case KEY_UP:
                ev.type = KEYEV_UP;
                break;
        case KEY_DOWN:
                ev.type = KEYEV_DOWN;
                break;
        case KEY_LEFT:
                ev.type = KEYEV_LEFT;
                break;
        case KEY_RIGHT:
                ev.type = KEYEV_RIGHT;
                break;
        default:
                ev.type = KEYEV_CHAR;
                ev.ch = c;
                break;
        }

        return ev;
}
