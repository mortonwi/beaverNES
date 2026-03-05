#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

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

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s rom.nes\n", argv[0]);
        return 1;
    }

    // --- SDL INIT VIDEO ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_INIT_VIDEO Error: %s\n", SDL_GetError());
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

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU_WIDTH,
        PPU_HEIGHT
    );

    // --- SDL INIT AUDIO ---
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL_INIT_AUDIO Error: %s", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq     = SAMPLE_RATE;
    spec.format   = AUDIO_F32SYS;
    spec.channels = 1;
    spec.samples  = AUDIO_BUFFER_SAMPLES;
    spec.callback = NULL;

    // Create audio device
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
    if (!device) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
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

    float audio_buffer[AUDIO_BUFFER_SAMPLES];
    const double CPU_PER_SAMPLE = 1789773.0 / (double)SAMPLE_RATE; // NTCS cycles per sample
    double cpu_cycle_accum = 0.0;

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // --- Generate one audio buffer worth of samples ---
        for (int i = 0; i < AUDIO_BUFFER_SAMPLES; i++) {
            cpu_cycle_accum += CPU_PER_SAMPLE;

            while (cpu_cycle_accum >= 1.0) {
                cpu_step(cpu);

                // PPU clocks 3 times per CPU tick
                ppu_clock();
                ppu_clock();
                ppu_clock();

                if (ppu_poll_nmi())
                    cpu_nmi(cpu);

                // Tick just the APU state
                apu_tick(apu);
                cpu_cycle_accum -= 1.0;
            }
            
            float sample = apu_get_output(apu);
            audio_buffer[i] = (sample * 2.0f) - 1.0f;
        }

        // Play audio
        while (SDL_GetQueuedAudioSize(device) > AUDIO_BUFFER_SAMPLES * sizeof(float) * 2) {
            SDL_Delay(1);
        }
        SDL_QueueAudio(device, audio_buffer, sizeof(audio_buffer));

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