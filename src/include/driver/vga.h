#ifndef _DRIVER_VGA_H
#define _DRIVER_VGA_H

void vga_putchar(char c);
void vga_write(const char *s);
void vga_clear(void);
void vga_init(void);

#endif
