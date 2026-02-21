#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

typedef struct {
    uint8_t buttons;      // 8 bits for buttons
    uint8_t strobe;
    uint8_t shift_reg;
} Controller;

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

// Set all 8 buttons at once (bit0=A ... bit7=Right)
static inline void controller_set_state(Controller* c, uint8_t buttons) {
    c->buttons = buttons;
    if (c->strobe) c->shift_reg = c->buttons;
}

void controller_init(Controller* c);
void controller_set_button(Controller* c, int button, int pressed);
void controller_write(Controller* c, uint8_t value);
uint8_t controller_read(Controller* c);

#endif