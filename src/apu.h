#ifndef APU_H
#define APU_H

#include <stdint.h>

/**
 * @brief Pulse Wave Channel
 * 
 * Square wave audio generator.
 */
typedef struct {
    uint8_t id;                    // pulse 1 or 2

    uint8_t duty_cycle;            // pulse wave duty (0–3)
    uint8_t length_halt;           // length counter halt / envelope loop
    uint8_t constant_volume;       // use constant volume instead of envelope
    uint8_t envelope_volume;       // envelope volume or decay level

    uint8_t sweep_enabled;         // sweep unit enabled
    uint8_t sweep_period;          // sweep period
    uint8_t sweep_negate;          // sweep negate flag
    uint8_t sweep_shift;           // sweep shift count

    // Timer
    uint16_t timer_reload_low;     // timer reload low bits
    uint16_t timer_reload_high;    // timer reload high bits
    uint16_t length_counter;       // length counter value
} Pulse;

/**
 * @brief Triangle Wave Channel
 * 
 * Generated audio waves are triangular
 */
typedef struct {
    uint8_t length_halt;            // length counter halt / linear counter control
    uint16_t linear_reload;         // linear counter reload value

    // Timer
    uint16_t timer_reload_low;      // timer reload low bits
    uint16_t timer_reload_high;     // limer reload high bits
    uint16_t length_counter;        // length counter value
} Triangle;

/**
 * @brief Noise Channel
 * 
 * Semi-random wave generation resulting in noise.
 */
typedef struct {
    uint8_t length_halt;            // length counter halt / envelope loop
    uint8_t constant_volume;        // use constant volume
    uint8_t envelope_volume;        // envelope volume or decay level

    uint8_t lfsr_mode;              // noise mode short / long LFSR
    uint8_t noise_period;           // noise timer period
    uint16_t length_counter;        // length counter value
} Noise;

/**
 * @brief Delta Modulation Channel
 *
 * Allows audio recordings to be played from ROMs on the NES.
 */
typedef struct {
    uint8_t irq_enable;             // IRQ enable flag
    uint8_t loop_enabled;           // loop sample playback
    uint8_t playback_rate;          // DMC playback frequency

    uint16_t sample_counter;        // current sample counter

    uint16_t sample_address;        // starting sample address in memory
    uint16_t sample_length;         // sample length in bytes
} Delta;

/**
 * @brief Audio Processing Unit
 * 
 * Main structure for the audio processing unit. Includes all audio channels.
 */
typedef struct {
    Pulse pulse1;
    Pulse pulse2;
    Triangle triangle;
    Noise noise;
    Delta delta;
} APU;

#endif
