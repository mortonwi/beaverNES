// mapper_0.c
// Mapper 0 (NROM) implementation

#include "mapper.h"       // Mapper interface
#include "rom_loader.h"   // Cartridge definition

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static bool mapper0_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out || !cart->prg) return false;

    // Mapper 0: CPU PRG window is $8000-$FFFF
    if (addr < 0x8000) return false;

    // NROM PRG is either 16 KiB (NROM-128) or 32 KiB (NROM-256)
    const uint32_t prg_size = (uint32_t)cart->prg_size;
    if (prg_size != 16384u && prg_size != 32768u) return false;

    uint32_t offset = (uint32_t)(addr - 0x8000);

    // If only 16 KiB PRG, mirror $C000-$FFFF onto $8000-$BFFF
    if (prg_size == 16384u) {
        offset &= 0x3FFFu; // same as % 16384, but faster/clear for power-of-two
    }

    *out = cart->prg[offset];
    return true;
}
// Mapper 0 has no PRG writes (ROM), so this always fails.
static bool mapper0_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m; (void)cart; (void)addr; (void)value;
    // Mapper 0 PRG is ROM (writes typically do nothing / not supported)
    return false;
}
// Mapper 0 PPU read/write implementations below; these depend on whether CHR is ROM or RAM, but the address window is the same.
static bool mapper0_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out || !cart->chr) return false;

    // Mapper 0: PPU CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    // Commonly 8 KiB for NROM; if you later support other CHR sizes, relax this
    if (cart->chr_size != 8192u) return false;

    *out = cart->chr[addr];
    return true;
}
// Mapper 0 PPU writes only succeed if CHR is RAM (some NROM carts have CHR-RAM instead of CHR-ROM).
static bool mapper0_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m;
    if (!cart || !cart->chr) return false;

    // Mapper 0: PPU CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    // Only writable if CHR is RAM
    if (!cart->chr_is_ram) return false;

    if (cart->chr_size != 8192u) return false;

    cart->chr[addr] = value;
    return true;
}

static void mapper0_destroy(Mapper *m) {
    // Mapper 0 has no extra heap state right now.
    (void)m;
}

// Factory function to create a Mapper 0 instance
Mapper *mapper0_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    // Initialize function pointers and state for Mapper 0
    m->mapper_id = 0;
    m->cpu_read  = mapper0_cpu_read;
    m->cpu_write = mapper0_cpu_write;
    m->ppu_read  = mapper0_ppu_read;
    m->ppu_write = mapper0_ppu_write;
    m->destroy   = mapper0_destroy;
    m->state     = NULL;

    return m;
}
