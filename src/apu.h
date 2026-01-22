#ifndef APU_H
#define APU_H

#include <cstdint>

/*
 * Contains:
 *      Envelope Generator
 *      Sweep Unit (ignored for now)
 *      Timer
 *      8-Step Sequencer
 *      Length Counter
 */
typedef struct {
    uint8_t id;                 // pulsewave 1 or 2
    uint8_t duty;               // selects pulse wave type
    uint8_t length;             // length of audio (frozen if looping)
    uint8_t envelope;           // decreasing saw envelope creates a decay effect
    uint8_t constant_volume;
    uint8_t volume;
    
    uint16_t timer;             // 11-bit timer controlling pitch
    uint16_t timer_counter;

    uint8_t enabled;            // silence / unsilence channel
    uint8_t phase;              // current position in the waveform
} Pulse;

typedef struct {
    
} Triangle;

typedef struct {
    
} Noise;

typedef struct {
    
} DMC;

typedef struct {

} APU;

#endif