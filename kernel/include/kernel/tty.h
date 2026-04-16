#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_setcolor(uint8_t color);
void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

void terminal_move_cursor_left(void);
void terminal_move_cursor_right(void);
void terminal_move_cursor_up(void);
void terminal_move_cursor_down(void);
void terminal_move_cursor_home(void);
void terminal_move_cursor_end(void);
void terminal_delete_at_cursor(void);

size_t terminal_getrow(void);
size_t terminal_getcolumn(void);
void terminal_setcursor(size_t row, size_t column);
void terminal_clear_line(size_t row);

void terminal_draw_capslock_indicator(void);
uint8_t terminal_getcolor(void);

#endif
