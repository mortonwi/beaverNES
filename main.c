#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "rom_loader.h"
#include "ppu.h"
#include "opcodes.h"
#include "apu.h"
#include "controller.h"

#define SCALE 2

#define AUDIO_BUFFER_SAMPLES 512
#define SAMPLE_RATE 48000

// NTSC CPU clock (Hz)
#define CPU_HZ 1789773.0f

static inline float clampf(float x, float lo, float hi) {
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s rom.nes\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "beaverNES",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        PPU_WIDTH * SCALE,
        PPU_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // VSYNC helps keep video pacing sane without manual delays
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU_WIDTH,
        PPU_HEIGHT
    );
    if (!texture) {
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // --- Audio (queued) ---
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = SAMPLE_RATE; // request 48k
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUFFER_SAMPLES;
    want.callback = NULL;

    SDL_AudioDeviceID device =
        SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!device) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("Audio device opened at %d Hz\n", have.freq);
    SDL_PauseAudioDevice(device, 0);

    // --- Emulator Init ---
    Memory *memory = memory_create();
    APU *apu = apu_create();

    // Controller 1
    Controller pad1;
    controller_init(&pad1);

    // NOTE: bus_create signature must match your updated bus.h/bus.c:
    // Bus *bus_create(Memory *mem, APU *apu, Controller *pad1);
    Bus *bus = bus_create(memory, apu);

    CPU *cpu = cpu_create(bus);

    Cartridge cart;
    char err[256];

    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        printf("ROM load failed: %s\n", err);
        SDL_CloseAudioDevice(device);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        apu_free(apu);
        return 1;
    }

    printf("PRG size: %u\n", (unsigned)cart.prg_size);
    printf("CHR size: %u\n", (unsigned)cart.chr_size);
    printf("CHR is RAM: %d\n", cart.chr_is_ram);

    bus->rom = &cart;

    ppu_init();
    ppu_connect_cartridge(&cart);
    init_opcode_table();
    cpu_reset(cpu);

    // cycles per audio sample (~37.287 at 48k)
    const double CPU_PER_SAMPLE = (double)CPU_HZ / (double)have.freq;
    double cpu_sample_frac = 0.0;

    float audio_buffer[AUDIO_BUFFER_SAMPLES];

    bool running = true;
    SDL_Event event;

    // Keep ~80ms queued to avoid huge latency
    const Uint32 TARGET_QUEUED_BYTES = (Uint32)(have.freq * (int)sizeof(float) * 0.08f);

    while (running)
    {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // --- Controller input (keyboard → NES controller) ---
    uint8_t buttons = 0;

    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_X])      buttons |= (1u << BTN_A);
    if (keys[SDL_SCANCODE_Z])      buttons |= (1u << BTN_B);
    if (keys[SDL_SCANCODE_RSHIFT]) buttons |= (1u << BTN_SELECT);
    if (keys[SDL_SCANCODE_RETURN]) buttons |= (1u << BTN_START);

    if (keys[SDL_SCANCODE_UP])     buttons |= (1u << BTN_UP);
    if (keys[SDL_SCANCODE_DOWN])   buttons |= (1u << BTN_DOWN);
    if (keys[SDL_SCANCODE_LEFT])   buttons |= (1u << BTN_LEFT);
    if (keys[SDL_SCANCODE_RIGHT])  buttons |= (1u << BTN_RIGHT);

    controller_set_state(&bus->pad1, buttons);

        // --- Emulation step + audio generation ---
        if (SDL_GetQueuedAudioSize(device) < TARGET_QUEUED_BYTES) {

            for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {

                cpu_sample_frac += CPU_PER_SAMPLE;
                int cycles_to_run = (int)cpu_sample_frac;
                cpu_sample_frac -= (double)cycles_to_run;

                // Average (low-pass) the APU output across the CPU cycles in this sample
                float acc = 0.0f;
                int acc_n = 0;

                while (cycles_to_run > 0) {
                    int inst_cycles = cpu_step(cpu);
                    if (inst_cycles < 1) inst_cycles = 1;

                    cycles_to_run -= inst_cycles;

                    // Run per-CPU-cycle timing
                    for (int c = 0; c < inst_cycles; c++) {

                        // PPU: 3 cycles per CPU cycle
                        for (int p = 0; p < 3; p++) {
                            ppu_clock();
                            if (ppu_poll_nmi())
                                cpu_nmi(cpu);
                        }

                        // APU: tick once per CPU cycle
                        apu_tick(apu);

                        // Accumulate current APU mix (0..1)
                        acc += apu_get_output(apu);
                        acc_n++;
                    }
                }

                float s01 = (acc_n > 0) ? (acc / (float)acc_n) : 0.0f; // 0..1
                float s   = (s01 * 2.0f) - 1.0f;                       // -1..+1

                // Conservative gain to avoid clipping harshness
                s *= 0.35f;

                audio_buffer[i] = clampf(s, -1.0f, 1.0f);
            }

            SDL_QueueAudio(device, audio_buffer, sizeof(audio_buffer));
        }

        // Render current framebuffer
        SDL_UpdateTexture(
            texture,
            NULL,
            ppu_get_framebuffer(),
            PPU_WIDTH * (int)sizeof(uint32_t)
        );

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_CloseAudioDevice(device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    rom_free(&cart);
    apu_free(apu);
    return 0;
}