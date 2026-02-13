// cartridge.c
#include "cartridge.h"
#include "mapper.h"

/* cartridge.c - NES cartridge access helpers
 *
 * At this point, cartridge.c should NOT contain mapper-specific logic.
 * Instead, it delegates CPU/PPU reads/writes to the active mapper that
 * was selected during ROM loading (cart->mapper).
 */

// Read from cartridge via the CPU address space
bool cart_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;
    if (!cart->mapper || !cart->mapper->cpu_read) return false;

    // mapper expects a non-const Cartridge*, but it does not have to mutate state.
    // Cast is safe here as long as mapper implementations don't modify cart for reads.
    return cart->mapper->cpu_read((Cartridge *)cart, addr, out);
}

// Read from cartridge via the PPU address space
bool cart_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out) {
    if (!cart || !out) return false;
    if (!cart->mapper || !cart->mapper->ppu_read) return false;

    return cart->mapper->ppu_read((Cartridge *)cart, addr, out);
}

// Write to cartridge via the PPU address space (CHR-RAM, bank regs, etc.)
bool cart_ppu_write(Cartridge *cart, uint16_t addr, uint8_t value) {
    if (!cart) return false;
    if (!cart->mapper || !cart->mapper->ppu_write) return false;

    return cart->mapper->ppu_write(cart, addr, value);
}
