#include "bus.h"
#include <stdlib.h>

#include "../beaverNES-anjelica-dev/cartridge.h"

Bus *bus_create(Memory *mem) {
    Bus *bus = malloc(sizeof(Bus));
    bus->mem = mem;
    bus->cpu = NULL;
    bus->rom = NULL;
    return bus;
}

uint8_t bus_read(Bus *bus, uint16_t addr) {
    // PRG/mapper region
    if (addr >= 0x8000) {
        uint8_t v = 0xFF;
        if (bus->rom && cart_cpu_read(bus->rom, addr, &v)) {
            return v;
        }
        return 0xFF;
    }

    return memory_read(bus->mem, addr);
}

void bus_write(Bus *bus, uint16_t addr, uint8_t value) {
    // PRG/mapper region (bank switching, etc.)
    if (addr >= 0x8000) {
        if (bus->rom) {
            cart_cpu_write(bus->rom, addr, value);
        }
        return;
    }

    memory_write(bus->mem, addr, value);
}
