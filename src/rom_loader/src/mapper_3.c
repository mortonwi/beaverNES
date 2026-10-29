#include "mapper.h"
#include "rom_loader.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    uint8_t chr_bank;
} Mapper3State;

static bool mapper3_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;
    if (!cart || !out || !cart->prg) return false;

    // Mapper 3 PRG window is $8000-$FFFF
    if (addr < 0x8000) return false;

    const uint32_t prg_size = (uint32_t)cart->prg_size;
    if (prg_size != 16384u && prg_size != 32768u) return false;

    uint32_t offset = (uint32_t)(addr - 0x8000);

    // 16 KiB PRG mirrors into upper half
    if (prg_size == 16384u) {
        offset &= 0x3FFFu;
    }

    *out = cart->prg[offset];
    return true;
}

static bool mapper3_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)cart;
    if (!m) return false;

    // CNROM uses CPU writes in $8000-$FFFF to select CHR bank
    if (addr < 0x8000) return false;

    Mapper3State *state = (Mapper3State *)m->state;
    if (!state) return false;

    state->chr_bank = value;

    // TEMP DEBUG
    FILE *f = fopen("mapper3_debug.txt", "a");
    if (f) {
        fprintf(f, "Mapper3 CHR bank write addr=%04X value=%02X selected=%u\n",
                addr, value, state->chr_bank);
        fclose(f);
    }

    return true;
}

static bool mapper3_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out || !cart->chr) return false;

    // CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    Mapper3State *state = (Mapper3State *)m->state;
    if (!state) return false;

    // Each CHR bank is 8 KiB
    uint32_t bank_count = (uint32_t)cart->chr_size / 0x2000u;
    if (bank_count == 0) return false;

    uint32_t selected_bank = (uint32_t)(state->chr_bank % bank_count);
    uint32_t offset = selected_bank * 0x2000u + (uint32_t)addr;

    if (offset >= (uint32_t)cart->chr_size) return false;

    *out = cart->chr[offset];
    return true;
}

static bool mapper3_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!m || !cart || !cart->chr) return false;

    if (addr >= 0x2000) return false;

    // CNROM is normally CHR-ROM, but allow CHR-RAM if present
    if (!cart->chr_is_ram) return false;

    Mapper3State *state = (Mapper3State *)m->state;
    if (!state) return false;

    uint32_t bank_count = (uint32_t)cart->chr_size / 0x2000u;
    if (bank_count == 0) return false;

    uint32_t selected_bank = (uint32_t)(state->chr_bank % bank_count);
    uint32_t offset = selected_bank * 0x2000u + (uint32_t)addr;

    if (offset >= (uint32_t)cart->chr_size) return false;

    cart->chr[offset] = value;
    return true;
}

static uint8_t mapper3_get_mirroring(Mapper *m) {
    (void)m;
    return 0;
}

static void mapper3_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
}

Mapper *mapper3_create(void) {
    Mapper *m = (Mapper *)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper3State *state = (Mapper3State *)calloc(1, sizeof(Mapper3State));
    if (!state) {
        free(m);
        return NULL;
    }

    state->chr_bank = 0;

    m->mapper_id = 3;
    m->cpu_read = mapper3_cpu_read;
    m->cpu_write = mapper3_cpu_write;
    m->ppu_read = mapper3_ppu_read;
    m->ppu_write = mapper3_ppu_write;
    m->get_mirroring = NULL;
    m->state = state;
    m->destroy = mapper3_destroy;

    return m;
}