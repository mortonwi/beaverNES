#include <stdio.h>
#include "rom_loader.h"
#include "cartridge.h"


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

    if (cart.header.mapper != 0) {
    fprintf(stderr, "Unsupported mapper: %u\n", cart.header.mapper);
    rom_free(&cart);
    return 1;
    }

    uint8_t v;

    if (cart_cpu_read(&cart, 0x8000, &v)) {
        printf("PRG[0x8000] = 0x%02X\n", v);
}

    if (cart_cpu_read(&cart, 0xC000, &v)) {
        printf("PRG[0xC000] = 0x%02X\n", v);
}
    
printf("Loaded ROM!\n");
printf("PRG banks: %u (%zu bytes)\n", cart.header.prg_rom_banks, cart.prg_size);
printf("CHR banks: %u (%zu bytes)\n", cart.header.chr_rom_banks, cart.chr_size);
printf("Mapper: %u\n", cart.header.mapper);

printf("Trainer: %s\n", cart.header.has_trainer ? "yes" : "no");
printf("Mirroring: %s\n", cart.header.four_screen
       ? "four-screen"
       : (cart.header.mirroring_vertical ? "vertical" : "horizontal"));

    rom_free(&cart);
    return 0;
}
