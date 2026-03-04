#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include "cartridge.h"

#define PPU_WIDTH  256
#define PPU_HEIGHT 240

// Public interface

void ppu_init(void);
void ppu_clock(void);

void ppu_write(uint16_t addr, uint8_t value);
uint8_t ppu_read(uint16_t addr);

// Cartridge integration
void ppu_connect_cartridge(Cartridge *cart);

// Framebuffer access
uint32_t *ppu_get_framebuffer(void);

// NMI check
uint8_t ppu_poll_nmi(void);

#endif