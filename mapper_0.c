// mapper_0.c
// Mapper 0 (NROM) implementation

#include "mapper.h"
#include "rom_loader.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static bool mapper0_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out) return false;

    // NROM maps CPU $8000-$FFFF to PRG-ROM
    if (addr < 0x8000) return false;

    // PRG can be 16 KiB (NROM-128) or 32 KiB (NROM-256)
    uint32_t prg_size = (uint32_t)cart->prg_size;
    if (prg_size != 16384 && prg_size != 32768) return false;

    uint32_t offset = (uint32_t)(addr - 0x8000);

    // If only 16 KiB PRG, mirror $C000-$FFFF onto $8000-$BFFF
    if (prg_size == 16384) {
        offset %= 16384;
    }

    *out = cart->prg[offset];
    return true;
}

static bool mapper0_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m; (void)cart; (void)addr; (void)value;
    // Mapper 0 typically does not support PRG writes
    return false;
}

static bool mapper0_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out) return false;

    // CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    if (!cart->chr || cart->chr_size != 8192) return false;

    *out = cart->chr[addr];
    return true;
}

static bool mapper0_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m;
    if (!cart) return false;

    // CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    // Only writable if this cartridge uses CHR-RAM
    if (!cart->chr_is_ram) return false;

    if (!cart->chr || cart->chr_size != 8192) return false;

    cart->chr[addr] = value;
    return true;
}

static void mapper0_destroy(Mapper *m) {
    // If you add heap-allocated state later, free it here.
    (void)m;
}

Mapper *mapper0_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    m->mapper_id = 0;
    m->cpu_read  = mapper0_cpu_read;
    m->cpu_write = mapper0_cpu_write;
    m->ppu_read  = mapper0_ppu_read;
    m->ppu_write = mapper0_ppu_write;
    m->destroy   = mapper0_destroy;
    m->state     = NULL;

    return m;
}
