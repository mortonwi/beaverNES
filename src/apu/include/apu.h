#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Region NTSC vs PAL
 */
typedef enum {
    NTSC,
    PAL
} Region;

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
    uint8_t length_counter;        // internal length counter
} Pulse;

/**
 * @brief Triangle Wave Channel
 * 
 * Generated audio waves are triangular
 */
typedef struct {
    uint8_t enabled;                // channel enabled

    uint8_t length_counter;         // length counter timer
    uint8_t control_flag;           // length halt + linear control

    uint8_t linear_counter;         // linear counter timer
    uint8_t linear_reload_value;    // reload value from $4008
    uint8_t linear_reload_flag;     // reload trigger flag

    uint16_t timer_reload;          // raw timer period
    uint16_t timer_counter;         // internal timer countdown

    uint8_t seq_pos;                // waveform step (0–31)
} Triangle;

/**
 * @brief Noise Channel
 * 
 * Semi-random wave generation resulting in noise.
 */
typedef struct {
    uint8_t enabled;                // channel enabled
    
    uint8_t length_halt;            // length counter halt / envelope loop
    uint8_t constant_volume;        // use constant volume
    uint8_t envelope_volume;        // envelope volume or decay level

    uint8_t envelope_divider;       // envelope divider counter
    uint8_t envelope_decay;         // current envelope decay level
    uint8_t envelope_start;         // envelope start flag

    uint8_t lfsr_mode;              // noise mode short / long LFSR
    uint8_t noise_period;           // noise timer period

    uint16_t timer_counter;         // internal timer countdown
    uint8_t length_counter;         // length counter value

    uint16_t lfsr;                  // 15-bit Linear Feedback Shift Register
} Noise;

/**
 * @brief Delta Modulation Channel
 *
 * Allows audio recordings to be played from ROMs on the NES.
 */
typedef struct {
    uint8_t  enabled;

    uint8_t  irq_enable;
    uint8_t  loop_enabled;
    uint8_t  playback_rate;         // index into dmc_rate table

    uint8_t  output_level;          // 7-bit DAC value (0–127)
    uint8_t  shift_register;        // 8-bit output shift register
    uint8_t  bits_remaining;        // bits left in shift register (0–8)
    uint8_t  silence_flag;          // output unit silent?

    uint8_t  sample_buffer;         // holds fetched byte
    uint8_t  sample_buffer_empty;   // 1 = buffer needs refill

    uint16_t current_address;       // current read address ($8000–$FFFF)
    uint16_t bytes_remaining;       // bytes left in sample

    uint16_t sample_address;        // starting address = $C000 + ($4012 * 64)
    uint16_t sample_length;         // length in bytes = ($4013 * 16) + 1

    uint16_t timer_counter;
    uint16_t timer_period;          // from dmc_rate table

    uint8_t  irq_flag;
    uint8_t  dma_pending;           // 1 = needs a DMA byte fetch from bus
} Delta;

/**
 * @brief Audio Processing Unit
 * 
 * Main structure for the audio processing unit. Includes all audio channels and timing components.
 */
typedef struct {
    Region region;

    Pulse pulse1;
    Pulse pulse2;
    Triangle triangle;
    Noise noise;
    Delta delta;

    uint64_t cycles;                // APU cycle
    uint8_t frame_mode;             // 0 = 4-step, 1 = 5-step
    uint8_t frame_irq_inhibit;      // IRQ inhibit flag
    uint8_t frame_interrupt;        // Frame interrupt flag
    uint16_t frame_counter_cycles;  // Cycles within current frame

    float pulse_mix_table[31];      // efficient lookup table for pulse output mixing
    float tnd_mix_table[203];    // efficient lookup table for tnd output mixing
} APU;

APU* apu_create();
void apu_free(APU *apu);
void init_apu(APU *apu, Region region);
void apu_tick(APU *apu);
float apu_get_output(APU *apu);
void apu_reset(APU *apu);
void apu_write(APU *apu, uint16_t addr, uint8_t value);
uint8_t apu_read(APU *apu, uint16_t addr);
void dmc_load_sample_byte(APU *apu, uint8_t byte);

#endif
