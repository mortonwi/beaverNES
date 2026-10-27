// mapper_7.c
// Mapper 7 / AxROM
// - 32KB PRG-ROM bank switching
// - CHR-RAM support
// - One-screen mirroring controlled by bit 4 of CPU writes

#include "mapper.h"
#include "cartridge.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t prg_bank;
    uint8_t mirroring;
} Mapper7State;

static Mapper7State *mapper7_state(Mapper *m) {
    return (Mapper7State *)m->state;
}

static void mapper7_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

static bool mapper7_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!m || !cart || !out || !cart->prg) return false;
    if (addr < 0x8000) return false;

    Mapper7State *s = mapper7_state(m);

    size_t bank_count_32k = cart->prg_size / 0x8000;
    if (bank_count_32k == 0) return false;

    size_t bank = s->prg_bank % bank_count_32k;
    size_t offset = bank * 0x8000 + (addr - 0x8000);

    if (offset >= cart->prg_size) return false;

    *out = cart->prg[offset];
    return true;
}

static bool mapper7_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)cart;

    if (!m) return false;
    if (addr < 0x8000) return false;

    Mapper7State *s = mapper7_state(m);

    // Bits 0-2 select 32KB PRG bank
    s->prg_bank = value & 0x07;

    // Bit 4 selects one-screen mirroring
    // 0 = one-screen lower, 1 = one-screen upper
    s->mirroring = (value & 0x10) ? 3 : 2;

    return true;
}

static bool mapper7_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;

    if (!cart || !out || !cart->chr) return false;
    if (addr >= 0x2000) return false;

    size_t offset = addr % cart->chr_size;
    *out = cart->chr[offset];

    return true;
}

static bool mapper7_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m;

    if (!cart || !cart->chr) return false;
    if (addr >= 0x2000) return false;
    if (!cart->chr_is_ram) return false;

    size_t offset = addr % cart->chr_size;
    cart->chr[offset] = value;

    return true;
}

static uint8_t mapper7_get_mirroring(Mapper *m) {
    Mapper7State *s = mapper7_state(m);
    return s->mirroring;
}

Mapper *mapper7_create(void) {
    Mapper *m = calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper7State *state = calloc(1, sizeof(Mapper7State));
    if (!state) {
        free(m);
        return NULL;
    }

    state->prg_bank = 0;
    state->mirroring = 2;

    m->mapper_id = 7;
    m->cpu_read = mapper7_cpu_read;
    m->cpu_write = mapper7_cpu_write;
    m->ppu_read = mapper7_ppu_read;
    m->ppu_write = mapper7_ppu_write;
    m->get_mirroring = mapper7_get_mirroring;
    m->state = state;
    m->destroy = mapper7_destroy;

    return m;
}