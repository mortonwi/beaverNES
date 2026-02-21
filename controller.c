#include "controller.h"

void controller_init(Controller* c) {
    c->buttons   = 0;
    c->strobe    = 0;
    c->shift_reg = 0;
}

void controller_write(Controller* c, uint8_t value) {
    uint8_t new_strobe = value & 1u;

    // If strobe goes high, latch current buttons into shift register
    // (common/simple behavior and works for tests)
    if (new_strobe) {
        c->shift_reg = c->buttons;
    }

    c->strobe = new_strobe;
}

uint8_t controller_read(Controller* c) {
    uint8_t bit0;

    if (c->strobe) {
        // While strobe is high, reads return live A button (bit0)
        bit0 = c->buttons & 1u;
    } else {
        // While strobe is low, reads shift out latched bits LSB-first
        bit0 = c->shift_reg & 1u;
        // After shifting out 8 bits, real controllers keep returning 1s.
        c->shift_reg = (uint8_t)((c->shift_reg >> 1) | 0x80);
    }

    return (uint8_t)(bit0 | 0x40);
}