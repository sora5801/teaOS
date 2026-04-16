#include <stddef.h>
#include <stdint.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <kernel/tty.h>
#include <keyboard.h>
#include <paging.h>
#include <line_editor.h>

#if defined(__linux__)
#error "This code must be compiled with a cross-compiler"
#elif !defined(__i386__)
#error "This code must be compiled with an x86-elf compiler"
#endif


void kernel_main(void) {
    char line[128];

    terminal_initialize();
    gdt_init();
    idt_init();
    irq_init();
    paging_init();
    line_editor_init();

    terminal_writestring("GDT loaded.\n");
    terminal_writestring("IDT loaded.\n");
    terminal_writestring("IRQs enabled.\n");
    terminal_writestring("Paging enabled.\n");

    terminal_writestring("About to test page fault...\n");

   /* volatile uint32_t *ptr = (uint32_t *)0x400000; /* 4 MiB */ 
    /*uint32_t value = *ptr;

    (void)value;  */

    __asm__ volatile ("sti");

    for (;;) {
	terminal_writestring("> ");
	int key = keyboard_getkey();
	line_editor_readline(line, sizeof(line));

	terminal_writestring("You typed: ");
	terminal_writestring(line);
	terminal_putchar('\n');
	
        switch (key) {
        case KEY_LEFT:
            terminal_move_cursor_left();
            break;
        case KEY_RIGHT:
            terminal_move_cursor_right();
            break;
        case KEY_UP:
            terminal_move_cursor_up();
            break;
        case KEY_DOWN:
            terminal_move_cursor_down();
            break;
        case KEY_HOME:
            terminal_move_cursor_home();
            break;
        case KEY_END:
            terminal_move_cursor_end();
            break;
        case KEY_DELETE:
            terminal_delete_at_cursor();
            break;
        default:
            break;
        }
    }
}
