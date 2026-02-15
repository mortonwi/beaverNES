#include "cartridge.h"
#include "mapper.h"

bool cart_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !cart->mapper || !cart->mapper->cpu_read || !out) return false;

    // Delegate CPU read to the mapper's cpu_read function, which implements the cartridge's memory mapping logic.
    return cart->mapper->cpu_read(cart->mapper, (Cartridge*)cart, addr, out);
}

bool cart_cpu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart || !cart->mapper || !cart->mapper->cpu_write) return false;
    return cart->mapper->cpu_write(cart->mapper, cart, addr, value);
}

bool cart_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !cart->mapper || !cart->mapper->ppu_read || !out) return false;
    return cart->mapper->ppu_read(cart->mapper, (Cartridge*)cart, addr, out);
}

bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart || !cart->mapper || !cart->mapper->ppu_write) return false;
    return cart->mapper->ppu_write(cart->mapper, cart, addr, value);
}
