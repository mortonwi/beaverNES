#include "cartridge.h"

// Read from cartridge via the CPU address space
bool cart_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;

    // Mapper 0 (NROM)
    // Mapper 0: CPU PRG window is $8000-$FFFF
    if (addr < 0x8000) return false;

    // PRG size is prg_rom_banks 16KiB
    // NROM-128 = 16KiB (mirror it), NROM-256 = 32KiB (no mirror)
    uint32_t prg_size = (uint32_t)cart->prg_size;
    if (prg_size != 16384 && prg_size != 32768) return false;

    // Convert CPU address to PRG offset
    uint32_t offset = (uint32_t)(addr - 0x8000);

    if (prg_size == 16384) {
        offset %= 16384; // mirror $C000-$FFFF onto $8000-$BFFF
    }

    *out = cart->prg[offset];
    return true;
}

// Read from cartridge via the PPU address space
bool cart_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;

    // Mapper 0: PPU CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    if (!cart->chr || cart->chr_size == 0) return false;

    // For Mapper 0, CHR is either 8 KiB ROM or 8 KiB RAM
    if (cart->chr_size != 8192) return false;

    *out = cart->chr[addr];
    return true;
}

// Write to cartridge via the PPU address space (only if CHR is RAM)
bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart) return false;

    // Mapper 0: CHR window is $0000-$1FFF
    if (addr >= 0x2000) return false;

    // Only writable if CHR is RAM
    if (!cart->chr_is_ram) return false;

    if (!cart->chr || cart->chr_size != 8192) return false;

    cart->chr[addr] = value;
    return true;
}
