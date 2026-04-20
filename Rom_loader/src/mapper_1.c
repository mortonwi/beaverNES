// mapper_1.c
// MMC1 (Mapper 1) – uses a shift register to control PRG/CHR bank switching

#include "mapper.h"
#include "cartridge.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Holds all runtime state for Mapper 1
typedef struct {
    uint8_t shift_reg;
    uint8_t control;
    uint8_t chr_bank0;
    uint8_t chr_bank1;
    uint8_t prg_bank;

    // Used for MMC1 consecutive-cycle write ignore behavior
    uint64_t last_write_cycle;
    bool last_write_valid;
} Mapper1State;

static Mapper1State *mapper1_state(Mapper *m) {
    return (Mapper1State *)m->state;
}

// Cleanup mapper state
static void mapper1_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

// Handles CPU reads from PRG ROM ($8000-$FFFF)
static bool mapper1_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out) return false;
    if (addr < 0x8000) return false;

    Mapper1State *s = mapper1_state(m);

    size_t prg_banks_16k = cart->prg_size / 0x4000;
    if (prg_banks_16k == 0) return false;

    uint8_t prg_mode = (s->control >> 2) & 0x03;
    uint8_t bank = s->prg_bank & 0x0F;

    size_t mapped = 0;

    // 32 KB mode
    if (prg_mode == 0 || prg_mode == 1) {
        size_t bank32 = (bank & 0x0E) >> 1;
        mapped = bank32 * 0x8000 + (addr - 0x8000);
    }
    // Fix first bank at $8000, switch bank at $C000
    else if (prg_mode == 2) {
        if (addr < 0xC000) {
            mapped = addr - 0x8000;
        } else {
            mapped = (size_t)bank * 0x4000 + (addr - 0xC000);
        }
    }
    // Switch bank at $8000, fix last bank at $C000
    else {
        if (addr < 0xC000) {
            mapped = (size_t)bank * 0x4000 + (addr - 0x8000);
        } else {
            mapped = (prg_banks_16k - 1) * 0x4000 + (addr - 0xC000);
        }
    }

    mapped %= cart->prg_size;
    *out = cart->prg[mapped];
    return true;
}

// MMC1 uses serial writes (1 bit at a time) into a shift register
static bool mapper1_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!m || !cart) return false;
    if (addr < 0x8000) return false;

    Mapper1State *s = mapper1_state(m);

    // MMC1 ignores data writes that happen on consecutive CPU cycles.
    // Reset writes (bit 7 set) must still be honored.
    bool consecutive =
        s->last_write_valid &&
        (cart->cpu_cycle == s->last_write_cycle + 1);

    s->last_write_cycle = cart->cpu_cycle;
    s->last_write_valid = true;

    // Reset shift register and force PRG mode to fixed-last-bank
    if (value & 0x80) {
        s->shift_reg = 0x10;
        s->control |= 0x0C;
        return true;
    }

    // Ignore consecutive-cycle data writes
    if (consecutive) {
        return true;
    }

    bool complete = (s->shift_reg & 0x01) != 0;

    s->shift_reg >>= 1;
    s->shift_reg |= (value & 0x01) << 4;

    if (complete) {
        uint8_t reg = s->shift_reg & 0x1F;
        s->shift_reg = 0x10;

        if (addr <= 0x9FFF) {
            s->control = reg;
        } else if (addr <= 0xBFFF) {
            s->chr_bank0 = reg;
        } else if (addr <= 0xDFFF) {
            s->chr_bank1 = reg;
        } else {
            s->prg_bank = reg;
        }
    }

    return true;
}

// Handles CHR ROM/RAM reads ($0000-$1FFF)
static bool mapper1_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out) return false;
    if (addr >= 0x2000) return false;
    if (!cart->chr || cart->chr_size == 0) return false;

    Mapper1State *s = mapper1_state(m);

    uint8_t chr_mode = (s->control >> 4) & 1;
    size_t mapped = 0;

    // 8 KB mode
    if (chr_mode == 0) {
        size_t bank8 = (s->chr_bank0 & 0x1E) >> 1;
        mapped = bank8 * 0x2000 + addr;
    }
    // 4 KB mode
    else {
        if (addr < 0x1000) {
            mapped = (size_t)s->chr_bank0 * 0x1000 + addr;
        } else {
            mapped = (size_t)s->chr_bank1 * 0x1000 + (addr - 0x1000);
        }
    }

    mapped %= cart->chr_size;
    *out = cart->chr[mapped];
    return true;
}

// Only valid if cartridge uses CHR RAM (not ROM)
static bool mapper1_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!m || !cart) return false;
    if (addr >= 0x2000) return false;
    if (!cart->chr_is_ram) return false;
    if (!cart->chr || cart->chr_size == 0) return false;

    Mapper1State *s = mapper1_state(m);

    uint8_t chr_mode = (s->control >> 4) & 1;
    size_t mapped = 0;

    if (chr_mode == 0) {
        size_t bank8 = (s->chr_bank0 & 0x1E) >> 1;
        mapped = bank8 * 0x2000 + addr;
    } else {
        if (addr < 0x1000) {
            mapped = (size_t)s->chr_bank0 * 0x1000 + addr;
        } else {
            mapped = (size_t)s->chr_bank1 * 0x1000 + (addr - 0x1000);
        }
    }

    mapped %= cart->chr_size;
    cart->chr[mapped] = value;
    return true;
}

//return mirroring from control register
static uint8_t mapper1_get_mirroring(Mapper *m) {
    Mapper1State *s = mapper1_state(m);

    // control bits 0–1
    uint8_t mode = s->control & 0x03;

    switch (mode) {
        case 0: return 0; 
        case 1: return 0;
        case 2: return 1;
        case 3: return 0;
    }

    return 0;
}

// Initializes mapper and default state
Mapper *mapper1_create(void) {
    Mapper *m = calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper1State *state = calloc(1, sizeof(Mapper1State));
    if (!state) {
        free(m);
        return NULL;

        m->get_mirroring = mapper1_get_mirroring;
    }

    // Default MMC1 power-on state
    state->shift_reg = 0x10;
    state->control   = 0x0C;
    state->chr_bank0 = 0;
    state->chr_bank1 = 0;
    state->prg_bank  = 0;
    state->last_write_cycle = 0;
    state->last_write_valid = false;

    m->mapper_id = 1;
    m->cpu_read  = mapper1_cpu_read;
    m->cpu_write = mapper1_cpu_write;
    m->ppu_read  = mapper1_ppu_read;
    m->ppu_write = mapper1_ppu_write;
    m->state     = state;
    m->destroy   = mapper1_destroy;

    return m;
}