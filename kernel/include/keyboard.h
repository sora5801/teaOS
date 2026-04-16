#ifndef KEYBOARD_H
#define KEYBOARD_H

enum {
    KEY_NONE = 0,

    KEY_UP = 256,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_DELETE
};

int keyboard_getkey(void);
int keyboard_has_key(void);
void keyboard_handle_irq(void);
int keyboard_capslock_enabled(void);

#endif
