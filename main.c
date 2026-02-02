#include <stdio.h>
#include "rom_loader.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s path/to/game.nes\n", argv[0]);
        return 1;
    }

    Cartridge cart;
    char err[256];

    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        printf("ROM load failed: %s\n", err);
        return 1;
    }

    printf("Loaded ROM!\n");
    printf("PRG banks: %u (%zu bytes)\n", cart.header.prg_rom_banks, cart.prg_size);
    printf("CHR banks: %u (%zu bytes)\n", cart.header.chr_rom_banks, cart.chr_size);
    printf("Mapper: %u\n", cart.header.mapper);

    rom_free(&cart);
    return 0;
}
