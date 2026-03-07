#ifndef BUS_H
#define BUS_H

#include <stdint.h>

#include "memory.h"

// notes from elvis-dev: changed paths to match new file structure
#include "rom_loader.h"
#include "apu.h"
#include "ppu.h"

// NEW: controller support
#include "controller.h"

// Bus acts as the communication channel that connects CPU to memory and other components (PPU, APU, etc.)

typedef struct CPU CPU;
typedef struct Cartridge Cartridge;

typedef struct Bus {
    Memory *mem; // pointer to memory (RAM)
    CPU *cpu;
    Cartridge *rom;
    APU *apu;
    uint8_t dmc_stall_cycles;  // CPU cycles remaining before current DMC DMA stall

    // NEW: NES controllers
    Controller pad1;
    Controller pad2; // optional (player 2)
} Bus;

Bus *bus_create(Memory *mem, APU *apu);
void bus_write(Bus *bus, uint16_t addr, uint8_t val);
uint8_t bus_read(Bus *bus, uint16_t addr);
void bus_service_dmc_dma(Bus *bus); // manages the DMC DMA stall

#endif // BUS_H
