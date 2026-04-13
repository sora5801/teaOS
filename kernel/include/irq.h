#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include <idt.h>

void irq_init(void);
void irq_handler(struct registers *r);

#endif
