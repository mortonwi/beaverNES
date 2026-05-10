// mapper.h
#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include <stdbool.h>

// Forward declare Cartridge to avoid circular includes.
typedef struct Cartridge Cartridge;

typedef struct Mapper Mapper;

/*
 * Mapper interface:
 * A mapper defines how CPU/PPU address space maps to cartridge PRG/CHR memory.
 * Different mappers implement bank switching and other cartridge hardware logic.
 */
struct Mapper {
    uint8_t mapper_id;

    // CPU bus access
    bool (*cpu_read)(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out);
    bool (*cpu_write)(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value);

    // PPU bus access
    bool (*ppu_read)(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t *out);
    bool (*ppu_write)(Mapper *m, Cartridge *cart, uint16_t addr, uint8_t value);

    //mirroring query (0 = horizontal, 1 = vertical)
    uint8_t (*get_mirroring)(Mapper *m);

       // MMC3 IRQ/Mapper 4 scanline support
    void (*notify_a12)(Mapper *m, Cartridge *cart, uint16_t addr);
    bool (*irq_pending)(Mapper *m);
    void (*clear_irq)(Mapper *m);

    void *state;

    void (*destroy)(Mapper *m);
};

/*
 * Create a mapper implementation based on mapper_id.
 * Returns a heap-allocated Mapper on success, or NULL if unsupported.
 */
Mapper *mapper_create(uint8_t mapper_id);

/*
 * Destroy a mapper created with mapper_create().
 */
void mapper_destroy(Mapper *m);

#endif // MAPPER_H