#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

#define SAMPLING_FREQUENCY 48000
#define BUFFER_SIZE 1024

/**
 * @brief Region NTSC vs PAL
 */
typedef enum {
    NTSC,
    PAL
} Region;

/**
 * @brief Registers used to change APU state.
 * 
 *  Mapping comes from NESDEV documenation
 */
typedef struct {
    uint8_t pulse1[4];          // $4000–$4003
    uint8_t pulse2[4];          // $4004–$4007
    uint8_t triangle[4];        // $4008–$400B
    uint8_t noise[4];           // $400C–$400F
    uint8_t dmc[4];             // $4010–$4013

    uint8_t status;             // $4015 Special
    uint8_t frame_counter;      // $4017 Special
} Registers;

/**
 * @brief Pulse Wave Channel
 * 
 * Square wave audio generator.
 */
typedef struct {
    uint8_t id;                    // pulse 1 or 2
    uint8_t enabled;               // is the channel active?

    uint8_t duty_cycle;            // pulse wave duty (0–3)
    uint8_t length_halt;           // length counter halt / envelope loop
    uint8_t constant_volume;       // use constant volume instead of envelope
    
    uint8_t envelope_volume;       // envelope volume or decay level
    uint8_t envelope_divider;      
    uint8_t envelope_decay;
    uint8_t envelope_start;

    uint8_t sweep_enabled;         // sweep unit enabled
    uint8_t sweep_period;          // sweep period
    uint8_t sweep_negate;          // sweep negate flag
    uint8_t sweep_shift;           // sweep shift count
    uint8_t sweep_divider;         // sweep divider
    uint8_t sweep_reload;          // sweep reloader

    // Timer
    uint16_t timer_reload;          // reload value from registers
    uint16_t timer_counter;         // internal countdown

    uint8_t seq_pos;                // duty sequence (0-7)
    uint16_t length_counter;        // internal length counter
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
 * Main structure for the audio processing unit. Includes all audio channels and registers.
 */
typedef struct {
    Region region;
    Registers registers;

    Pulse pulse1;
    Pulse pulse2;
    Triangle triangle;
    Noise noise;
    Delta delta;

    uint64_t cycles;        // APU cycle
} APU;

void init_apu(APU *apu, Region region);
float apu_tick(APU *apu, Region region);
void apu_write(APU *apu, uint16_t addr, uint8_t value);

#endif
