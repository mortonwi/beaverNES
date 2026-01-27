#include "rom_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_err(char *err, size_t err_len, const char *msg) {
    if (!err || err_len == 0) return;
    snprintf(err, err_len, "%s", msg);
}

static bool parse_ines_header(const uint8_t h[INES_HEADER_SIZE], INesHeader *out,
                              char *err_msg, size_t err_msg_len) {
    // NES 0x1A
    if (h[0] != 'N' || h[1] != 'E' || h[2] != 'S' || h[3] != 0x1A) {
        set_err(err_msg, err_msg_len, "Not a valid iNES ROM (bad magic)");
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->prg_rom_banks = h[4];
    out->chr_rom_banks = h[5];
    out->flags6 = h[6];
    out->flags7 = h[7];

    out->has_trainer = (out->flags6 & 0x04) != 0;
    out->mirroring_vertical = (out->flags6 & 0x01) != 0;
    out->four_screen = (out->flags6 & 0x08) != 0;

    uint8_t lower = (out->flags6 >> 4) & 0x0F;
    uint8_t upper = (out->flags7 >> 4) & 0x0F;
    out->mapper = (upper << 4) | lower;

    if (out->prg_rom_banks == 0) {
        set_err(err_msg, err_msg_len, "Invalid ROM: 0 PRG banks");
        return false;
    }

    return true;
}