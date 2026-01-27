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
    // NES 0x1A
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

    //ROM must contain at least one PRG-ROM bank
    if (out->prg_rom_banks == 0) {
        set_err(err_msg, err_msg_len, "Invalid ROM: 0 PRG banks");
        return false;
    }

    return true;
}