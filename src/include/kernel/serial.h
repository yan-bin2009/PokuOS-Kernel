#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_write(const char *s);
void serial_write_char(char c);
void serial_write_hex(uint32_t num);

#endif
