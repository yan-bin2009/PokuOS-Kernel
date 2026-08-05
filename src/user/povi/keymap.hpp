#ifndef POVI_KEYMAP_HPP
#define POVI_KEYMAP_HPP

enum KeyType
{
        KEYEV_NONE,
        KEYEV_CHAR,
        KEYEV_ESC,
        KEYEV_ENTER,
        KEYEV_BACKSPACE,
        KEYEV_UP,
        KEYEV_DOWN,
        KEYEV_LEFT,
        KEYEV_RIGHT
};

struct KeyEvent
{
        KeyType type;
        char ch;
};

/* 阻塞读取键盘，返回内部按键事件。 */
KeyEvent get_key_event();

#endif
