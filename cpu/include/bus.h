#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "memory.h"

#include "../beaverNES-anjelica-dev/rom_loader.h"
#include "../../apu/apu.h"

// Bus acts as the communication channel that connects CPU to memory and later other comoponents (PPU, APU)


typedef struct CPU CPU;

typedef struct Bus {
    Memory *mem; // pointer to memory (RAM)
    CPU *cpu;     
    Cartridge *rom;
    APU *apu;
} Bus;

Bus *bus_create(Memory *mem, APU *apu); 
void bus_write(Bus *bus, uint16_t addr, uint8_t val); // writes a byte to a specific memory address
uint8_t bus_read(Bus *bus, uint16_t addr); // reads a byte from a specific memory address

#endif // BUS_H
