#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include <io.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

static void terminal_clear_row(size_t row);
static void terminal_scroll(void);
static void terminal_newline(void);
static void terminal_backspace(void);
static void terminal_update_cursor(void);
void terminal_move_cursor_left(void);
void terminal_move_cursor_right(void);
void terminal_move_cursor_up(void);
void terminal_move_cursor_down(void);
void terminal_move_cursor_home(void);
void terminal_move_cursor_end(void);
void terminal_delete_at_cursor(void);


void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}

	terminal_update_cursor();
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

static void terminal_clear_row(size_t row) {
	for (size_t x = 0; x < VGA_WIDTH; x++) {
		terminal_putentryat(' ', terminal_color, x, row);
	}
}

static void terminal_scroll(void) {
	for (size_t y = 1; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			terminal_buffer[(y - 1) * VGA_WIDTH + x] =
				terminal_buffer[y * VGA_WIDTH + x];
		}
	}

	terminal_clear_row(VGA_HEIGHT - 1);
}

static void terminal_newline(void) {
	terminal_column = 0;
	terminal_row++;

	if (terminal_row >= VGA_HEIGHT) {
		terminal_scroll();
		terminal_row = VGA_HEIGHT - 1;
	}
}

static void terminal_backspace(void) {
	if (terminal_column > 0) {
		terminal_column--;
		terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
		return;
	}

	if (terminal_row > 0) {
		terminal_row--;
		terminal_column = VGA_WIDTH - 1;

		while (terminal_column > 0) {
			uint16_t entry = terminal_buffer[terminal_row * VGA_WIDTH + terminal_column];
			unsigned char ch = (unsigned char)(entry & 0xFF);

			if (ch != ' ')
				break;

			terminal_column--;
		}

		{
			uint16_t entry = terminal_buffer[terminal_row * VGA_WIDTH + terminal_column];
			unsigned char ch = (unsigned char)(entry & 0xFF);

			if (ch == ' ' && terminal_column == 0) {
				terminal_putentryat(' ', terminal_color, 0, terminal_row);
			} else {
				terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
			}
		}
	}
}

static void terminal_update_cursor(void) {
	size_t pos = terminal_row * VGA_WIDTH + terminal_column;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));

	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void terminal_putchar(char c) {
	switch (c) {
	case '\n':
		terminal_newline();
		terminal_update_cursor();
		return;

	case '\r':
		terminal_column = 0;
		terminal_update_cursor();
		return;

	case '\t':
		for (int i = 0; i < 4; i++) {
			terminal_putchar(' ');
		}
		return;

	case '\b':
		terminal_backspace();
		terminal_update_cursor();
		return;

	default:
		terminal_putentryat((unsigned char)c, terminal_color,
			terminal_column, terminal_row);
		terminal_column++;
		break;
	}

	if (terminal_column >= VGA_WIDTH) {
		terminal_newline();
	}

	terminal_update_cursor();
}

void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++) {
		terminal_putchar(data[i]);
	}
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

void terminal_move_cursor_left(void) {
	if (terminal_column > 0) {
		terminal_column--;
	} else if (terminal_row > 0) {
		terminal_row--;
		terminal_column = VGA_WIDTH - 1;
	}
	terminal_update_cursor();
}

void terminal_move_cursor_right(void) {
	if (terminal_column + 1 < VGA_WIDTH) {
		terminal_column++;
	} else if (terminal_row + 1 < VGA_HEIGHT) {
		terminal_row++;
		terminal_column = 0;
	}
	terminal_update_cursor();
}

void terminal_move_cursor_up(void) {
	if (terminal_row > 0) {
		terminal_row--;
	}
	terminal_update_cursor();
}

void terminal_move_cursor_down(void) {
	if (terminal_row + 1 < VGA_HEIGHT) {
		terminal_row++;
	}
	terminal_update_cursor();
}

void terminal_move_cursor_home(void) {
	terminal_column = 0;
	terminal_update_cursor();
}

void terminal_move_cursor_end(void) {
	size_t col = VGA_WIDTH;

	while (col > 0) {
		uint16_t entry = terminal_buffer[terminal_row * VGA_WIDTH + (col - 1)];
		unsigned char ch = (unsigned char)(entry & 0xFF);

		if (ch != ' ')
			break;

		col--;
	}

	terminal_column = col;
	if (terminal_column >= VGA_WIDTH)
		terminal_column = VGA_WIDTH - 1;

	terminal_update_cursor();
}

void terminal_delete_at_cursor(void) {
	for (size_t x = terminal_column; x + 1 < VGA_WIDTH; x++) {
		terminal_buffer[terminal_row * VGA_WIDTH + x] =
			terminal_buffer[terminal_row * VGA_WIDTH + (x + 1)];
	}

	terminal_putentryat(' ', terminal_color, VGA_WIDTH - 1, terminal_row);
	terminal_update_cursor();
}
