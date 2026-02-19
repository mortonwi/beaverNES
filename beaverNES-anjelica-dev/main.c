// main.c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "rom_loader.h"
#include "cartridge.h"

// Helper to dump a range of CPU addresses from the cartridge for testing
static void dump_cpu_bytes(const Cartridge *cart, uint16_t addr, int count) {
    printf("0x%04X:", addr);
    for (int i = 0; i < count; i++) {
        uint8_t v = 0;
        if (cart_cpu_read(cart, (uint16_t)(addr + i), &v)) {
            printf(" %02X", v);
        } else {
            printf(" ??");
        }
    }
    printf("\n");
}

// Helper to convert mirroring mode to string for display
static const char *mirroring_str(const Cartridge *cart) {
    if (cart->header.four_screen) return "four-screen";
    return cart->header.mirroring_vertical ? "vertical" : "horizontal";
}

// Entry point: load ROM specified as command line argument and print info + test reads/writes.
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s path/to/game.nes\n", argv[0]);
        return 1;
    }

    Cartridge cart;
    char err[256];

    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        fprintf(stderr, "ROM load failed: %s\n", err);
        return 1;
    }

    printf("Loaded ROM!\n\n");

    //Summary
    printf("PRG banks: %u (%zu bytes)\n", cart.header.prg_rom_banks, cart.prg_size);
    printf("CHR banks: %u (%zu bytes)\n", cart.header.chr_rom_banks, cart.chr_size);
    printf("Mapper:    %u\n", cart.header.mapper);
    printf("CHR type:  %s\n", cart.chr_is_ram ? "CHR-RAM" : "CHR-ROM");
    printf("Trainer:   %s\n", cart.header.has_trainer ? "yes" : "no");
    printf("Mirroring: %s\n", mirroring_str(&cart));
    printf("\n");

    //Basic PRG checks
    uint8_t v = 0;
    if (cart_cpu_read(&cart, 0x8000, &v)) {
        printf("PRG[0x8000] = 0x%02X\n", v);
    } else {
        printf("PRG[0x8000] read failed\n");
    }

    // If 16 KiB PRG, $C000-$FFFF should mirror $8000-$BFFF; if 32 KiB, should be independent
    if (cart_cpu_read(&cart, 0xC000, &v)) {
        printf("PRG[0xC000] = 0x%02X\n", v);
    } else {
        printf("PRG[0xC000] read failed\n");
    }

    //Mapper 2 (UxROM) bank switching test
    if (cart.header.mapper == 2) {
        printf("\n[Mapper 2] Bank switching test (dump 16 bytes)\n");

        // Bank 0 at $8000
        cart_cpu_write(&cart, 0x8000, 0);
        printf("Bank 0 @ $8000  "); dump_cpu_bytes(&cart, 0x8000, 16);

        // Bank 1 at $8000
        cart_cpu_write(&cart, 0x8000, 1);
        printf("Bank 1 @ $8000  "); dump_cpu_bytes(&cart, 0x8000, 16);

        // Bank 2 at $8000
        cart_cpu_write(&cart, 0x8000, 2);
        printf("Bank 2 @ $8000  "); dump_cpu_bytes(&cart, 0x8000, 16);

        // Fixed bank at $C000 should remain constant
        cart_cpu_write(&cart, 0x8000, 0);
        printf("Fixed @ $C000 (after selecting bank 0): ");
        dump_cpu_bytes(&cart, 0xC000, 16);

        cart_cpu_write(&cart, 0x8000, 2);
        printf("Fixed @ $C000 (after selecting bank 2): ");
        dump_cpu_bytes(&cart, 0xC000, 16);
    }

    //CHR read/write test
    printf("\nCHR test @ $0000:\n");
    if (cart_ppu_read(&cart, 0x0000, &v)) {
        printf("CHR[0x0000] before = 0x%02X\n", v);
    } else {
        printf("CHR[0x0000] read failed\n");
    }

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

    rom_free(&cart);
    return 0;
}
