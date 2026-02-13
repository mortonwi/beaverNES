// mapper_0.c
// Mapper 0 (NROM) implementation
// - PRG-ROM: 16 KiB (mirrored) or 32 KiB (no mirror) mapped at $8000-$FFFF
// - CHR: 8 KiB at $0000-$1FFF (CHR-ROM read-only OR CHR-RAM read/write)

#include "mapper.h"
#include "rom_loader.h"   // for Cartridge fields (prg/chr sizes, chr_is_ram, etc.)
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

//CPU READ (PRG)
static bool m0_cpu_read(Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;

    // NROM maps PRG into CPU $8000-$FFFF
    if (addr < 0x8000) return false;
    if (!cart->prg || cart->prg_size == 0) return false;

    // NROM-128 = 16 KiB, NROM-256 = 32 KiB
    if (cart->prg_size != 16384 && cart->prg_size != 32768) return false;

    uint32_t offset = (uint32_t)(addr - 0x8000);

    // If 16 KiB PRG, mirror $C000-$FFFF onto $8000-$BFFF
    if (cart->prg_size == 16384) {
        offset &= 0x3FFF;
    }

    *out = cart->prg[offset];
    return true;
}

// Mapper 0 has no PRG bank switching
static bool m0_cpu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)cart;
    (void)addr;
    (void)value;
    return false;
}

//PPU READ (CHR)
static bool m0_ppu_read(Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;

    // NROM CHR is visible at PPU $0000-$1FFF
    if (addr >= 0x2000) return false;
    if (!cart->chr || cart->chr_size == 0) return false;

    if (cart->chr_size != 8192) return false;

    *out = cart->chr[addr];
    return true;
}

// PPU WRITE (CHR-RAM only)
static bool m0_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart) return false;

    // CHR address range
    if (addr >= 0x2000) return false;

    // Only writable if this cartridge uses CHR-RAM
    if (!cart->chr_is_ram) return false;
    if (!cart->chr || cart->chr_size != 8192) return false;

    cart->chr[addr] = value;
    return true;
}

static void m0_destroy(struct Mapper *m) {
    if (!m) return;
    m->state = NULL;
}

// Factory function used by mapper.c
Mapper *mapper0_create(void) {
    Mapper *m = (Mapper *)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    m->mapper_id = 0;
    m->cpu_read  = m0_cpu_read;
    m->cpu_write = m0_cpu_write;
    m->ppu_read  = m0_ppu_read;
    m->ppu_write = m0_ppu_write;

    m->state = NULL;
    m->destroy = m0_destroy;

    return m;
}
