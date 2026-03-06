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

#define SCALE 2
#define AUDIO_BUFFER_SAMPLES 2048
#define SAMPLE_RATE 48000
#define CPU_HZ 1789773.0
#define MASTER_VOLUME 0.5f

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s rom.nes\n", argv[0]);
        return 1;
    }

    // --- SDL INIT VIDEO & AUDIO ---
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_INIT Error: %s\n", SDL_GetError());
        return 1;
    }

    // --- Create window and renderer --- 
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

    // --- SDL Audio Setup ---
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_F32SYS;
    want.channels = 1;
    want.samples  = AUDIO_BUFFER_SAMPLES;
    want.callback = NULL;

    // Create audio device
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!device) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_PauseAudioDevice(device, 0);

    // --- Emulator Init ---
    Memory *memory = memory_create();
    APU *apu = apu_create();
    Bus *bus = bus_create(memory, apu);
    CPU *cpu = cpu_create(bus);

    Cartridge cart;
    char err[256];

    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        printf("ROM load failed: %s\n", err);
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

    // use the real sample rate so CPU_PER_SAMPLE is accurate
    const double CPU_PER_SAMPLE = CPU_HZ / (double)have.freq;
    double cpu_sample_frac = 0.0;

    float audio_buffer[AUDIO_BUFFER_SAMPLES];

    // Keep ~80ms queued — prevents gaps without building up excess latency
    const Uint32 TARGET_QUEUED_BYTES = (Uint32)(have.freq * sizeof(float) * 0.08);

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Generate audio only when the queue needs topped off
        if (SDL_GetQueuedAudioSize(device) < TARGET_QUEUED_BYTES) {
            for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
            
                cpu_sample_frac += CPU_PER_SAMPLE;
                int cycles_to_run = (int)cpu_sample_frac;
                cpu_sample_frac -= (double)cycles_to_run;

                float accum = 0.0f;
                int accum_count = 0;

                while (cycles_to_run > 0) {
                    // cpu_step returns the real cycle count for this instruction
                    int inst_cycles = cpu_step(cpu);
                    if (inst_cycles < 1) inst_cycles = 1;

                    cycles_to_run -= inst_cycles;

                    for (int c = 0; c < inst_cycles; c++) {
                        // PPU runs 3 cycles per CPU cycle
                        for (int p = 0; p < 3; p++) {
                            ppu_clock();
                            if (ppu_poll_nmi())
                                cpu_nmi(cpu);
                        }

                        // APU ticks once per CPU cycle
                        apu_tick(apu);

                        accum += apu_get_output(apu);
                        accum_count++;
                    }
                }

                // Average to [0..1], then remap to [-1..+1] for SDL
                float s = (accum_count > 0) ? (accum / (float)accum_count) : 0.0f;
                audio_buffer[i] = s * MASTER_VOLUME;
            }

            // queue audio only if there is space
            SDL_QueueAudio(device, audio_buffer, sizeof(audio_buffer));
        }


        // Update texture from PPU framebuffer
        SDL_UpdateTexture(
            texture,
            NULL,
            ppu_get_framebuffer(),
            PPU_WIDTH * sizeof(uint32_t)
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