#include "state.h"

#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "cartridge.h"

#define SAVE_MAGIC "BVNS"
#define SAVE_VERSION 1

typedef struct {
    char magic[4];
    uint32_t version;
} SaveHeader;

bool save_state(const char *path, Bus *bus) {
    if (!path || !bus || !bus->cpu || !bus->mem) {
        return false;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }

    SaveHeader header = {
        {'B','V','N','S'},
        SAVE_VERSION
    };

    fwrite(&header, sizeof(header), 1, f);

    // CPU state
    if (!cpu_save_state(bus->cpu, f)) {
        fclose(f);
        return false;
    }

    // PPU state
    if (!ppu_save_state(f)) {
        fclose(f);
        return false;
    }

    // RAM
    fwrite(bus->mem->data, sizeof(bus->mem->data), 1, f);

    // Cartridge PRG RAM
    if (bus->rom && bus->rom->prg_ram) {
        fwrite(bus->rom->prg_ram,
               bus->rom->prg_ram_size,
               1,
               f);
    }

    fclose(f);
    return true;
}

bool load_state(const char *path, Bus *bus) {
    if (!path || !bus || !bus->cpu || !bus->mem) {
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }

    SaveHeader header;

    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return false;
    }

    if (memcmp(header.magic, SAVE_MAGIC, 4) != 0 ||
        header.version != SAVE_VERSION) {
        fclose(f);
        return false;
    }

    // CPU state
    if (!cpu_load_state(bus->cpu, f)) {
        fclose(f);
        return false;
    }

    // PPU state
    if (!ppu_load_state(f)) {
        fclose(f);
        return false;
    }

    // RAM
    fread(bus->mem->data,
          sizeof(bus->mem->data),
          1,
          f);

    // Cartridge PRG RAM
    if (bus->rom && bus->rom->prg_ram) {
        fread(bus->rom->prg_ram,
              bus->rom->prg_ram_size,
              1,
              f);
    }

    fclose(f);
    return true;
}