#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void paging_init(void);

static inline uint32_t read_cr2(void)
{
    uint32_t value;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(value));
    return value;
}

#endif
