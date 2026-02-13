// mapper.h
#ifndef MAPPER_H
#define MAPPER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Cartridge Cartridge;

/*
 * Mapper interface:
 * Each mapper provides functions that translate CPU/PPU bus reads/writes
 * into PRG/CHR memory accesses for the specific cartridge hardware.
 */
typedef struct Mapper {
    uint8_t mapper_id;

    // CPU bus: $8000-$FFFF maps to PRG-ROM (or PRG-RAM)
    bool (*cpu_read)(Cartridge *cart, uint16_t addr, uint8_t *out);
    bool (*cpu_write)(Cartridge *cart, uint16_t addr, uint8_t value);

    // PPU bus: $0000-$1FFF maps to CHR-ROM/CHR-RAM
    bool (*ppu_read)(Cartridge *cart, uint16_t addr, uint8_t *out);
    bool (*ppu_write)(Cartridge *cart, uint16_t addr, uint8_t value);

    void *state;

    void (*destroy)(struct Mapper *m);
} Mapper;

#endif
