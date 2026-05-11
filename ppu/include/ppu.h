#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include "cartridge.h"
#include <stdbool.h>
#include <stdio.h>

#define PPU_WIDTH  256
#define PPU_HEIGHT 240

// save state prototypes
bool ppu_save_state(FILE *f);
bool ppu_load_state(FILE *f);
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