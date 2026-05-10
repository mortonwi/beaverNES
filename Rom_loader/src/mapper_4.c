// mapper_4.c
// Mapper 4 / MMC3
// Sources:
// https://www.nesdev.org/wiki/MMC3
//
// This implements basic MMC3 PRG/CHR bank switching and mirroring.
// IRQ support is partially stored, but not wired to CPU yet because the current
// Mapper interface has no IRQ signal callback.
//full MMC3 IRQ behavior not ready yet.
#include "mapper.h"
#include "rom_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KB_1  1024u
#define KB_2  (2u * 1024u)
#define KB_8  (8u * 1024u)

typedef struct {
    uint8_t bank_select;
    uint8_t regs[8];

    uint8_t prg_mode;
    uint8_t chr_mode;

    // 0 = horizontal, 1 = vertical
    uint8_t mirroring;

    // IRQ fields are stored for future PPU/CPU integration.
    uint8_t irq_latch;
    uint8_t irq_counter;
    uint8_t prev_a12;
    uint16_t a12_low_cycles;
    bool irq_reload;
    bool irq_enabled;
    bool irq_pending;
} Mapper4State;

static size_t wrap_offset(size_t offset, size_t size) {
    if (size == 0) return 0;
    return offset % size;
}

static uint8_t mapper4_get_mirroring(Mapper *m) {
    Mapper4State *s = (Mapper4State*)m->state;
    return s->mirroring;
}

static size_t mapper4_prg_offset(Cartridge *cart, int slot) {
    Mapper4State *s = (Mapper4State*)cart->mapper->state;

    size_t bank_count = cart->prg_size / KB_8;
    if (bank_count == 0) return 0;

    uint8_t r6 = s->regs[6];
    uint8_t r7 = s->regs[7];

    size_t second_last = (bank_count >= 2) ? bank_count - 2 : 0;
    size_t last        = bank_count - 1;

    size_t bank = 0;

    /*
     * MMC3 CPU map:
     *
     * PRG mode 0:
     * $8000-$9FFF = R6
     * $A000-$BFFF = R7
     * $C000-$DFFF = second-last bank fixed
     * $E000-$FFFF = last bank fixed
     *
     * PRG mode 1:
     * $8000-$9FFF = second-last bank fixed
     * $A000-$BFFF = R7
     * $C000-$DFFF = R6
     * $E000-$FFFF = last bank fixed
     */
    if (s->prg_mode == 0) {
        switch (slot) {
            case 0: bank = r6; break;
            case 1: bank = r7; break;
            case 2: bank = second_last; break;
            default: bank = last; break;
        }
    } else {
        switch (slot) {
            case 0: bank = second_last; break;
            case 1: bank = r7; break;
            case 2: bank = r6; break;
            default: bank = last; break;
        }
    }

    bank %= bank_count;
    return bank * KB_8;
}

static bool mapper4_cpu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;

    if (addr < 0x8000 || addr > 0xFFFF) return false;
    if (!cart || !cart->prg || cart->prg_size == 0 || !out) return false;

    int slot = (addr - 0x8000) / 0x2000;
    size_t base = mapper4_prg_offset(cart, slot);
    size_t offset = base + ((size_t)addr & 0x1FFF);

    *out = cart->prg[wrap_offset(offset, cart->prg_size)];
    return true;
}

static bool mapper4_cpu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)cart;

    Mapper4State *s = (Mapper4State*)m->state;
    if (addr < 0x8000 || addr > 0xFFFF) return false;

    bool even = ((addr & 1u) == 0);

    if (addr >= 0x8000 && addr <= 0x9FFF) {
        if (even) {
            /*
             * $8000 even: bank select
             * bits 0-2 = target register R0-R7
             * bit 6    = PRG bank mode
             * bit 7    = CHR bank mode
             */
            s->bank_select = value & 0x07;
            s->prg_mode = (value >> 6) & 1;
            s->chr_mode = (value >> 7) & 1;
        } else {
            /*
             * $8001 odd: bank data
             * Writes value into whichever R register was selected by $8000.
             */
            s->regs[s->bank_select] = value;
        }
        return true;
    }

    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (even) {
            /*
             * $A000 even: nametable mirroring
             * 0 = horizontal, 1 = vertical
             */
            s->mirroring = value & 1;
        } else {
            /*
             * $A001 odd: PRG-RAM protect.
             * Ignored for now because cartridge.c already handles PRG-RAM directly.
             */
        }
        return true;
    }

    if (addr >= 0xC000 && addr <= 0xDFFF) {
        if (even) {
            // $C000 even: IRQ latch
            s->irq_latch = value;
            printf("MMC3 IRQ LATCH=%u\n", value);
        } else {
            // $C001 odd: IRQ reload
            s->irq_reload = true;
            printf("MMC3 IRQ RELOAD\n");
        }
        return true;
    }

    if (addr >= 0xE000 && addr <= 0xFFFF) {
        if (even) {
            // $E000 even: disable IRQ
            s->irq_enabled = false;
            s->irq_pending = false;
            printf("MMC3 IRQ DISABLE\n");
        } else {
            // $E001 odd: enable IRQ
            s->irq_enabled = true;
            printf("MMC3 IRQ ENABLE\n");
        }
        return true;
    }

    return false;
}

static size_t mapper4_chr_offset(Cartridge *cart, uint16_t addr) {
    Mapper4State *s = (Mapper4State*)cart->mapper->state;

    size_t chr_size = cart->chr_size;
    if (chr_size == 0) return 0;

    uint8_t r0 = s->regs[0] & 0xFE; // 2KB bank, force even
    uint8_t r1 = s->regs[1] & 0xFE; // 2KB bank, force even
    uint8_t r2 = s->regs[2];
    uint8_t r3 = s->regs[3];
    uint8_t r4 = s->regs[4];
    uint8_t r5 = s->regs[5];

    size_t offset = 0;

    /*
     * MMC3 CHR mode 0:
     * $0000-$07FF = R0 as 2KB
     * $0800-$0FFF = R1 as 2KB
     * $1000-$13FF = R2 as 1KB
     * $1400-$17FF = R3 as 1KB
     * $1800-$1BFF = R4 as 1KB
     * $1C00-$1FFF = R5 as 1KB
     *
     * CHR mode 1 swaps the two 2KB banks with the four 1KB banks.
     */
    if (s->chr_mode == 0) {
        if (addr < 0x0800) {
            offset = ((size_t)r0 * KB_1) + (addr & 0x07FF);
        } else if (addr < 0x1000) {
            offset = ((size_t)r1 * KB_1) + (addr & 0x07FF);
        } else if (addr < 0x1400) {
            offset = ((size_t)r2 * KB_1) + (addr & 0x03FF);
        } else if (addr < 0x1800) {
            offset = ((size_t)r3 * KB_1) + (addr & 0x03FF);
        } else if (addr < 0x1C00) {
            offset = ((size_t)r4 * KB_1) + (addr & 0x03FF);
        } else {
            offset = ((size_t)r5 * KB_1) + (addr & 0x03FF);
        }
    } else {
        if (addr < 0x0400) {
            offset = ((size_t)r2 * KB_1) + (addr & 0x03FF);
        } else if (addr < 0x0800) {
            offset = ((size_t)r3 * KB_1) + (addr & 0x03FF);
        } else if (addr < 0x0C00) {
            offset = ((size_t)r4 * KB_1) + (addr & 0x03FF);
        } else if (addr < 0x1000) {
            offset = ((size_t)r5 * KB_1) + (addr & 0x03FF);
        } else if (addr < 0x1800) {
            offset = ((size_t)r0 * KB_1) + (addr & 0x07FF);
        } else {
            offset = ((size_t)r1 * KB_1) + (addr & 0x07FF);
        }
    }

    return wrap_offset(offset, chr_size);
}

static bool mapper4_ppu_read(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out) {
    (void)m;

    if (!cart || !cart->chr || !out) return false;
    if (addr > 0x1FFF) return false;

    size_t offset = mapper4_chr_offset(cart, addr);
    *out = cart->chr[offset];
    return true;
}

static bool mapper4_ppu_write(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value) {
    (void)m;

    if (!cart || !cart->chr) return false;
    if (addr > 0x1FFF) return false;

    // Only CHR-RAM cartridges allow PPU writes to pattern memory.
    if (!cart->chr_is_ram) return false;

    size_t offset = mapper4_chr_offset(cart, addr);
    cart->chr[offset] = value;
    return true;
}

static void mapper4_notify_a12(Mapper *m, Cartridge *cart, uint16_t addr) {
    (void)cart;

    Mapper4State *s = (Mapper4State*)m->state;
    uint8_t a12 = (addr & 0x1000) ? 1 : 0;

    if (!a12) {
        if (s->a12_low_cycles < 1000) {
            s->a12_low_cycles++;
        }
    }

    if (a12 && !s->prev_a12 && s->a12_low_cycles >= 8) {
        if (s->irq_counter == 0 || s->irq_reload) {
            s->irq_counter = s->irq_latch;
            s->irq_reload = false;
        } else {
            s->irq_counter--;
        }

        if (s->irq_counter == 0 && s->irq_enabled) {
            s->irq_pending = true;
        }

        s->a12_low_cycles = 0;

        printf("MMC3 IRQ clock latch=%u counter=%u reload=%d enabled=%d pending=%d\n", s->irq_latch, s->irq_counter, s->irq_reload, s->irq_enabled, s->irq_pending);
    }

    s->prev_a12 = a12;
}

static bool mapper4_irq_pending(Mapper *m) {
    Mapper4State *s = (Mapper4State*)m->state;
    return s->irq_pending;
}

static void mapper4_clear_irq(Mapper *m) {
    Mapper4State *s = (Mapper4State*)m->state;
    s->irq_pending = false;
}

static void mapper4_destroy(Mapper *m) {
    if (!m) return;
    free(m->state);
    m->state = NULL;
}

Mapper *mapper4_create(void) {
    Mapper *m = (Mapper*)calloc(1, sizeof(Mapper));
    if (!m) return NULL;

    Mapper4State *s = (Mapper4State*)calloc(1, sizeof(Mapper4State));
    if (!s) {
        free(m);
        return NULL;
    }

    /*
     * Default mirroring does not know the ROM header here.
     * The game usually writes $A000 to set MMC3 mirroring.
     */
    s->mirroring = 0;
    m->mapper_id = 4;
    m->notify_a12 = mapper4_notify_a12;
    m->irq_pending = mapper4_irq_pending;
    m->clear_irq = mapper4_clear_irq;
    m->cpu_read = mapper4_cpu_read;
    m->cpu_write = mapper4_cpu_write;
    m->ppu_read = mapper4_ppu_read;
    m->ppu_write = mapper4_ppu_write;
    m->get_mirroring = mapper4_get_mirroring;
    m->state = s;
    m->destroy = mapper4_destroy;

    return m;
}