#include "cartridge.h"
#include "mapper.h"

bool cart_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;

    // Handle cartridge PRG-RAM at $6000-$7FFF
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (!cart->prg_ram || cart->prg_ram_size == 0) return false;

        size_t offset = (size_t)(addr - 0x6000);
        if (offset >= cart->prg_ram_size) return false;

        *out = cart->prg_ram[offset];
        return true;
    }

    if (!cart->mapper || !cart->mapper->cpu_read) return false;
    return cart->mapper->cpu_read(cart->mapper, (Cartridge*)cart, addr, out);
}

bool cart_cpu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart) return false;

    // Handle cartridge PRG-RAM at $6000-$7FFF
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        if (!cart->prg_ram || cart->prg_ram_size == 0) return false;

        size_t offset = (size_t)(addr - 0x6000);
        if (offset >= cart->prg_ram_size) return false;

        cart->prg_ram[offset] = value;
        return true;
    }

    if (!cart->mapper || !cart->mapper->cpu_write) return false;
    return cart->mapper->cpu_write(cart->mapper, cart, addr, value);
}

bool cart_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out || !cart->mapper || !cart->mapper->ppu_read) return false;
    return cart->mapper->ppu_read(cart->mapper, (Cartridge*)cart, addr, out);
}

bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart || !cart->mapper || !cart->mapper->ppu_write) return false;
    return cart->mapper->ppu_write(cart->mapper, cart, addr, value);
}