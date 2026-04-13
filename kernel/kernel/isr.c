#include <stdint.h>
#include <stddef.h>
#include <idt.h>

/* Provided by your terminal code */
extern void terminal_writestring(const char *data);

static const char *exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

static void terminal_write_dec(uint32_t value)
{
    char buf[16];
    int i = 0;

    if (value == 0) {
        terminal_writestring("0");
        return;
    }

    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        char c[2];
        c[0] = buf[--i];
        c[1] = '\0';
        terminal_writestring(c);
    }
}

void isr_handler(struct registers *r)
{
    terminal_writestring("EXCEPTION: ");
    terminal_write_dec(r->int_no);
    terminal_writestring(" - ");

    if (r->int_no < 32) {
        terminal_writestring(exception_messages[r->int_no]);
    } else {
        terminal_writestring("Unknown");
    }

    terminal_writestring("\n");

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
