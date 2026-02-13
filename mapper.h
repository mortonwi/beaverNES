// mapper.h
#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include <stdbool.h>

// Forward declare Cartridge to avoid circular includes.
typedef struct Cartridge Cartridge;

/*
 * Mapper interface:
 * A mapper defines how CPU/PPU address space maps to cartridge PRG/CHR memory.
 * Different mappers implement bank switching and other cartridge hardware logic.
 */
typedef struct Mapper {
    uint8_t mapper_id;

    // CPU bus access (typically PRG-ROM/PRG-RAM in $8000-$FFFF)
    bool (*cpu_read)(Cartridge *cart, uint16_t addr, uint8_t *out);
    bool (*cpu_write)(Cartridge *cart, uint16_t addr, uint8_t value);

    // PPU bus access (typically CHR-ROM/CHR-RAM in $0000-$1FFF)
    bool (*ppu_read)(Cartridge *cart, uint16_t addr, uint8_t *out);
    bool (*ppu_write)(Cartridge *cart, uint16_t addr, uint8_t value);

    // Optional mapper-specific state (bank registers, etc.)
    void *state;

    // Optional cleanup hook for state
    void (*destroy)(struct Mapper *m);
} Mapper;

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
