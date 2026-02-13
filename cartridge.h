#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include "rom_loader.h"

bool cart_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out);
bool cart_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out);
bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value);

#endif
