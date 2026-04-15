#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

void terminal_move_cursor_left(void);
void terminal_move_cursor_right(void);
void terminal_move_cursor_up(void);
void terminal_move_cursor_down(void);
void terminal_move_cursor_home(void);
void terminal_move_cursor_end(void);
void terminal_delete_at_cursor(void);

#endif
