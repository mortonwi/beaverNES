// rom_loader.h
// Sources:
// https://www.nesdev.org/wiki/INES
// https://www.copetti.org/writings/consoles/nes/

#ifndef ROM_LOADER_H
#define ROM_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Forward declare to avoid circular includes (mapper.h will include rom_loader.h in some layouts)
typedef struct Mapper Mapper;

/*
Size of the iNES header in bytes.
All NES ROM files in the iNES format begin with a fixed 16-byte header
that describes cartridge layout and hardware behavior.
*/
#define INES_HEADER_SIZE 16

typedef struct {
    /*
    This structure holds metadata extracted from the first 16 bytes of the ROM
    file and is used to configure memory mapping and cartridge behavior.
    */

    // Number of 16 KiB PRG-ROM banks (program code/data for the CPU)
    uint8_t prg_rom_banks;

    /*
    Number of 8 KiB CHR-ROM banks.
    CHR-ROM contains graphics data used by the PPU.
    If this value is 0, the cartridge uses CHR-RAM instead.
    */
    uint8_t chr_rom_banks;

    // Flags byte 6: mirroring, trainer presence, four-screen, lower mapper bits
    uint8_t flags6;

    // Flags byte 7: upper mapper bits and format flags
    uint8_t flags7;

    /*
    The mapper controls how ROM banks are mapped into CPU/PPU address space.
    Mapper number is composed of bits from flags6 and flags7.
    */
    uint8_t mapper;

    // True if the ROM contains a 512-byte trainer after the header
    bool has_trainer;

    /*
    Mirroring mode used by the cartridge (affects PPU nametable layout).
    - horizontal (false)
    - vertical (true)
    */
    bool mirroring_vertical;

    // True if the cartridge uses four-screen nametable layout
    bool four_screen;
} INesHeader;

/*
Represents an NES cartridge loaded into memory.
This structure models the physical layout of an NES cartridge and provides
PRG and CHR data to the CPU and PPU during emulation.
*/
typedef struct Cartridge {
    INesHeader header;

    /*
    Active mapper implementation for this cartridge.
    Created in rom_load() and destroyed in rom_free().
    */
    Mapper *mapper;

    // PRG-ROM buffer and size in bytes
    uint8_t *prg;
    size_t prg_size;

    /*
    Pointer to CHR memory (either CHR-ROM or CHR-RAM).
    This memory is used by the PPU for tile and sprite graphics.
    */
    uint8_t *chr;
    size_t chr_size;

    // True when CHR is RAM (chr_rom_banks == 0), false when CHR is ROM
    bool chr_is_ram;

    // Optional 512-byte trainer (present when header.has_trainer == true)
    uint8_t *trainer;
    size_t trainer_size;
} Cartridge;

/*
Loads an NES ROM file from disk and initializes a Cartridge structure.
On success returns true and populates out_cart.
On failure returns false and writes a message to err_msg (if provided).
*/
bool rom_load(const char *path, Cartridge *out_cart, char *err_msg, size_t err_msg_len);

/*
Frees all dynamically allocated memory associated with a Cartridge.
This should be called when unloading a ROM or shutting down the emulator.
*/
void rom_free(Cartridge *cart);

#endif // ROM_LOADER_H
