#include <stdint.h>
#include <stddef.h>

#include <io.h>
#include <keyboard.h>

#define KBD_DATA_PORT 0x60
#define KBD_BUF_SIZE 128

static volatile char kbd_buf[KBD_BUF_SIZE];
static volatile uint32_t kbd_head = 0;
static volatile uint32_t kbd_tail = 0;

/* Modifier state */
static int left_shift = 0;
static int right_shift = 0;
static int caps_lock = 0;

/* For now, ignore extended E0/E1 sequences cleanly */
static int extended_scancode = 0;

/*
 * Unshifted map.
 * Index = set-1 scancode.
 */
static const char keymap[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b',
    '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,   '*', 0,  ' ',
};

/*
 * Shifted map.
 */
static const char keymap_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b',
    '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,   '*', 0,  ' ',
};

static void kbd_push(char c)
{
    uint32_t next = (kbd_head + 1) % KBD_BUF_SIZE;

    if (next == kbd_tail)
        return; /* drop if full */

    kbd_buf[kbd_head] = c;
    kbd_head = next;
}

int keyboard_has_char(void)
{
    return kbd_head != kbd_tail;
}

char keyboard_getchar(void)
{
    char c;

    for (;;) {
        __asm__ volatile ("cli");
        if (kbd_head != kbd_tail) {
            c = kbd_buf[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
            __asm__ volatile ("sti");
            return c;
        }
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
}

static int is_letter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void keyboard_handle_irq(void)
{
    uint8_t scancode = inb(KBD_DATA_PORT);
    int released = (scancode & 0x80) != 0;
    uint8_t code = scancode & 0x7F;
    int shift_active;
    char c;

    /* Handle extended prefixes by ignoring the next byte for now */
    if (scancode == 0xE0 || scancode == 0xE1) {
        extended_scancode = 1;
        return;
    }

    if (extended_scancode) {
        extended_scancode = 0;
        return;
    }

    /* Shift press/release */
    if (code == 0x2A) { /* left shift */
        left_shift = !released;
        return;
    }
    if (code == 0x36) { /* right shift */
        right_shift = !released;
        return;
    }

    /* Caps Lock toggles only on press */
    if (code == 0x3A) {
        if (!released)
            caps_lock = !caps_lock;
        return;
    }

    /* Ignore releases for ordinary keys */
    if (released)
        return;

    if (code >= 128)
        return;

    shift_active = left_shift || right_shift;

    /* Start from shift/unshift map */
    if (shift_active)
        c = keymap_shift[code];
    else
        c = keymap[code];

    if (!c)
        return;

    /*
     * Caps Lock affects letters only.
     * For letters:
     *   output uppercase when Shift XOR CapsLock is true
     *   output lowercase when Shift XOR CapsLock is false
     */
    if (is_letter(c)) {
        int uppercase = shift_active ^ caps_lock;

        if (uppercase) {
            if (c >= 'a' && c <= 'z')
                c = (char)(c - 'a' + 'A');
        } else {
            if (c >= 'A' && c <= 'Z')
                c = (char)(c - 'A' + 'a');
        }
    }

    kbd_push(c);
}
