#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "rom_loader.h"
#include "ppu.h"
#include "opcodes.h"
#define SCALE 3

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s rom.nes\n", argv[0]);
        return 1;
    }

    // --- SDL INIT ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
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

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU_WIDTH,
        PPU_HEIGHT
    );

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

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Run CPU cycles
        for (int i = 0; i < 1000; i++) {
            cpu_step(cpu);
            ppu_clock();
            ppu_clock();
            ppu_clock();
            if (ppu_poll_nmi()) {
                cpu_nmi(cpu);
            }
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

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    rom_free(&cart);
    return 0;
}