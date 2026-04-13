#include <stddef.h>
#include <stdint.h>
#include <keyboard.h>

#if defined(__linux__)
#error "This code must be compiled with a cross-compiler"
#elif !defined(__i386__)
#error "This code must be compiled with an x86-elf compiler"
#endif

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const int VGA_COLS = 80;
const int VGA_ROWS = 25;

int term_col = 0;
int term_row = 0;
uint8_t term_color = 0x0F;

void term_init(void) {
    for (int col = 0; col < VGA_COLS; col++) {
        for (int row = 0; row < VGA_ROWS; row++) {
            const size_t index = (VGA_COLS * row) + col;
            vga_buffer[index] = ((uint16_t)term_color << 8) | ' ';
        }
    }
    term_col = 0;
    term_row = 0;
}

void term_putc(char c) {
    switch (c) {
        case '\n':
            term_col = 0;
            term_row++;
            break;

        case '\b':
            if (term_col > 0) {
                term_col--;
                const size_t index = (VGA_COLS * term_row) + term_col;
                vga_buffer[index] = ((uint16_t)term_color << 8) | ' ';
            }
            break;

        default: {
            const size_t index = (VGA_COLS * term_row) + term_col;
            vga_buffer[index] = ((uint16_t)term_color << 8) | c;
            term_col++;
            break;
        }
    }

    if (term_col >= VGA_COLS) {
        term_col = 0;
        term_row++;
    }

    if (term_row >= VGA_ROWS) {
        term_row = 0;
    }
}

void term_print(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        term_putc(str[i]);
    }
}

void kernel_main(void) {
    term_init();
    term_print("Keyboard polling demo\n");
    term_print("Type on the keyboard:\n");

    for (;;) {
        char c = keyboard_getchar();

        if (c != 0) {
            term_putc(c);
        }
    }
}
