#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#include "rom_loader.h"   // Cartridge
#include "mapper.h"       // Mapper interface

// CPU reads PRG/mapper space through the cartridge
bool cart_cpu_read(Cartridge *cart, uint16_t addr, uint8_t *out);
bool cart_cpu_write(Cartridge *cart, uint16_t addr, uint8_t value);

// PPU reads/writes CHR through the cartridge
bool cart_ppu_read(Cartridge *cart, uint16_t addr, uint8_t *out);
bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value);

#endif
