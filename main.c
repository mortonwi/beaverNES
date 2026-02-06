// main.c
#include <stdio.h>
#include <stdint.h>

#include "rom_loader.h"
#include "cartridge.h"

// This is a simple test program to verify that ROM loading and cartridge access work correctly.
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s path/to/game.nes\n", argv[0]);
        return 1;
    }

    // Load the ROM into a Cartridge structure
    Cartridge cart;
    char err[256];

    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        fprintf(stderr, "ROM load failed: %s\n", err);
        return 1;
    }

    // Support Mapper 0 (NROM) cartridges.
    if (cart.header.mapper != 0) {
        fprintf(stderr, "Unsupported mapper: %u\n", cart.header.mapper);
        rom_free(&cart);
        return 1;
    }

    printf("Loaded ROM!\n");

    // PRG read check (Mapper 0 CPU window)
    uint8_t v = 0;
    if (cart_cpu_read(&cart, 0x8000, &v)) {
        printf("PRG[0x8000] = 0x%02X\n", v);
    } else {
        printf("PRG[0x8000] read failed\n");
    }

    // Test reading from the second PRG bank if it exists (NROM-256)
    if (cart_cpu_read(&cart, 0xC000, &v)) {
        printf("PRG[0xC000] = 0x%02X\n", v);
    } else {
        printf("PRG[0xC000] read failed\n");
    }

    // CHR read/write check (Mapper 0 PPU window)
    if (cart_ppu_read(&cart, 0x0000, &v)) {
        printf("CHR[0x0000] before = 0x%02X\n", v);
    } else {
        printf("CHR[0x0000] read failed\n");
    }

    // Attempt to write to CHR. This should only succeed if the cartridge uses CHR-RAM (chr_rom_banks == 0).
    if (cart_ppu_write(&cart, 0x0000, 0xAB)) {
        printf("CHR write succeeded (CHR-RAM)\n");
    } else {
        printf("CHR write blocked (CHR-ROM)\n");
    }

    if (cart_ppu_read(&cart, 0x0000, &v)) {
        printf("CHR[0x0000] after  = 0x%02X\n", v);
    } else {
        printf("CHR[0x0000] read failed\n");
    }

    // Print summary info
    printf("PRG banks: %u (%zu bytes)\n", cart.header.prg_rom_banks, cart.prg_size);
    printf("CHR banks: %u (%zu bytes)\n", cart.header.chr_rom_banks, cart.chr_size);
    printf("Mapper: %u\n", cart.header.mapper);
    printf("CHR type: %s\n", cart.chr_is_ram ? "CHR-RAM" : "CHR-ROM");

    printf("Trainer: %s\n", cart.header.has_trainer ? "yes" : "no");
    printf("Mirroring: %s\n",
           cart.header.four_screen
               ? "four-screen"
               : (cart.header.mirroring_vertical ? "vertical" : "horizontal"));

    rom_free(&cart);
    return 0;
}
