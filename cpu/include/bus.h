#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include "memory.h"

//notes from elvis-dev
//changed paths to match new file structure
#include "rom_loader.h"
#include "apu.h"
#include "ppu.h"
// Bus acts as the communication channel that connects CPU to memory and later other comoponents (PPU, APU)


typedef struct CPU CPU;
typedef struct Cartridge Cartridge;


typedef struct Bus {
    Memory *mem; // pointer to memory (RAM)
    CPU *cpu;     
    Cartridge *rom;
    APU *apu;
    uint8_t dmc_stall_cycles;  // CPU cycles remaining before current DMC DMA stall
} Bus;

Bus *bus_create(Memory *mem, APU *apu); 
void bus_write(Bus *bus, uint16_t addr, uint8_t val); // writes a byte to a specific memory address
uint8_t bus_read(Bus *bus, uint16_t addr); // reads a byte from a specific memory address
void bus_service_dmc_dma(Bus *bus); // manages the DMC DMA stall

#endif // BUS_H
