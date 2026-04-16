#include <stddef.h>
#include <stdint.h>

#include <keyboard.h>
#include <kernel/tty.h>
#include <line_editor.h>

#define LINE_EDITOR_MAX_SCREEN_COLS 80
#define LINE_EDITOR_HISTORY_SIZE 8
#define LINE_EDITOR_HISTORY_MAX 128

static char history[LINE_EDITOR_HISTORY_SIZE][LINE_EDITOR_HISTORY_MAX];
static size_t history_count = 0;   /* how many valid entries exist */
static size_t history_start = 0;   /* oldest entry index in ring */

static size_t min_size(size_t a, size_t b)
{
	return (a < b) ? a : b;
}

static size_t str_len_local(const char *s)
{
	size_t len = 0;
	while (s[len] != '\0')
		len++;
	return len;
}

static void str_copy_local(char *dst, const char *src, size_t max_len)
{
	size_t i;

	if (max_len == 0)
		return;

	for (i = 0; i + 1 < max_len && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}

	dst[i] = '\0';
}

static int str_equal_local(const char *a, const char *b)
{
	size_t i = 0;

	while (a[i] != '\0' && b[i] != '\0') {
		if (a[i] != b[i])
			return 0;
		i++;
	}

	return a[i] == b[i];
}

static void history_store(const char *line)
{
	size_t slot;

	/* Don't store empty commands */
	if (line[0] == '\0')
		return;

	/* Avoid storing exact duplicate of most recent entry */
	if (history_count > 0) {
		size_t newest = (history_start + history_count - 1) % LINE_EDITOR_HISTORY_SIZE;
		if (str_equal_local(history[newest], line))
			return;
	}

	if (history_count < LINE_EDITOR_HISTORY_SIZE) {
		slot = (history_start + history_count) % LINE_EDITOR_HISTORY_SIZE;
		history_count++;
	} else {
		/* overwrite oldest and advance start */
		slot = history_start;
		history_start = (history_start + 1) % LINE_EDITOR_HISTORY_SIZE;
	}

	str_copy_local(history[slot], line, LINE_EDITOR_HISTORY_MAX);
}

static const char *history_get(size_t logical_index)
{
	/* logical_index: 0 = oldest, history_count - 1 = newest */
	size_t slot = (history_start + logical_index) % LINE_EDITOR_HISTORY_SIZE;
	return history[slot];
}

static void load_line_from_source(
	char *out_buf,
	size_t max_len,
	size_t row,
	size_t start_col,
	const char *src,
	size_t *len,
	size_t *cursor)
{
	size_t i;
	size_t max_copy;

	if (max_len == 0)
		return;

	/* keep room for '\0' and fit on visible line */
	max_copy = max_len - 1;
	if (start_col < LINE_EDITOR_MAX_SCREEN_COLS) {
		size_t visible = LINE_EDITOR_MAX_SCREEN_COLS - start_col;
		max_copy = min_size(max_copy, visible);
	} else {
		max_copy = 0;
	}

	for (i = 0; i < max_copy && src[i] != '\0'; i++) {
		out_buf[i] = src[i];
	}
	out_buf[i] = '\0';

	*len = i;
	*cursor = i;

	/* redraw below */
	{
		uint8_t color = terminal_getcolor();

		for (size_t x = start_col; x < LINE_EDITOR_MAX_SCREEN_COLS; x++) {
			terminal_putentryat(' ', color, x, row);
		}

		for (size_t j = 0; j < *len; j++) {
			terminal_putentryat((unsigned char)out_buf[j], color, start_col + j, row);
		}

		terminal_setcursor(row, start_col + *cursor);
	}
}

static void line_editor_redraw(
	size_t row,
	size_t start_col,
	const char *buf,
	size_t len,
	size_t cursor_pos)
{
	uint8_t color = terminal_getcolor();

	for (size_t x = start_col; x < LINE_EDITOR_MAX_SCREEN_COLS; x++) {
		terminal_putentryat(' ', color, x, row);
	}

	for (size_t i = 0; i < len && (start_col + i) < LINE_EDITOR_MAX_SCREEN_COLS; i++) {
		terminal_putentryat((unsigned char)buf[i], color, start_col + i, row);
	}

	terminal_setcursor(row, start_col + cursor_pos);
}

void line_editor_init(void)
{
	for (size_t i = 0; i < LINE_EDITOR_HISTORY_SIZE; i++) {
		history[i][0] = '\0';
	}

	history_count = 0;
	history_start = 0;
}

void line_editor_readline(char *out_buf, size_t max_len)
{
	size_t row;
	size_t start_col;
	size_t len = 0;
	size_t cursor = 0;

	/*
	 * history_view:
	 *   -1  => editing current scratch line
	 *   0..history_count-1 => viewing history entry by logical index
	 */
	int history_view = -1;
	char scratch[LINE_EDITOR_HISTORY_MAX];

	if (out_buf == NULL || max_len == 0)
		return;

	row = terminal_getrow();
	start_col = terminal_getcolumn();

	out_buf[0] = '\0';
	scratch[0] = '\0';

	for (;;) {
		int key = keyboard_getkey();

		if (key == '\n') {
			out_buf[len] = '\0';
			history_store(out_buf);
			terminal_putchar('\n');
			return;
		}

		if (key == '\b') {
			if (history_view != -1) {
				/* copy history entry into editable line first */
				load_line_from_source(out_buf, max_len, row, start_col,
					history_get((size_t)history_view), &len, &cursor);
				history_view = -1;
			}

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

		if (key == KEY_DELETE) {
			if (history_view != -1) {
				load_line_from_source(out_buf, max_len, row, start_col,
					history_get((size_t)history_view), &len, &cursor);
				history_view = -1;
			}

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

		if (key == KEY_UP) {
			if (history_count == 0)
				continue;

			if (history_view == -1) {
				/* save current unfinished line before entering history */
				str_copy_local(scratch, out_buf, sizeof(scratch));
				history_view = (int)history_count - 1; /* newest */
			} else if (history_view > 0) {
				history_view--;
			}

			load_line_from_source(out_buf, max_len, row, start_col,
				history_get((size_t)history_view), &len, &cursor);
			continue;
		}

		if (key == KEY_DOWN) {
			if (history_count == 0)
				continue;

			if (history_view == -1) {
				continue;
			} else if (history_view < (int)history_count - 1) {
				history_view++;
				load_line_from_source(out_buf, max_len, row, start_col,
					history_get((size_t)history_view), &len, &cursor);
			} else {
				/* past newest => restore scratch line */
				history_view = -1;
				load_line_from_source(out_buf, max_len, row, start_col,
					scratch, &len, &cursor);
			}
			continue;
		}

		if (key >= 256) {
			continue;
		}

		if (key < 32 || key > 126) {
			continue;
		}

		if (history_view != -1) {
			/*
			 * Start editing from the shown history entry.
			 * Copy it into the live buffer first.
			 */
			load_line_from_source(out_buf, max_len, row, start_col,
				history_get((size_t)history_view), &len, &cursor);
			history_view = -1;
		}

		if (len + 1 < max_len && start_col + len < LINE_EDITOR_MAX_SCREEN_COLS) {
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
