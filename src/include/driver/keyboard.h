#ifndef KEYBOARD_H
#define KEYBOARD_H

/* 扩展功能键码（与 ASCII 控制字符区错开，避免冲突） */
#define KEY_UP    1
#define KEY_DOWN  2
#define KEY_LEFT  3
#define KEY_RIGHT 4
#define KEY_HOME  0x10
#define KEY_END   0x11
#define KEY_PGUP  0x12
#define KEY_PGDN  0x13
#define KEY_INS   0x14
#define KEY_DEL   0x15

void keyboard_init(void);
char getchar(void);

#endif
