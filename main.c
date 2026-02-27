#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>
#include <math.h>
#include "src/apu.h"
#include "test_apu.h"

#define CPU_FREQ    1789773  // frequency of a NTCS CPU
#define SAMPLE_RATE 48000    // Audio sample rate
#define REGION      0        // 0:NTSC 1:PAL

/**
 * @brief Test APU audio generation
 */
void play_apu_audio(APU *apu, float time)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return;
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);

    spec.freq = SAMPLE_RATE;
    spec.format = AUDIO_F32SYS;
    spec.channels = 1;
    spec.samples = 1024;
    spec.callback = NULL;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (!device) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(device, 0);

    const int buffer_samples = 1024;
    float buffer[buffer_samples];

    double cycles_per_sample = (double)CPU_FREQ / SAMPLE_RATE;
    double cycle_accumulator = 0.0;

    printf("Playing audio for %f seconds...\n", time);

    Uint32 start = SDL_GetTicks();

    while (SDL_GetTicks() - start < (time * 1000))
    {
        for (int i = 0; i < buffer_samples; i++)
        {
            cycle_accumulator += cycles_per_sample;

            float cpu_sample = 0.0f;

            // Run enough CPU/APU cycles to produce 1 audio sample
            while (cycle_accumulator >= 1.0)
            {
                cpu_sample = apu_tick(apu);
                cycle_accumulator -= 1.0;
            }

            // Convert [0,1] → [-1,1]
            float audio_sample = (cpu_sample * 2.0f) - 1.0f;

            // Safety clamp
            if (audio_sample > 1.0f) audio_sample = 1.0f;
            if (audio_sample < -1.0f) audio_sample = -1.0f;

            buffer[i] = audio_sample;
        }

        SDL_QueueAudio(device, buffer, sizeof(buffer));

        // Prevent runaway latency
        while (SDL_GetQueuedAudioSize(device) > SAMPLE_RATE * sizeof(float))
        {
            SDL_Delay(1);
        }
    }

    SDL_CloseAudioDevice(device);
    SDL_Quit();
}

/**
 * @brief Main entry point.  
 * 
 * Runs the APU test suite and plays audio if all tests pass.
 * Specific writes for running were generated with Claude.
 */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int failures = run_apu_test_suite();

    if (failures == 0) {
        printf("All tests passed!\n");

        APU apu;
        init_apu(&apu, REGION);

        /* ── 1. Pulse only ────────────────────────────────────────────────
        *
        * Channel: Pulse 1
        * Note:    A4 — 440 Hz
        * Timer:   T = CPU_FREQ / (16 * 440) - 1 = 254  ->  f ~= 438.7 Hz
        *
        * $4015 = 0x01   enable pulse 1 only
        * $4000 = 0xBF   duty=2 (50%), length_halt=1, const_vol=1, vol=15
        * $4002 = 0xFE   timer low  = 254
        * $4003 = 0xF8   timer high = 0, length_table[31] = 30 frames
        */
        printf("Playing: Pulse (440 Hz, 0.75 s)...\n");
        apu_reset(&apu);
        apu_write(&apu, 0x4015, 0x01);
        apu_write(&apu, 0x4000, 0xBF);
        apu_write(&apu, 0x4002, 0xFE);
        apu_write(&apu, 0x4003, 0xF8);
        play_apu_audio(&apu, 0.75);

        /* ── 2. Triangle only ─────────────────────────────────────────────
        *
        * Channel: Triangle
        * Note:    A3 — 220 Hz (one octave below pulse)
        * Timer:   T = CPU_FREQ / (32 * 220) - 1 = 253  ->  f ~= 220.2 Hz
        *
        * $4015 = 0x04   enable triangle only
        * $4008 = 0xFF   control=1 (length_halt + linear_reload=127)
        * $400A = 0xFD   timer low  = 253
        * $400B = 0xF8   timer high = 0, length_table[31] = 30 frames
        */
        printf("Playing: Triangle (220 Hz, 0.75 s)...\n");
        apu_reset(&apu);
        apu_write(&apu, 0x4015, 0x04);
        apu_write(&apu, 0x4008, 0xFF);
        apu_write(&apu, 0x400A, 0xFD);
        apu_write(&apu, 0x400B, 0xF8);
        play_apu_audio(&apu, 0.75);

        /* ── 3. Noise only ────────────────────────────────────────────────
        *
        * Channel: Noise
        * Mode:    Long LFSR (bit 7 of $400E = 0) — white noise character
        * Period:  Index 3 -> 32 CPU cycles between LFSR shifts (mid-high freq)
        * Volume:  Constant, level 15 (max)
        *
        * $4015 = 0x08   enable noise only
        * $400C = 0xBF   length_halt=1, const_vol=1, vol=15
        * $400E = 0x03   mode=0 (long LFSR), period index=3
        * $400F = 0xF8   length_table[31] = 30 frames
        */
        printf("Playing: Noise (white, 0.75 s)...\n");
        apu_reset(&apu);
        apu_write(&apu, 0x4015, 0x08);
        apu_write(&apu, 0x400C, 0xBF);
        apu_write(&apu, 0x400E, 0x03);
        apu_write(&apu, 0x400F, 0xF8);
        play_apu_audio(&apu, 0.75);

        /* ── 4. All three combined ────────────────────────────────────────
        *
        * Pulse 1:  440 Hz melody  (same registers as segment 1)
        * Triangle: 220 Hz bass    (same registers as segment 2)
        * Noise:    percussion     (vol=8 so it sits behind the melody)
        *
        * $4015 = 0x0D   enable pulse1 (bit0) + triangle (bit2) + noise (bit3)
        */
        printf("Playing: Pulse + Triangle + Noise combined (1.5 s)...\n");
        apu_reset(&apu);
        apu_write(&apu, 0x4015, 0x0D);

        // Pulse 1 — 440 Hz
        apu_write(&apu, 0x4000, 0xBF);
        apu_write(&apu, 0x4002, 0xFE);
        apu_write(&apu, 0x4003, 0xF8);

        // Triangle — 220 Hz
        apu_write(&apu, 0x4008, 0xFF);
        apu_write(&apu, 0x400A, 0xFD);
        apu_write(&apu, 0x400B, 0xF8);

        // Noise — same period, lower volume so it does not swamp the melody
        apu_write(&apu, 0x400C, 0xB8);  // const_vol=1, vol=8
        apu_write(&apu, 0x400E, 0x03);
        apu_write(&apu, 0x400F, 0xF8);
        play_apu_audio(&apu, 1.5);

        return 0;
    }

    return 1;
}
