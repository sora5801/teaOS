#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H

#include <stddef.h>

void line_editor_init(void);

/*
 * Reads one editable line from keyboard input.
 * The returned string is null-terminated in out_buf.
 * max_len includes room for the null terminator.
 */
void line_editor_readline(char *out_buf, size_t max_len);

#endif
