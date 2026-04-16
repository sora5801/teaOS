#include <stdint.h>

#include <kernel/tty.h>
#include <gdt.h>
#include <idt.h>
#include <irq.h>
#include <paging.h>
#include <keyboard.h>
#include <line_editor.h>

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

    __asm__ volatile ("sti");

    for (;;) {
        terminal_writestring("> ");
        line_editor_readline(line, sizeof(line));

        terminal_writestring("You typed: ");
        terminal_writestring(line);
        terminal_putchar('\n');
    }
}
