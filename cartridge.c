#include "cartridge.h"

/*
cartridge.c - Cartridge access layer.

This file acts as a thin wrapper that routes CPU/PPU reads and writes
through the currently active mapper (cart->mapper).
*/

bool cart_cpu_read(Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;
    if (!cart->mapper || !cart->mapper->cpu_read) return false;

    return cart->mapper->cpu_read(cart->mapper, cart, addr, out);
}

bool cart_cpu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart) return false;
    if (!cart->mapper || !cart->mapper->cpu_write) return false;

    return cart->mapper->cpu_write(cart->mapper, cart, addr, value);
}

bool cart_ppu_read(Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;
    if (!cart->mapper || !cart->mapper->ppu_read) return false;

    return cart->mapper->ppu_read(cart->mapper, cart, addr, out);
}

bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart) return false;
    if (!cart->mapper || !cart->mapper->ppu_write) return false;

    return cart->mapper->ppu_write(cart->mapper, cart, addr, value);
}
