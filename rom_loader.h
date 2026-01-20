// Sources: 
// https://www.nesdev.org/wiki/INES
// https://www.copetti.org/writings/consoles/nes/


#ifndef ROM_LOADER_H
#define ROM_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
Size of the iNES header in bytes
All NES ROM files in the iNES format begin with a fixed 16-byte header
that describes cartridge layout and harware behavior
*/
#define INES_HEADER_SIZE 16

typedef struct {
    /*
    This structure holds metadata extracted from the first 16 bytes of the ROM
    file and is used to configure memory mapping and cartridge behavior.
    */
    uint8_t prg_rom_banks;      // 16 KiB units
    /*
    Number of 8 KiB CHR-ROM banks
    CHR-ROM contains graphics data used by the PPU. 
    If this value is 0, CHR-RAM is used instead
    */
    uint8_t chr_rom_banks;      // 8 KiB units
    /*
    Contains mirroring mode, trainer presence, and lower mapper bits
    */
    uint8_t flags6;
    /*
    Contains upper mapper bits and format flags
    */
    uint8_t flags7;
    /*
    The mapper controls how ROM banks are mapped into CPU/PPU address space
    */

    uint8_t mapper;
    bool has_trainer;
    /*
    Mirroring mode used by the cartridge.
    */
    bool mirroring_vertical;    // 0 = horizontal, 1 = vertical
    bool four_screen;
} INesHeader;

/* 
Represents an NES cartridge laoded into memory.
This structure models the physical layout of an NES cartridge and provides
PRG and CHR data to the CPU and PPU during emulation
*/

typedef struct {
    INesHeader header;

    uint8_t *prg;
    size_t prg_size;
    /*
    Pointer to CHR-ROM or CHR-RAM data.
    This memory is used by the PPU for tile and sprite graphics
    */

    uint8_t *chr;
    size_t chr_size;

    uint8_t *trainer;   // optional 512 bytes
    size_t trainer_size;
} Cartridge;

/*
Loads an NES ROM file from disk and initializes a Cartridge structure
*/

bool rom_load(const char *path, Cartridge *out_cart, char *err_msg, size_t err_msg_len);

/*
Frees all dynamically allocated memory associated with a Cartridge.
This should be called when unloading a ROM or shutting downt he emulator
*/
void rom_free(Cartridge *cart);

#endif
