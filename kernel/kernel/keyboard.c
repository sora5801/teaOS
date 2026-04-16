#include <stdint.h>
#include <stddef.h>

#include <io.h>
#include <keyboard.h>
#include <kernel/tty.h>

#define KBD_DATA_PORT 0x60
#define KBD_BUF_SIZE 128

static volatile int kbd_buf[KBD_BUF_SIZE];
static volatile uint32_t kbd_head = 0;
static volatile uint32_t kbd_tail = 0;

static int left_shift = 0;
static int right_shift = 0;
static int caps_lock = 0;
static int extended_scancode = 0;

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

static void kbd_push(int key)
{
    uint32_t next = (kbd_head + 1) % KBD_BUF_SIZE;

    if (next == kbd_tail)
        return;

    kbd_buf[kbd_head] = key;
    kbd_head = next;
}

int keyboard_has_key(void)
{
    return kbd_head != kbd_tail;
}

int keyboard_getkey(void)
{
    int key;

    for (;;) {
        __asm__ volatile ("cli");
        if (kbd_head != kbd_tail) {
            key = kbd_buf[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
            __asm__ volatile ("sti");
            return key;
        }
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
}

static int is_letter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int decode_extended(uint8_t code)
{
    switch (code) {
    case 0x48: return KEY_UP;
    case 0x50: return KEY_DOWN;
    case 0x4B: return KEY_LEFT;
    case 0x4D: return KEY_RIGHT;
    case 0x47: return KEY_HOME;
    case 0x4F: return KEY_END;
    case 0x53: return KEY_DELETE;
    default:   return KEY_NONE;
    }
}

void keyboard_handle_irq(void)
{
    uint8_t scancode = inb(KBD_DATA_PORT);
    int released = (scancode & 0x80) != 0;
    uint8_t code = scancode & 0x7F;
    int shift_active;
    char c;

    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    if (scancode == 0xE1) {
        /* Ignore Pause/Break sequence for now */
        extended_scancode = 0;
        return;
    }

    if (extended_scancode) {
        extended_scancode = 0;

        if (!released) {
            int ext = decode_extended(code);
            if (ext != KEY_NONE)
                kbd_push(ext);
        }
        return;
    }

    if (code == 0x2A) {
        left_shift = !released;
        return;
    }

    if (code == 0x36) {
        right_shift = !released;
        return;
    }

    if (code == 0x3A) {
        if (!released){
            caps_lock = !caps_lock;
	    terminal_draw_capslock_indicator();
	}
        return;
    }

    if (released)
        return;

    if (code >= 128)
        return;

    shift_active = left_shift || right_shift;
    c = shift_active ? keymap_shift[code] : keymap[code];

    if (!c)
        return;

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

    kbd_push((int)(unsigned char)c);
}

int keyboard_capslock_enabled(void)
{
    return caps_lock;
}
