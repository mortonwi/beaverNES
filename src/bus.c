#include "bus.h"
#include <stdlib.h>

Bus *bus_create(Memory *mem) {
    Bus *bus = malloc(sizeof(Bus));
    bus->mem = mem;
    bus->cpu = NULL;
    bus->rom = NULL;
    return bus;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t value) {
    // ROM region is read-only
    if (addr >= 0x8000) {
        return;
    }
    memory_write(bus->mem, addr, value);
}

uint8_t bus_read(Bus *bus, uint16_t addr) {
    // PRG ROM region
    if (addr >= 0x8000) {
        uint32_t index = addr - 0x8000;

        if (bus->rom && bus->rom->header.prg_rom_banks == 1) {
            index %= 0x4000; // mirror 16 KB PRG
        }
        return bus->rom ? bus->rom->prg[index] : 0xFF;
    }
    return memory_read(bus->mem, addr);
}