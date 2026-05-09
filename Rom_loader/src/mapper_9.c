// mapper_9.c
// mapper 9 (MMC2) implementation
// based on: https://www.nesdev.org/wiki/MMC2
//
// CPU: $6000-$7FFF: 8 KB PRG RAM bank
//      $8000-$9FFF: 8 KB switchable PRG ROM bank
//      $A000-$FFFF: three 8 KB PRG ROM banks, fixed to the last three banks
// PPU: $0000-$0FFF: two 4 KB switchable CHR ROM banks
//      $1000-$1FFF: two 4 KB switchable CHR ROM banks

#include "mapper.h"
#include "rom_loader.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// state handles latch mechanism and banks
typedef struct {
    uint8_t prg_bank;       // $A000-$AFFF

    uint8_t chr_bank_0FD;   // $B000-$BFFF
    uint8_t chr_bank_0FE;   // $C000-$CFFF
    uint8_t chr_bank_1FD;   // $D000-$DFFF
    uint8_t chr_bank_1FE;   // $E000-$EFFF

    uint8_t mirroring;      // $F000-$FFFF Note: inverted from mapper.h convention (0=H, 1=V)

    uint8_t latch_0;        // selects which CHR bank 0xFD or 0xFE
    uint8_t latch_1;        // selects which CHR bank 1xFD or 1xFE
} Mapper9State;

// -- Helper Functions --

// helper to calculate the total number of 8KB PRG banks in the ROM
static inline uint32_t num_prg_8k(const Cartridge *cart) {
    if (!cart || !cart->prg || cart->prg_size < 0x2000) return 0;
    return (uint32_t)(cart->prg_size / 0x2000);
}

// Resolves a byte pointer into PRG ROM for a given 8KB bank + offset.
// The modulo keeps it in bounds regardless of how prg_bank was written.
static inline uint8_t prg_read_8k(const Cartridge *cart, uint8_t bank, uint16_t offset) {
    uint32_t n = num_prg_8k(cart);
    if (n == 0) return 0xFF;
    uint32_t base = (bank % n) * 0x2000;
    return cart->prg[base + (offset & 0x1FFF)];
}

// Resolves a byte from CHR (ROM or RAM) for a given 4KB bank + offset.
static inline uint8_t chr_read_4k(const Cartridge *cart, uint8_t bank, uint16_t offset) {
    if (!cart->chr || cart->chr_size < 0x1000) return 0xFF;
    uint32_t num_4k = (uint32_t)(cart->chr_size / 0x1000);
    uint32_t base   = (bank % num_4k) * 0x1000;
    return cart->chr[base + (offset & 0x0FFF)];
}

static inline void chr_write_4k(Cartridge *cart, uint8_t bank, uint16_t offset, uint8_t val) {
    if (!cart->chr || !cart->chr_is_ram || cart->chr_size < 0x1000) return;
    uint32_t num_4k = (uint32_t)(cart->chr_size / 0x1000);
    uint32_t base   = (bank % num_4k) * 0x1000;
    cart->chr[base + (offset & 0x0FFF)] = val;
}

// ----------------------

// latch update function that handles the cases where the latch needs to point to a new chr bank
static void update_latches(Mapper9State *st, uint16_t addr) {
    if ((addr & 0x1FF8) == 0x0FD8) {
        st->latch_0 = 0xFD;
    } else if ((addr & 0x1FF8) == 0x0FE8) {
        st->latch_0 = 0xFE;
    } else if ((addr & 0x1FF8) == 0x1FD8) {
        st->latch_1 = 0xFD;
    } else if ((addr & 0x1FF8) == 0x1FE8) {
        st->latch_1 = 0xFE;
    }
}

static bool mapper9_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    Mapper9State *st = (Mapper9State *)m->state;

    // $6000-$7FFF PRG RAM
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (cart->prg_ram && cart->prg_ram_size > 0) {
            *out = cart->prg_ram[addr - 0x6000];
            return true;
        }
        return false;
    }

    // $8000-$9FFF switchable 8KB bank
    if (addr >= 0x8000 && addr <= 0x9FFF) {
        *out = prg_read_8k(cart, st->prg_bank, addr - 0x8000);
        return true;
    }

    // $A000-$BFFF fixed to third-to-last bank (N-3)
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        uint8_t bank = (uint8_t)(num_prg_8k(cart) - 3);
        *out = prg_read_8k(cart, bank, addr - 0xA000);
        return true;
    }

    // $C000-$DFFF fixed to second-to-last bank (N-2)
    if (addr >= 0xC000 && addr <= 0xDFFF) {
        uint8_t bank = (uint8_t)(num_prg_8k(cart) - 2);
        *out = prg_read_8k(cart, bank, addr - 0xC000);
        return true;
    }

    // $E000-$FFFF fixed to last bank (N-1)
    if (addr >= 0xE000) {
        uint8_t bank = (uint8_t)(num_prg_8k(cart) - 1);
        *out = prg_read_8k(cart, bank, addr - 0xE000);
        return true;
    }

    return false;
}

static bool mapper9_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    Mapper9State *st = (Mapper9State *)m->state;

    // $6000-$7FFF: PRG RAM write
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (cart->prg_ram && cart->prg_ram_size > 0) {
            cart->prg_ram[addr - 0x6000] = value;
            return true;
        }
        return false;
    }

    // only the low 5 bits of value are used.
    // $8000-$9FFF has no register so writes there are ignored.
    if (addr < 0xA000) return false;

    value &= 0x1F;

    if  (addr <= 0xAFFF) {
        st->prg_bank = value; 
    } else if (addr <= 0xBFFF) { 
        st->chr_bank_0FD = value; 
    } else if (addr <= 0xCFFF) {
        st->chr_bank_0FE = value; 
    } else if (addr <= 0xDFFF) { 
        st->chr_bank_1FD = value; 
    } else if (addr <= 0xEFFF) { 
        st->chr_bank_1FE = value; 
    } else { 
        st->mirroring = value & 0x01; 
    }

    return true;
}

static bool mapper9_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    Mapper9State *st = (Mapper9State *)m->state;

    if (addr > 0x1FFF) return false;

    uint8_t data;

    if (addr < 0x1000) {
        // $0000-$0FFF: bank selected by latch_0
        uint8_t bank = (st->latch_0 == 0xFE) ? st->chr_bank_0FE : st->chr_bank_0FD;
        data = chr_read_4k(cart, bank, addr);
    } else {
        // $1000-$1FFF: bank selected by latch_1
        uint8_t bank = (st->latch_1 == 0xFE) ? st->chr_bank_1FE : st->chr_bank_1FD;
        data = chr_read_4k(cart, bank, addr);
    }

    *out = data;

    // latch updates happen AFTER returning the byte
    update_latches(st, addr);

    return true;
}

static bool mapper9_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    Mapper9State *st = (Mapper9State *)m->state;

    if (addr > 0x1FFF || !cart->chr_is_ram) return false;

    if (addr < 0x1000) {
        uint8_t bank = (st->latch_0 == 0xFE) ? st->chr_bank_0FE : st->chr_bank_0FD;
        chr_write_4k(cart, bank, addr, value);
    } else {
        uint8_t bank = (st->latch_1 == 0xFE) ? st->chr_bank_1FE : st->chr_bank_1FD;
        chr_write_4k(cart, bank, addr, value);
    }

    return true;
}

// mirroring function that handles the situation that the mapping logic between
//      the $F000 register and mapper.h is inverted.
static uint8_t mapper9_get_mirroring(Mapper *m) {
    Mapper9State *st = (Mapper9State *)m->state;
    return st->mirroring ? 0 : 1;
}

static void mapper9_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

// factory function to create a Mapper 9 instance
Mapper *mapper9_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper9State *st = (Mapper9State*)calloc(1, sizeof(Mapper9State));
    if (!st) {
        free(m);
        return NULL;
    }

    // initial behavior
    st->latch_0 = 0xFD;
    st->latch_1 = 0xFD;
    st->prg_bank  = 0;
    st->mirroring = 0;

    // initialize function pointers and state for Mapper 9
    m->mapper_id = 9;
    m->cpu_read  = mapper9_cpu_read;
    m->cpu_write = mapper9_cpu_write;
    m->ppu_read  = mapper9_ppu_read;
    m->ppu_write = mapper9_ppu_write;
    m->get_mirroring = mapper9_get_mirroring;
    m->state     = st;
    m->destroy   = mapper9_destroy;

    return m;
}