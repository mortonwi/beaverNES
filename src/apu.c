#include "apu.h"

// pulse sequencer duty cycle sequences
static uint8_t duty[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};

static void init_pulse(Pulse *pulse, uint8_t id) {
    pulse->id = id;
}