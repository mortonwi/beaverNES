#include "bus.h"
#include <stdlib.h>

/* notes from elvis-dev-----------------------------------------
changed paths to match new file structure
Include cartridge.h instead of rom_loader.h because bus.c calls
cart_cpu_read/write, which are declared in cartridge.h.
rom_loader.h defines the Cartridge struct but does not declare
the cart_* access functions needed by the bus.
*/
#include "cartridge.h"
#include "apu.h"

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
        uint16_t reg = 0x2000 | (addr & 0x0007);
        return ppu_read(reg);
    }

    // $4000–$4017: APU
    //   if (addr < 0x4000) {
    //     // Stub for now
    //     return 0xFF;
    // }
    /*
    Notes from elvis-dev: added new apu read conditions to fix some errors when compiling beaverNES
    */
    // $4000–$4013
    if (addr >= 0x4000 && addr <= 0x4013)
        return apu_read(bus->apu, addr);

    // $4015
    if (addr == 0x4015)
        return apu_read(bus->apu, addr);

    // $4016 (controller read stub)
    if (addr == 0x4016)
        return 0;

    // $4017
    if (addr == 0x4017)
        return apu_read(bus->apu, addr);
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
        uint16_t reg = 0x2000 | (addr & 0x0007);
        ppu_write(reg, value);
        return;
    }


    // $4000–$4017: APU
    // if (addr < 0x4018) {
    //     apu_write(bus->apu, addr, value);
    //     return;
    // }
    /*
    Notes from elvis-dev: added new apu write conditions to fix some errors when compiling beaverNES
    */
   // $4000–$4013: APU
    if (addr >= 0x4000 && addr <= 0x4013) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // $4014: OAM DMA
    if (addr == 0x4014) {
        uint16_t base = value << 8;
        for (int i = 0; i < 256; i++) {
            uint8_t data = bus_read(bus, base + i);
            ppu_write(0x2004, data);
        }
        return;
    }

    // $4015: APU status
    if (addr == 0x4015) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // $4016: Controller (stub for now)
    if (addr == 0x4016) {
        // ignore for now
        return;
    }

    // $4017: APU frame counter
    if (addr == 0x4017) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // Need to handle I/O registers
    // $4020–$FFFF: Cartridge space (PRG-RAM, PRG-ROM, mapper regs)
    if (bus->rom) {
        cart_cpu_write(bus->rom, addr, value);
    }
}