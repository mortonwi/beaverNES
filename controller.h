#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

typedef struct {
    uint8_t buttons;    // latched "current" buttons (A..Right)
    uint8_t strobe;     // 1 = strobe on
    uint8_t shift_reg;  // serial shift register
} Controller;

// Button bit positions (bit0 = A ... bit7 = Right)
typedef enum {
    BTN_A = 0,
    BTN_B,
    BTN_SELECT,
    BTN_START,
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT
} NesButton;

void controller_init(Controller* c);

// Set all 8 buttons at once (bit0=A ... bit7=Right)
static inline void controller_set_state(Controller* c, uint8_t buttons) {
    c->buttons = buttons;
    // If strobe is high, controller should reflect live state immediately.
    if (c->strobe) {
        c->shift_reg = c->buttons;
    }
}

static inline void controller_set_button(Controller* c, NesButton button, int pressed) {
    if (pressed) c->buttons |=  (1u << button);
    else         c->buttons &= ~(1u << button);

    if (c->strobe) {
        c->shift_reg = c->buttons;
    }
}

// CPU writes to $4016: bit0 is strobe
void    controller_write(Controller* c, uint8_t value);

// CPU reads from $4016: returns serial bit in bit0 (often OR 0x40)
uint8_t controller_read(Controller* c);

#endif