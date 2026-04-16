#include <stddef.h>
#include <stdint.h>

#include <keyboard.h>
#include <kernel/tty.h>
#include <line_editor.h>

static void line_editor_redraw(
	size_t row,
	size_t start_col,
	const char *buf,
	size_t len,
	size_t cursor_pos)
{
	/* Clear from start_col to end of line */
	for (size_t x = start_col; x < 80; x++) {
		terminal_putentryat(' ', 0x07, x, row);
	}

	/* Draw buffer */
	for (size_t i = 0; i < len && (start_col + i) < 80; i++) {
		terminal_putentryat((unsigned char)buf[i], 0x07, start_col + i, row);
	}

	/* Put cursor at logical edit position */
	terminal_setcursor(row, start_col + cursor_pos);
}

void line_editor_init(void)
{
	/* Nothing needed yet */
}

void line_editor_readline(char *out_buf, size_t max_len)
{
	size_t row;
	size_t start_col;

	size_t len = 0;
	size_t cursor = 0;

	if (max_len == 0)
		return;

	row = terminal_getrow();
	start_col = terminal_getcolumn();

	out_buf[0] = '\0';

	for (;;) {
		int key = keyboard_getkey();

		/* Enter: finish line */
		if (key == '\n') {
			out_buf[len] = '\0';
			terminal_putchar('\n');
			return;
		}

		/* Backspace: delete left of cursor */
		if (key == '\b') {
			if (cursor > 0) {
				for (size_t i = cursor - 1; i < len - 1; i++) {
					out_buf[i] = out_buf[i + 1];
				}
				len--;
				cursor--;
				out_buf[len] = '\0';
				line_editor_redraw(row, start_col, out_buf, len, cursor);
			}
			continue;
		}

		/* Delete: delete at cursor */
		if (key == KEY_DELETE) {
			if (cursor < len) {
				for (size_t i = cursor; i < len - 1; i++) {
					out_buf[i] = out_buf[i + 1];
				}
				len--;
				out_buf[len] = '\0';
				line_editor_redraw(row, start_col, out_buf, len, cursor);
			}
			continue;
		}

		/* Cursor movement */
		if (key == KEY_LEFT) {
			if (cursor > 0) {
				cursor--;
				terminal_setcursor(row, start_col + cursor);
			}
			continue;
		}

		if (key == KEY_RIGHT) {
			if (cursor < len) {
				cursor++;
				terminal_setcursor(row, start_col + cursor);
			}
			continue;
		}

		if (key == KEY_HOME) {
			cursor = 0;
			terminal_setcursor(row, start_col);
			continue;
		}

		if (key == KEY_END) {
			cursor = len;
			terminal_setcursor(row, start_col + cursor);
			continue;
		}

		/* Ignore unsupported extended keys for now */
		if (key >= 256) {
			continue;
		}

		/* Ignore non-printable controls except tab if you want it */
		if (key < 32 || key > 126) {
			continue;
		}

		/* Insert printable character at cursor */
		if (len + 1 < max_len && start_col + len < 80) {
			for (size_t i = len; i > cursor; i--) {
				out_buf[i] = out_buf[i - 1];
			}

			out_buf[cursor] = (char)key;
			len++;
			cursor++;
			out_buf[len] = '\0';

			line_editor_redraw(row, start_col, out_buf, len, cursor);
		}
	}
}
