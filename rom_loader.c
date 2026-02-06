#include "rom_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Helper function to safely write an error message.
*/
static void set_err(char *err, size_t err_len, const char *msg) {
    if (!err || err_len == 0) return;
    snprintf(err, err_len, "%s", msg);
}

/**
 * Parses and validates the 16-byte iNES ROM header.
 *
 * Parameters:
 *  - h: Pointer to the raw 16-byte iNES header read from the ROM file
 *  - out: Output structure populated with parsed header data
 *  - err_msg: Buffer to store a human-readable error message
 *  - err_msg_len: Size of the error message buffer
 *
 * Returns:
 *  - true if the header is valid and successfully parsed
 *  - false if validation fails
 */
static bool parse_ines_header(const uint8_t h[INES_HEADER_SIZE], INesHeader *out,
                             char *err_msg, size_t err_msg_len) {
    // "NES" 0x1A
    if (h[0] != 'N' || h[1] != 'E' || h[2] != 'S' || h[3] != 0x1A) {
        set_err(err_msg, err_msg_len, "Not a valid iNES ROM (bad magic)");
        return false;
    }

    memset(out, 0, sizeof(*out));

    // Read PRG-ROM and CHR-ROM bank counts
    out->prg_rom_banks = h[4];
    out->chr_rom_banks = h[5];

    // Store control flags
    out->flags6 = h[6];
    out->flags7 = h[7];

    // Extract cartridge features from flags
    out->has_trainer = (out->flags6 & 0x04) != 0;
    out->mirroring_vertical = (out->flags6 & 0x01) != 0;
    out->four_screen = (out->flags6 & 0x08) != 0;

    // Combine upper and lower mapper bits from flags 6 and 7
    uint8_t lower = (out->flags6 >> 4) & 0x0F;
    uint8_t upper = (out->flags7 >> 4) & 0x0F;
    out->mapper = (upper << 4) | lower;

    // ROM must contain at least one PRG-ROM bank
    if (out->prg_rom_banks == 0) {
        set_err(err_msg, err_msg_len, "Invalid ROM: 0 PRG banks");
        return false;
    }

    return true;
}

void rom_free(Cartridge *cart) {
    if (!cart) return;
    free(cart->prg);
    free(cart->chr);
    free(cart->trainer);
    memset(cart, 0, sizeof(*cart));
}

// Loads an NES ROM file from disk and initializes a Cartridge structure
bool rom_load(const char *path, Cartridge *out_cart, char *err_msg, size_t err_msg_len) {
    if (!path || !out_cart) {
        set_err(err_msg, err_msg_len, "Invalid args to rom_load");
        return false;
    }

    memset(out_cart, 0, sizeof(*out_cart));

    FILE *f = fopen(path, "rb");
    if (!f) {
        set_err(err_msg, err_msg_len, "Failed to open ROM file");
        return false;
    }

    // Read the 16-byte iNES header
    uint8_t header[INES_HEADER_SIZE];
    if (fread(header, 1, INES_HEADER_SIZE, f) != INES_HEADER_SIZE) {
        fclose(f);
        set_err(err_msg, err_msg_len, "Failed to read iNES header");
        return false;
    }

    // Parse and validate the iNES header
    if (!parse_ines_header(header, &out_cart->header, err_msg, err_msg_len)) {
        fclose(f);
        return false;
    }

    // If the ROM has a trainer, read the 512-byte trainer data
    if (out_cart->header.has_trainer) {
        out_cart->trainer_size = 512;
        out_cart->trainer = (uint8_t*)malloc(out_cart->trainer_size);
        if (!out_cart->trainer) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Out of memory allocating trainer");
            return false;
        }

        // Trainer data is located immediately after the 16-byte header
        if (fread(out_cart->trainer, 1, out_cart->trainer_size, f) != out_cart->trainer_size) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Failed to read trainer");
            rom_free(out_cart);
            return false;
        }
    }

    // --- PRG LOADING ---
    out_cart->prg_size = (size_t)out_cart->header.prg_rom_banks * 16 * 1024;
    out_cart->prg = (uint8_t*)malloc(out_cart->prg_size);
    if (!out_cart->prg) {
        fclose(f);
        set_err(err_msg, err_msg_len, "Out of memory allocating PRG-ROM");
        rom_free(out_cart);
        return false;
    }

    // PRG-ROM data is located immediately after the header and optional trainer
    if (fread(out_cart->prg, 1, out_cart->prg_size, f) != out_cart->prg_size) {
        fclose(f);
        set_err(err_msg, err_msg_len, "Failed to read PRG-ROM");
        rom_free(out_cart);
        return false;
    }

    // --- CHR LOADING ---
    // If CHR banks == 0, the cartridge uses CHR-RAM (allocate 8 KiB).
    // Otherwise, load CHR-ROM from the file.
    if (out_cart->header.chr_rom_banks == 0) {
        // CHR-RAM (8 KiB)
        out_cart->chr_is_ram = true;
        out_cart->chr_size = 8 * 1024;
        out_cart->chr = (uint8_t*)calloc(1, out_cart->chr_size);
        if (!out_cart->chr) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Out of memory allocating CHR-RAM");
            rom_free(out_cart);
            return false;
        }
    } else {
        // CHR-ROM
        out_cart->chr_is_ram = false;
        out_cart->chr_size = (size_t)out_cart->header.chr_rom_banks * 8 * 1024;
        out_cart->chr = (uint8_t*)malloc(out_cart->chr_size);
        if (!out_cart->chr) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Out of memory allocating CHR-ROM");
            rom_free(out_cart);
            return false;
        }

        // CHR-ROM data is located immediately after the PRG-ROM data
        if (fread(out_cart->chr, 1, out_cart->chr_size, f) != out_cart->chr_size) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Failed to read CHR-ROM");
            rom_free(out_cart);
            return false;
        }
    }

    fclose(f);
    return true;
}
