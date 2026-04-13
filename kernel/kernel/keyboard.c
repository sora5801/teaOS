#include <stdint.h>
#include <io.h>
#include <keyboard.h>

/*
 * Very small US keyboard map for scancode set 1.
 * Index = scancode, value = ASCII character.
 * We only map common keys for now.
 */
static const char scancode_map[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', /* 0x00 - 0x09 */
    '9', '0', '-', '=', '\b',                         /* 0x0A - 0x0E */
    '\t',                                             /* 0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', /* 0x10 - 0x19 */
    '[', ']', '\n', 0,                                /* 0x1A - 0x1D */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 0x1E - 0x27 */
    '\'', '`', 0, '\\',                               /* 0x28 - 0x2B */
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', /* 0x2C - 0x35 */
    0,   '*', 0,  ' ',                                /* 0x36 - 0x39 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                     /* F1-F10, etc. */
    0, 0, 0, 0, 0, 0,                                 /* rest */
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
    0, 0, 0, 0, 0, 0, 0,
};

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_OUTPUT_FULL 0x01

/*
 * Blocks until a key press is available.
 * Returns ASCII for supported keys, 0 for unsupported ones.
 *
 * This version:
 * - ignores key release scancodes (high bit set)
 * - ignores extended scancodes like 0xE0
 * - ignores Shift/Ctrl/Alt/CapsLock handling
 */
char keyboard_getchar(void) {
    uint8_t scancode;

    for (;;) {
        /* Wait until the controller says output buffer is full */
        if ((inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) == 0) {
            continue;
        }

        scancode = inb(PS2_DATA_PORT);

        /* Ignore key releases */
        if (scancode & 0x80) {
            continue;
        }

        /* Ignore extended scancode prefix for now */
        if (scancode == 0xE0 || scancode == 0xE1) {
            continue;
        }

        if (scancode < sizeof(scancode_map)) {
            return scancode_map[scancode];
        }

        return 0;
    }
}
