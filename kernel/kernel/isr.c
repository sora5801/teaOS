#include <stdint.h>
#include <stddef.h>
#include <idt.h>
#include <paging.h>

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

static void terminal_write_hex(uint32_t value)
{
    char hex[] = "0123456789ABCDEF";
    char out[11];
    out[0] = '0';
    out[1] = 'x';

    for (int i = 0; i < 8; i++) {
        out[9 - i] = hex[value & 0xF];
        value >>= 4;
    }

    out[10] = '\0';
    terminal_writestring(out);
}

static void panic_halt(void)
{
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

static void page_fault_report(struct registers *r)
{
    uint32_t fault_addr = read_cr2();
    uint32_t err = r->err_code;

    int present  =  err        & 0x1;
    int write    = (err >> 1)  & 0x1;
    int user     = (err >> 2)  & 0x1;
    int reserved = (err >> 3)  & 0x1;
    int instr    = (err >> 4)  & 0x1;

    terminal_writestring("\nPAGE FAULT\n");
    terminal_writestring("  addr: ");
    terminal_write_hex(fault_addr);
    terminal_writestring("\n");

    terminal_writestring("  err:  ");
    terminal_write_hex(err);
    terminal_writestring("\n");

    terminal_writestring("  kind: ");
    if (present)
        terminal_writestring("protection violation");
    else
        terminal_writestring("non-present page");
    terminal_writestring("\n");

    terminal_writestring("  access: ");
    if (write)
        terminal_writestring("write");
    else
        terminal_writestring("read");
    terminal_writestring("\n");

    terminal_writestring("  mode: ");
    if (user)
        terminal_writestring("user");
    else
        terminal_writestring("supervisor");
    terminal_writestring("\n");

    if (reserved) {
        terminal_writestring("  note: reserved-bit violation\n");
    }

    if (instr) {
        terminal_writestring("  note: instruction fetch\n");
    }

    terminal_writestring("  eip:  ");
    terminal_write_hex(r->eip);
    terminal_writestring("\n");

    panic_halt();
}

void isr_handler(struct registers *r)
{
    if (r->int_no == 14) {
        page_fault_report(r);
    }

    terminal_writestring("EXCEPTION: ");
    terminal_write_dec(r->int_no);
    terminal_writestring(" - ");

    if (r->int_no < 32)
        terminal_writestring(exception_messages[r->int_no]);
    else
        terminal_writestring("Unknown");

    terminal_writestring("\n");

    panic_halt();
}
