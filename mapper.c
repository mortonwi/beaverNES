// mapper.c
#include "mapper.h"
#include "rom_loader.h"   // for Cartridge definition

#include <stdlib.h>
#include <string.h>

// Mapper 0 (NROM)

static bool mapper0_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out) return false;

    // CPU PRG window: $8000-$FFFF
    if (addr < 0x8000) return false;

    // NROM PRG is either 16KiB or 32KiB
    if (cart->prg_size != 16384 && cart->prg_size != 32768) return false;

    uint32_t offset = (uint32_t)(addr - 0x8000);

    // If 16KiB, mirror into $C000-$FFFF
    if (cart->prg_size == 16384) {
        offset %= 16384;
    }

    *out = cart->prg[offset];
    return true;
}

static bool mapper0_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m; (void)cart; (void)addr; (void)value;
    // NROM has no PRG write behavior (ROM is read-only)
    return false;
}

static bool mapper0_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out) return false;

    // PPU CHR window: $0000-$1FFF
    if (addr >= 0x2000) return false;
    if (!cart->chr || cart->chr_size != 8192) return false;

    *out = cart->chr[addr];
    return true;
}

static bool mapper0_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m;
    if (!cart) return false;

    // PPU CHR window: $0000-$1FFF
    if (addr >= 0x2000) return false;

    // Only writable if CHR is RAM
    if (!cart->chr_is_ram) return false;
    if (!cart->chr || cart->chr_size != 8192) return false;

    cart->chr[addr] = value;
    return true;
}

static void mapper0_destroy(Mapper *m) {
    // No extra state
    (void)m;
}

static Mapper *mapper0_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    m->mapper_id = 0;
    m->cpu_read  = mapper0_cpu_read;
    m->cpu_write = mapper0_cpu_write;
    m->ppu_read  = mapper0_ppu_read;
    m->ppu_write = mapper0_ppu_write;
    m->state     = NULL;
    m->destroy   = mapper0_destroy;

    return m;
}


// Mapper 2 (UxROM)
// - CPU: $8000-$BFFF = switchable 16KiB bank
//        $C000-$FFFF = fixed last 16KiB bank
// - PPU: CHR is usually RAM (or ROM), no banking here

typedef struct {
    uint8_t bank_select;  // selected 16KiB bank for $8000-$BFFF
} Mapper2State;

static uint32_t mapper2_num_prg_banks_16k(const Cartridge *cart) {
    // prg_size is bytes; each bank is 16KiB
    return (cart && cart->prg_size) ? (uint32_t)(cart->prg_size / 16384) : 0;
}

static bool mapper2_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out) return false;
    if (addr < 0x8000) return false;
    if (!cart->prg || cart->prg_size < 16384) return false;

    Mapper2State *st = (Mapper2State*)m->state;
    uint32_t banks = mapper2_num_prg_banks_16k(cart);
    if (banks == 0) return false;

    uint32_t offset = 0;

    if (addr < 0xC000) {
        // $8000-$BFFF -> selected bank
        uint32_t bank = (uint32_t)st->bank_select % banks;
        offset = bank * 16384u + (uint32_t)(addr - 0x8000);
    } else {
        // $C000-$FFFF
        uint32_t last_bank = banks - 1;
        offset = last_bank * 16384u + (uint32_t)(addr - 0xC000);
    }

    if (offset >= cart->prg_size) return false;
    *out = cart->prg[offset];
    return true;
}

static bool mapper2_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)cart;
    if (!m) return false;

    // UxROM bank select is typically written anywhere in $8000-$FFFF
    if (addr < 0x8000) return false;

    Mapper2State *st = (Mapper2State*)m->state;

    st->bank_select = (uint8_t)(value & 0x0F);
    return true;
}

static bool mapper2_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    return mapper0_ppu_read(m, cart, addr, out);
}

static bool mapper2_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    // Allow CHR writes only if CHR-RAM
    return mapper0_ppu_write(m, cart, addr, value);
}

static void mapper2_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

static Mapper *mapper2_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper2State *st = (Mapper2State*)calloc(1, sizeof(Mapper2State));
    if (!st) {
        free(m);
        return NULL;
    }

    st->bank_select = 0; // default bank at startup

    m->mapper_id = 2;
    m->cpu_read  = mapper2_cpu_read;
    m->cpu_write = mapper2_cpu_write;
    m->ppu_read  = mapper2_ppu_read;
    m->ppu_write = mapper2_ppu_write;
    m->state     = st;
    m->destroy   = mapper2_destroy;

    return m;
}


Mapper *mapper_create(uint8_t mapper_id) {
    // Factory function to create mapper instances based on mapper_id
    switch (mapper_id) {
        case 0: return mapper0_create();
        case 2: return mapper2_create();
        default: return NULL;
    }
}

void mapper_destroy(Mapper *m) {
    if (!m) return;
    if (m->destroy) {
        m->destroy(m);
    }
    free(m);
}
