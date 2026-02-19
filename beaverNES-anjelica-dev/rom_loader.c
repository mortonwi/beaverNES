// rom_loader.c
// Sources:
// https://www.nesdev.org/wiki/INES
// https://www.copetti.org/writings/consoles/nes/

#include "rom_loader.h"
#include "mapper.h"   // mapper_create / mapper_destroy

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_err(char *err, size_t err_len, const char *msg) {
    if (!err || err_len == 0) return;
    snprintf(err, err_len, "%s", msg);
}

static bool parse_ines_header(const uint8_t h[INES_HEADER_SIZE],
                              INesHeader *out,
                              char *err_msg,
                              size_t err_msg_len) {
    // iNES magic: "NES" 0x1A
    if (h[0] != 'N' || h[1] != 'E' || h[2] != 'S' || h[3] != 0x1A) {
        set_err(err_msg, err_msg_len, "Not a valid iNES ROM (bad magic)");
        return false;
    }

    memset(out, 0, sizeof(*out));

    out->prg_rom_banks = h[4]; // 16 KiB units
    out->chr_rom_banks = h[5]; // 8 KiB units
    out->flags6 = h[6];
    out->flags7 = h[7];

    out->has_trainer = (out->flags6 & 0x04) != 0;
    out->mirroring_vertical = (out->flags6 & 0x01) != 0;
    out->four_screen = (out->flags6 & 0x08) != 0;

    // Mapper number = high nibble of flags7 + high nibble of flags6
    uint8_t lower = (out->flags6 >> 4) & 0x0F;
    uint8_t upper = (out->flags7 >> 4) & 0x0F;
    out->mapper = (upper << 4) | lower;

    if (out->prg_rom_banks == 0) {
        set_err(err_msg, err_msg_len, "Invalid ROM: 0 PRG banks");
        return false;
    }

    return true;
}

void rom_free(Cartridge *cart) {
    if (!cart) return;

    if (cart->mapper) {
        mapper_destroy(cart->mapper);
        cart->mapper = NULL;
    }

    free(cart->prg);
    free(cart->chr);
    free(cart->trainer);

    memset(cart, 0, sizeof(*cart));
}

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

    //Read & parse header
    uint8_t header[INES_HEADER_SIZE];
    if (fread(header, 1, INES_HEADER_SIZE, f) != INES_HEADER_SIZE) {
        fclose(f);
        set_err(err_msg, err_msg_len, "Failed to read iNES header");
        return false;
    }

    if (!parse_ines_header(header, &out_cart->header, err_msg, err_msg_len)) {
        fclose(f);
        return false;
    }

    //Optional trainer (512 bytes)
    if (out_cart->header.has_trainer) {
        out_cart->trainer_size = 512;
        out_cart->trainer = (uint8_t*)malloc(out_cart->trainer_size);
        if (!out_cart->trainer) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Out of memory allocating trainer");
            return false;
        }

        if (fread(out_cart->trainer, 1, out_cart->trainer_size, f) != out_cart->trainer_size) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Failed to read trainer");
            rom_free(out_cart);
            return false;
        }
    }

    //PRG-ROM
    out_cart->prg_size = (size_t)out_cart->header.prg_rom_banks * 16u * 1024u;
    out_cart->prg = (uint8_t*)malloc(out_cart->prg_size);
    if (!out_cart->prg) {
        fclose(f);
        set_err(err_msg, err_msg_len, "Out of memory allocating PRG-ROM");
        rom_free(out_cart);
        return false;
    }

    if (fread(out_cart->prg, 1, out_cart->prg_size, f) != out_cart->prg_size) {
        fclose(f);
        set_err(err_msg, err_msg_len, "Failed to read PRG-ROM");
        rom_free(out_cart);
        return false;
    }

    //CHR-ROM or CHR-RAM
    if (out_cart->header.chr_rom_banks == 0) {
        // CHR-RAM: allocate 8 KiB and zero it
        out_cart->chr_is_ram = true;
        out_cart->chr_size = 8u * 1024u;
        out_cart->chr = (uint8_t*)calloc(1, out_cart->chr_size);
        if (!out_cart->chr) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Out of memory allocating CHR-RAM");
            rom_free(out_cart);
            return false;
        }
    } else {
        out_cart->chr_is_ram = false;
        out_cart->chr_size = (size_t)out_cart->header.chr_rom_banks * 8u * 1024u;
        out_cart->chr = (uint8_t*)malloc(out_cart->chr_size);
        if (!out_cart->chr) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Out of memory allocating CHR-ROM");
            rom_free(out_cart);
            return false;
        }

        if (fread(out_cart->chr, 1, out_cart->chr_size, f) != out_cart->chr_size) {
            fclose(f);
            set_err(err_msg, err_msg_len, "Failed to read CHR-ROM");
            rom_free(out_cart);
            return false;
        }
    }

    fclose(f);

    //Create mapper after ROM data is loaded
    out_cart->mapper = mapper_create(out_cart->header.mapper);
    if (!out_cart->mapper) {
        set_err(err_msg, err_msg_len, "Unsupported mapper (no implementation)");
        rom_free(out_cart);
        return false;
    }

    return true;
}
