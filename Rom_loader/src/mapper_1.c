// mapper_1.c
// MMC1 (Mapper 1)

#include "mapper.h"
#include "cartridge.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t shift_reg;
    uint8_t control;
    uint8_t chr_bank0;
    uint8_t chr_bank1;
    uint8_t prg_bank;

    uint64_t last_write_cycle;
    bool last_write_valid;
} Mapper1State;

static Mapper1State *mapper1_state(Mapper *m) {
    return (Mapper1State *)m->state;
}

static void mapper1_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

static bool mapper1_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out || !cart->prg) return false;
    if (addr < 0x8000) return false;

    Mapper1State *s = mapper1_state(m);

    size_t prg_banks_16k = cart->prg_size / 0x4000;
    if (prg_banks_16k == 0) return false;

    uint8_t prg_mode = (s->control >> 2) & 0x03;
    uint8_t bank = s->prg_bank & 0x0F;

    size_t mapped = 0;

    if (prg_mode == 0 || prg_mode == 1) {
        size_t bank32 = (bank & 0x0E) >> 1;
        mapped = bank32 * 0x8000 + (addr - 0x8000);
    }
    else if (prg_mode == 2) {
        if (addr < 0xC000) {
            mapped = addr - 0x8000;
        } else {
            mapped = (size_t)bank * 0x4000 + (addr - 0xC000);
        }
    }
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

static bool mapper1_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!m || !cart) return false;
    if (addr < 0x8000) return false;

    Mapper1State *s = mapper1_state(m);

    bool consecutive =
        s->last_write_valid &&
        (cart->cpu_cycle == s->last_write_cycle + 1);

    s->last_write_cycle = cart->cpu_cycle;
    s->last_write_valid = true;

    if (value & 0x80) {
        s->shift_reg = 0x10;
        s->control |= 0x0C;
        return true;
    }

    if (consecutive) {
        return true;
    }

    bool complete = (s->shift_reg & 0x01) != 0;

    s->shift_reg >>= 1;
    s->shift_reg |= (uint8_t)((value & 0x01) << 4);

    if (complete) {
        uint8_t reg = s->shift_reg & 0x1F;
        s->shift_reg = 0x10;

        if (addr <= 0x9FFF) {
            s->control = reg;
        }
        else if (addr <= 0xBFFF) {
            s->chr_bank0 = reg;
        }
        else if (addr <= 0xDFFF) {
            s->chr_bank1 = reg;
        }
        else {
            s->prg_bank = reg;
        }

        // TEMP DEBUG
        FILE *f = fopen("mapper1_debug.txt", "a");
        if (f) {
            fprintf(f,
                "MMC1 write addr=%04X reg=%02X control=%02X chr0=%02X chr1=%02X prg=%02X\n",
                addr, reg, s->control, s->chr_bank0, s->chr_bank1, s->prg_bank
            );
            fclose(f);
        }
    }

    return true;
}

static bool mapper1_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out) return false;
    if (addr >= 0x2000) return false;
    if (!cart->chr || cart->chr_size == 0) return false;

    Mapper1State *s = mapper1_state(m);

    uint8_t chr_mode = (s->control >> 4) & 0x01;
    size_t mapped = 0;

    if (chr_mode == 0) {
        // 8KB CHR mode: ignore low bit of chr_bank0
        size_t bank8 = (size_t)(s->chr_bank0 & 0x1E) >> 1;
        mapped = bank8 * 0x2000 + addr;
    } else {
        // 4KB CHR mode
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

static bool mapper1_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!m || !cart) return false;
    if (addr >= 0x2000) return false;
    if (!cart->chr_is_ram) return false;
    if (!cart->chr || cart->chr_size == 0) return false;

    Mapper1State *s = mapper1_state(m);

    uint8_t chr_mode = (s->control >> 4) & 0x01;
    size_t mapped = 0;

    if (chr_mode == 0) {
        size_t bank8 = (size_t)(s->chr_bank0 & 0x1E) >> 1;
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

static uint8_t mapper1_get_mirroring(Mapper *m) {
    Mapper1State *s = mapper1_state(m);
    uint8_t mode = s->control & 0x03;

    switch (mode) {
        case 0: return 2; // one-screen lower
        case 1: return 3; // one-screen upper
        case 2: return 1; // vertical
        case 3: return 0; // horizontal
    }

    return 0;
}

Mapper *mapper1_create(void) {
    Mapper *m = calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper1State *state = calloc(1, sizeof(Mapper1State));
    if (!state) {
        free(m);
        return NULL;
    }

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
    m->get_mirroring = mapper1_get_mirroring;
    m->state = state;
    m->destroy = mapper1_destroy;

    return m;
}