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
#include "controller.h"

// Resource for CPU memory map:
// https://www.nesdev.org/wiki/CPU_memory_map

Bus *bus_create(Memory *mem, APU *apu) {
    Bus *bus = (Bus*)malloc(sizeof(Bus));
    if (!bus) return NULL;

    bus->mem = mem;
    bus->cpu = NULL;
    bus->rom = NULL;
    bus->apu = apu;

    // NEW: controller
    controller_init(&bus->pad1);
    controller_init(&bus->pad2);

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

    // $4000–$4013: APU
    if (addr >= 0x4000 && addr <= 0x4013)
        return apu_read(bus->apu, addr);

    // $4015: APU status
    if (addr == 0x4015)
        return apu_read(bus->apu, addr);

    // $4016: Controller 1 read
    if (addr == 0x4016)
        return controller_read(&bus->pad1);

    // $4017: APU frame counter read (and controller 2 on real HW, but ignoring)
    if (addr == 0x4017)
        return apu_read(bus->apu, addr);

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

    // $4000–$4013: APU
    if (addr >= 0x4000 && addr <= 0x4013) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // $4014: OAM DMA
    if (addr == 0x4014) {
        uint16_t base = (uint16_t)value << 8;
        for (int i = 0; i < 256; i++) {
            uint8_t data = bus_read(bus, (uint16_t)(base + i));
            ppu_write(0x2004, data);
        }
        return;
    }

    // $4015: APU status
    if (addr == 0x4015) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // $4016: Controller strobe write
    if (addr == 0x4016) {
        controller_write(&bus->pad1, value);
        return;
    }

    // $4017: APU frame counter
    if (addr == 0x4017) {
        apu_write(bus->apu, addr, value);
        return;
    }

    // $4020–$FFFF: Cartridge space (PRG-RAM, PRG-ROM, mapper regs)
    if (bus->rom) {
        cart_cpu_write(bus->rom, addr, value);
    }
}