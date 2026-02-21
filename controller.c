#include "controller.h"

void controller_init(Controller* c) {
    c->buttons = 0;
    c->strobe = 0;
    c->shift_reg = 0;
}

void controller_set_button(Controller* c, int button, int pressed) {
    if (pressed)
        c->buttons |= (uint8_t)(1u << button);
    else
        c->buttons &= (uint8_t)~(1u << button);

    if (c->strobe) {
        c->shift_reg = c->buttons;
    }
}

void controller_write(Controller* c, uint8_t value) {
    uint8_t new_strobe = value & 1u;

    // On a 1 -> 0 transition, latch current buttons into shift register
    if (c->strobe == 1 && new_strobe == 0) {
        c->shift_reg = c->buttons;
    }

    c->strobe = new_strobe;

    // If strobe is 1, keep latching (common behavior)
    if (c->strobe) {
        c->shift_reg = c->buttons;
    }
}

uint8_t controller_read(Controller* c) {
    uint8_t result;

    if (c->strobe) {
        // While strobe high, always return A button
        result = c->buttons & 1u;
    } else {
        // Shift out next bit
        result = c->shift_reg & 1u;
        c->shift_reg >>= 1;

        
        c->shift_reg |= 0x80;
    }

    return result;
}