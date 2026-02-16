// mapper_2.c
// Mapper 2 (UxROM) implementation
// CPU: $8000-$BFFF = switchable 16KiB bank
//      $C000-$FFFF = fixed last 16KiB bank
// PPU: CHR usually not banked; CHR-RAM allowed if present

#include "mapper.h"
#include "rom_loader.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t bank_select;   // selected 16KiB bank for $8000-$BFFF
} Mapper2State;

// Helper to calculate number of 16 KiB PRG banks in the cartridge
static uint32_t num_prg_banks_16k(const Cartridge *cart) {
    if (!cart || !cart->prg || cart->prg_size < 16384) return 0;
    return (uint32_t)(cart->prg_size / 16384u);
}
    // CPU read implementation for Mapper 2
static bool mapper2_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out || !cart->prg) return false;
    if (addr < 0x8000) return false;
// Mapper 2 has a switchable bank at $8000-$BFFF and a fixed last bank at $C000-$FFFF
    Mapper2State *st = (Mapper2State*)m->state;
    uint32_t banks = num_prg_banks_16k(cart);
    if (banks == 0) return false;

    uint32_t offset;

    // If only 16 KiB PRG, $8000-$FFFF mirrors the single bank; if multiple banks, $C000-$FFFF is fixed to last bank.
    if (addr < 0xC000) {
        // $8000-$BFFF -> selected bank
        uint32_t bank = (uint32_t)(st->bank_select % banks);
        offset = bank * 16384u + (uint32_t)(addr - 0x8000);
    } else {
        // $C000-$FFFF -> last bank fixed
        uint32_t last_bank = banks - 1;
        offset = last_bank * 16384u + (uint32_t)(addr - 0xC000);
    }

    if (offset >= cart->prg_size) return false;
    *out = cart->prg[offset];
    return true;
}

// CPU write implementation for Mapper 2: writes to $8000-$BFFF select the bank; writes to $C000-$FFFF typically do nothing.
static bool mapper2_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)cart;
    if (!m) return false;
    if (addr < 0x8000) return false;

    Mapper2State *st = (Mapper2State*)m->state;
    // Typical UxROM uses low bits for bank select (often 4 bits)
    st->bank_select = (uint8_t)(value & 0x0F);
    return true;
}

// PPU read/write implementations for Mapper 2: typically CHR is not banked, but support CHR-RAM if present.
static bool mapper2_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out || !cart->chr) return false;
    if (addr >= 0x2000) return false;

    // For now, support direct 8KiB CHR (ROM or RAM)
    if (cart->chr_size != 8192u) return false;

    *out = cart->chr[addr];
    return true;
}

// PPU writes only succeed if CHR is RAM
static bool mapper2_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m;
    if (!cart || !cart->chr) return false;
    if (addr >= 0x2000) return false;

    // Only writable if CHR is RAM
    if (!cart->chr_is_ram) return false;
    if (cart->chr_size != 8192u) return false;

    cart->chr[addr] = value;
    return true;
}

static void mapper2_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

// Factory function to create a Mapper 2 instance
Mapper *mapper2_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper2State *st = (Mapper2State*)calloc(1, sizeof(Mapper2State));
    if (!st) {
        free(m);
        return NULL;
    }

    st->bank_select = 0;

    // Initialize function pointers and state for Mapper 2
    m->mapper_id = 2;
    m->cpu_read  = mapper2_cpu_read;
    m->cpu_write = mapper2_cpu_write;
    m->ppu_read  = mapper2_ppu_read;
    m->ppu_write = mapper2_ppu_write;
    m->state     = st;
    m->destroy   = mapper2_destroy;

    return m;
}
