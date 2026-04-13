#include <stdint.h>
#include <io.h>
#include <irq.h>
#include <keyboard.h>

/* PIC ports */
#define PIC1_CMD   0x20
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0
#define PIC2_DATA  0xA1

#define PIC_EOI    0x20

#define ICW1_INIT  0x10
#define ICW1_ICW4  0x01
#define ICW4_8086  0x01

static void io_wait(void)
{
    outb(0x80, 0);
}

static void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);

    outb(PIC1_CMD, PIC_EOI);
}

static void pic_remap(uint8_t offset1, uint8_t offset2)
{
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void irq_init(void)
{
    /* map IRQ0..7 => 32..39, IRQ8..15 => 40..47 */
    pic_remap(0x20, 0x28);

    /* unmask only IRQ1 (keyboard) on master for now; keep slave masked */
    outb(PIC1_DATA, 0xFD); /* 11111101b: enable IRQ1 only */
    outb(PIC2_DATA, 0xFF);
}

void irq_handler(struct registers *r)
{
    uint8_t irq = (uint8_t)(r->int_no - 32);

    if (irq == 1) {
        keyboard_handle_irq();
    }

    pic_send_eoi(irq);
}
