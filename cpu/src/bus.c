#include "bus.h"
#include <stdlib.h>

#include "../beaverNES-anjelica-dev/cartridge.h"
#include "../../apu/apu.h"

// Resource for CPU memory map:
// https://www.nesdev.org/wiki/CPU_memory_map

Bus *bus_create(Memory *mem, APU *apu) {
    Bus *bus = malloc(sizeof(Bus));
    bus->mem = mem;
    bus->cpu = NULL;
    bus->rom = NULL;
    bus->apu = apu;
    return bus;
}

uint8_t bus_read(Bus *bus, uint16_t addr) {

    // $0000–$1FFF: 2 KB internal RAM + mirrors
    if (addr < 0x2000) {
        return memory_read(bus->mem, addr & 0x07FF);
    }

    // $2000–$3FFF: PPU registers (mirrored every 8 bytes)
    if (addr < 0x4000) {
        // Stub for now
        return 0xFF;
    }

    // $4000–$4017: APU
    if (addr < 0x4018) {
        return apu_read(bus->apu, addr);
    }

    // Handle I/O registers

    // $4020–$FFFF: Cartridge space
    uint8_t v = 0xFF;
    if (bus->rom && cart_cpu_read(bus->rom, addr, &v)) {
        return v;
    }

    return 0xFF;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t value) {

    // $0000–$1FFF: 2 KB internal RAM + mirrors
    if (addr < 0x2000) {
        memory_write(bus->mem, addr & 0x07FF, value);
        return;
    }

    // $2000–$3FFF: PPU registers (mirrored every 8 bytes)
    if (addr < 0x4000) {
        // Stub for now
        return;
    }

    // $4000–$4017: APU
    if (addr < 0x4018) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // Need to handle I/O registers

    // $4020–$FFFF: Cartridge space (PRG-RAM, PRG-ROM, mapper regs)
    if (bus->rom) {
        cart_cpu_write(bus->rom, addr, value);
    }
}