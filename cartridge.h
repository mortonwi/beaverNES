#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include "rom_loader.h"   // for Cartridge

// CPU reads PRG-ROM through the cartridge at $8000-$FFFF
bool cart_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out);

// PPU reads CHR through the cartridge at $0000-$1FFF
bool cart_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out);



#endif