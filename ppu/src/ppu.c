/*
PPU prototype (wired to Cartridge/Mapper for CHR access)

Notes:
- CHR ($0000-$1FFF) reads/writes now go through cart_ppu_read/cart_ppu_write.
- Nametable/palette remain internal (as in your teammate’s PPU).
- Mirroring is set from the connected cartridge header (vertical/horizontal). Four-screen not implemented here.

To compile PPU-only tests (no ROM loader):
  gcc -Wall -Wextra -DPPU_TEST ppu.c -o ppu_test

To compile PPU and ROM Loader tests
   gcc -Wall -Wextra -std=c11 -DPPU_ROM_TEST ppu.c rom_loader.c cartridge.c mapper.c mapper_0.c mapper_2.c -o ppu_rom_test.exe

   and then .\ppu_rom_test.exe NameOfROM.nes
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ppu.h"

#define PPU_WIDTH 256
#define PPU_HEIGHT 240


#include "cartridge.h"
#include "rom_loader.h"

//--------------------------------------------------------------------------//

// NES palette: 64 colors (6-bit indices) mapped to RGB values
static const uint32_t nes_palette[64] = {
    0x545454,0x001E74,0x081090,0x300088,0x440064,0x5C0030,0x540400,0x3C1800,
    0x202A00,0x083A00,0x004000,0x003C00,0x00323C,0x000000,0x000000,0x000000,
    0x989698,0x084CC4,0x3032EC,0x5C1EE4,0x8814B0,0xA01464,0x982220,0x783C00,
    0x545A00,0x287200,0x087C00,0x007628,0x006678,0x000000,0x000000,0x000000,
    0xECEEEC,0x4C9AEC,0x787CEC,0xB062EC,0xE454EC,0xEC58B4,0xEC6A64,0xD48820,
    0xA0AA00,0x74C400,0x4CD020,0x38CC6C,0x38B4CC,0x3C3C3C,0x000000,0x000000,
    0xECEEEC,0xA8CCEC,0xBCBCEC,0xD4B2EC,0xECAEEC,0xECAED4,0xECB4B0,0xE4C490,
    0xCCD278,0xB4DE78,0xA8E290,0x98E2B4,0xA0D6E4,0xA0A2A0,0x000000,0x000000
};

// PPU Structure for state and registers
typedef struct {
    uint8_t vram[0x4000];   // legacy placeholder; no longer used for CHR when cart is connected
    uint8_t oam[256];       // Sprite RAM

    // PPU registers
    uint8_t ppuCtrl;        // $2000
    uint8_t ppuMask;        // $2001
    uint8_t ppuStatus;      // $2002
    uint8_t oamAddr;        // $2003
    uint8_t oamData;        // $2004
    uint8_t ppuScroll;      // $2005
    uint8_t ppuAddr;        // $2006
    uint8_t ppuData;        // $2007
    uint8_t oamDMA;         // $4014

    // Internal PPU state
    uint16_t v;             // current VRAM address (15 bits)
    uint16_t t;             // temp VRAM address (15 bits)
    uint8_t  x;             // fine X (3 bits)
    uint8_t  w;             // write toggle
    uint8_t  buffer;        // PPUDATA read buffer

    // Nametable and palette RAM (still internal)
    uint8_t nametable[0x800]; // 2KB internal NT RAM
    uint8_t palette[0x20];    // palette RAM
    uint8_t mirroring;        // 0 = horizontal, 1 = vertical

    // Timing state
    int cycle;        // 0–340
    int scanline;     // -1–260
    int frame;
    uint8_t nmi;      // NMI request flag

    // Background shift registers
    uint16_t bg_shift_pattern_low;
    uint16_t bg_shift_pattern_high;
    uint16_t bg_shift_attr_low;
    uint16_t bg_shift_attr_high;
    uint8_t next_tile_id;
    uint8_t next_tile_attr;
    uint8_t next_tile_lsb;
    uint8_t next_tile_msb;

    uint32_t framebuffer[PPU_WIDTH * PPU_HEIGHT];

    // OAM secondary buffer for sprite evaluation
    uint8_t secondary_oam[32];
    uint8_t sprite_count;

    // Sprite rendering
    uint8_t  sprite_shifter_pattern_low[8];
    uint8_t  sprite_shifter_pattern_high[8];
    uint8_t  sprite_x_counter[8];
    uint8_t  sprite_attr[8];
    uint8_t  sprite_id[8];
} PPU;

static PPU ppu;

//-------Cartridge pointer the PPU uses for CHR accesses (added by Anjelica for ROM Loader integration)
// Global cartridge pointer used for CHR access
static Cartridge *g_cart = NULL;

// power-up state
void ppu_init(void) {
    memset(&ppu, 0, sizeof(PPU));
    ppu.ppuStatus = 0xA0;
    ppu.mirroring = 0; // horizontal by default
}

static uint16_t mirror_nametable_addr(uint16_t addr) {
    // addr expected in $2000–$2FFF
    uint16_t offset = (addr - 0x2000) & 0x0FFF;
    uint16_t table  = offset / 0x400;   // 0–3
    uint16_t index  = offset & 0x3FF;

    if (ppu.mirroring == 1) {
        // vertical: NT0,NT2 | NT1,NT3
        table &= 1;
    } else {
        // horizontal: NT0,NT1 | NT2,NT3
        table >>= 1;
    }

    return (table * 0x400) + index;
}

// --- Cartridge connection (added by Anjelica for ROM Loader integration)---
void ppu_connect_cartridge(Cartridge *cart) {
    g_cart = cart;

    if (cart) {
        // 0 = horizontal, 1 = vertical (matches this PPU's convention)
        ppu.mirroring = cart->header.mirroring_vertical ? 1 : 0;

        // Four-screen mirroring not implemented here; ignore for now
        // if (cart->header.four_screen) { ... }
    }
}

// --- PPU address space access helpers (CHR via cartridge/mapper)(Added by Anjelica for ROM integration ) ---
static uint8_t ppu_mem_read(uint16_t addr) {
    addr &= 0x3FFF;

    // $0000-$1FFF: CHR via cartridge/mapper when connected
    if (addr < 0x2000) {
        uint8_t data = 0;
        if (g_cart && cart_ppu_read(g_cart, addr, &data))
            return data;

        return 0;
    }

    // $2000-$3EFF: nametables
    if (addr < 0x3F00) {
        uint16_t nt = mirror_nametable_addr(addr);
        return ppu.nametable[nt];
    }

    // $3F00-$3FFF: palette
    uint16_t pal_addr = addr & 0x1F;
    // Palette mirroring: $3F10/$14/$18/$1C -> $3F00/$04/$08/$0C
    if ((pal_addr & 0x13) == 0x10) pal_addr &= (uint16_t)~0x10;
    return ppu.palette[pal_addr];
}

static void ppu_mem_write(uint16_t addr, uint8_t value) {
    addr &= 0x3FFF;

    // $0000-$1FFF: CHR via cartridge/mapper when connected (Added by Anjelica for ROM integration)
    if (addr < 0x2000) {
        if (g_cart)
            (void)cart_ppu_write(g_cart, addr, value);
        return;
    }
    // $2000-$3EFF: nametables
    if (addr < 0x3F00) {
        uint16_t nt = mirror_nametable_addr(addr);
        ppu.nametable[nt] = value;
        return;
    }

    // $3F00-$3FFF: palette
    uint16_t pal_addr = addr & 0x1F;
    if ((pal_addr & 0x13) == 0x10) pal_addr &= (uint16_t)~0x10;
    ppu.palette[pal_addr] = value;
}

// Evaluate sprites for current scanline (max 8)
static void evaluate_sprites(void)
{
    ppu.sprite_count = 0;

    // Clear secondary OAM
    memset(ppu.secondary_oam, 0xFF, sizeof(ppu.secondary_oam));

    int sprite_height = (ppu.ppuCtrl & 0x20) ? 16 : 8;

    for (int i = 0; i < 64; i++)
    {
        uint8_t y    = ppu.oam[i * 4 + 0];
        uint8_t tile = ppu.oam[i * 4 + 1];
        uint8_t attr = ppu.oam[i * 4 + 2];
        uint8_t x    = ppu.oam[i * 4 + 3];

        (void)tile; (void)attr; (void)x;

        int row = ppu.scanline - y;

        if (row >= 0 && row < sprite_height)
        {
            if (ppu.sprite_count < 8)
            {
                int idx = ppu.sprite_count * 4;

                ppu.secondary_oam[idx + 0] = y;
                ppu.secondary_oam[idx + 1] = tile;
                ppu.secondary_oam[idx + 2] = attr;
                ppu.secondary_oam[idx + 3] = x;

                ppu.sprite_count++;
            }
            else
            {
                // Sprite overflow flag
                ppu.ppuStatus |= 0x20;
                break;
            }
        }
    }
}

// Fetch sprite pattern data for the next scanline based on sprites selected in secondary OAM.
static void load_sprite_shifters(int next_scanline)
{
    // Clear shifters
    for (int i = 0; i < 8; i++) {
        ppu.sprite_shifter_pattern_low[i]  = 0;
        ppu.sprite_shifter_pattern_high[i] = 0;
        ppu.sprite_x_counter[i]            = 0;
        ppu.sprite_attr[i]                 = 0;
        ppu.sprite_id[i]                   = 0;
    }

    for (int i = 0; i < ppu.sprite_count && i < 8; i++)
    {
        uint8_t spr_y    = ppu.secondary_oam[i * 4 + 0];
        uint8_t spr_id   = ppu.secondary_oam[i * 4 + 1];
        uint8_t spr_attr = ppu.secondary_oam[i * 4 + 2];
        uint8_t spr_x    = ppu.secondary_oam[i * 4 + 3];

        ppu.sprite_x_counter[i] = spr_x;
        ppu.sprite_attr[i]      = spr_attr;
        ppu.sprite_id[i]        = spr_id;

        int row = next_scanline - spr_y;
        uint8_t sprite_height = (ppu.ppuCtrl & 0x20) ? 16 : 8;

        // Vertical flip
        if (spr_attr & 0x80) {
            row = sprite_height - 1 - row;
        }

        uint16_t addr;

        if (ppu.ppuCtrl & 0x20) {
            // 8x16 sprite mode
            uint16_t table = (spr_id & 0x01) ? 0x1000 : 0x0000;
            uint8_t tile_index = spr_id & 0xFE;

            if (row > 7) {
                tile_index++;
                row -= 8;
            }

            addr = table + (uint16_t)(tile_index * 16) + (uint16_t)row;
        } else {
            // 8x8 sprite mode
            uint16_t table = (ppu.ppuCtrl & 0x08) ? 0x1000 : 0x0000;
            addr = table + (uint16_t)(spr_id * 16) + (uint16_t)row;
        }

        uint8_t lo = ppu_mem_read(addr);
        uint8_t hi = ppu_mem_read((uint16_t)(addr + 8));

        // Horizontal flip (bit 6)
        if (spr_attr & 0x40)
        {
            // bit-reverse each byte
            uint8_t rlo = 0, rhi = 0;
            for (int b = 0; b < 8; b++) {
                rlo = (uint8_t)((rlo << 1) | (lo & 1));
                rhi = (uint8_t)((rhi << 1) | (hi & 1));
                lo >>= 1;
                hi >>= 1;
            }
            lo = rlo;
            hi = rhi;
        }

        ppu.sprite_shifter_pattern_low[i]  = lo;
        ppu.sprite_shifter_pattern_high[i] = hi;
    }
}

// Tick sprite x counters / shifters each visible pixel.
static void tick_sprite_shifters(void)
{
    for (int i = 0; i < ppu.sprite_count && i < 8; i++)
    {
        if (ppu.sprite_x_counter[i] > 0) {
            ppu.sprite_x_counter[i]--;
        } else {
            ppu.sprite_shifter_pattern_low[i]  <<= 1;
            ppu.sprite_shifter_pattern_high[i] <<= 1;
        }
    }
}

// PPU register writes (CPU side)
void ppu_write(uint16_t addr, uint8_t value) {
    switch (addr & 0x2007) {
        case 0x2000: // PPUCTRL
            ppu.ppuCtrl = value;
            // Update nametable select bits in temp VRAM address (t)
            ppu.t = (ppu.t & 0xF3FF) | ((uint16_t)(value & 0x03) << 10);
            break;

        case 0x2001: // PPUMASK
            ppu.ppuMask = value;
            break;

        case 0x2003: // OAMADDR
            ppu.oamAddr = value;
            break;

        case 0x2004: // OAMDATA
            ppu.oam[ppu.oamAddr++] = value;
            break;

        case 0x2005: // PPUSCROLL
            if (!ppu.w) {
                // First write: horizontal scroll
                ppu.x = value & 0x07;         // fine X
                ppu.t = (ppu.t & 0x7FE0)      // keep coarse Y + fine Y + NT
                      | ((value >> 3) & 0x1F);// coarse X
                ppu.w = 1;
            } else {
                // Second write: vertical scroll
                ppu.t = (ppu.t & 0x0C1F)            // keep coarse X + NT
                      | ((uint16_t)(value & 0x07) << 12) // fine Y
                      | ((uint16_t)(value & 0xF8) << 2); // coarse Y
                ppu.w = 0;
            }
            break;

        case 0x2006: // PPUADDR
            if (!ppu.w) {
                // First write: high byte (bits 8–13)
                ppu.t = (ppu.t & 0x00FF) | ((uint16_t)(value & 0x3F) << 8);
                ppu.w = 1;
            } else {
                // Second write: low byte, then copy t -> v
                ppu.t = (ppu.t & 0xFF00) | value;
                ppu.v = ppu.t;
                ppu.w = 0;
            }
            break;

        case 0x2007: { // PPUDATA
            uint16_t a = ppu.v & 0x3FFF;
            ppu_mem_write(a, value);
            ppu.v += (ppu.ppuCtrl & 0x04) ? 32 : 1;
            break;
        }
    }
}

uint8_t ppu_read(uint16_t addr) {
    uint8_t result = 0;

    switch (addr & 0x2007) {
        case 0x2002: // PPUSTATUS
            result = ppu.ppuStatus;
            ppu.ppuStatus &= (uint8_t)~0x80; // clear VBlank
            ppu.w = 0;
            ppu.nmi = 0; // Clear NMI request on status read
            break;

        case 0x2007: { // PPUDATA
            uint16_t a = ppu.v & 0x3FFF;
            uint8_t value = ppu_mem_read(a);

            if (a < 0x3F00) {
                result = ppu.buffer;
                ppu.buffer = value;
            } else {
                result = value;
                ppu.buffer = value; // palette reads are not delayed
            }

            ppu.v += (ppu.ppuCtrl & 0x04) ? 32 : 1;
            break;
        }
    }

    return result;
}

// PPU clock function to handle background rendering and VBlank timing
void ppu_clock(void)
{
    ppu.cycle++;

    int rendering_scanline = (ppu.scanline >= 0 && ppu.scanline < 240);
    int rendering_cycle    = (ppu.cycle >= 1 && ppu.cycle <= 256);
    int rendering_enabled  = (ppu.ppuMask & 0x08) != 0;

    if (rendering_enabled && rendering_scanline && rendering_cycle)
    {
        // Shift background registers every visible pixel
        ppu.bg_shift_pattern_low  <<= 1;
        ppu.bg_shift_pattern_high <<= 1;
        ppu.bg_shift_attr_low     <<= 1;
        ppu.bg_shift_attr_high    <<= 1;

        // Tick sprite shifters each visible pixel
        tick_sprite_shifters();

        // Background pixel generation
        uint16_t bit_mux = (uint16_t)(0x8000 >> ppu.x);

        uint8_t p0 = (ppu.bg_shift_pattern_low  & bit_mux) ? 1 : 0;
        uint8_t p1 = (ppu.bg_shift_pattern_high & bit_mux) ? 1 : 0;
        uint8_t bg_pixel = (uint8_t)((p1 << 1) | p0);

        uint8_t a0 = (ppu.bg_shift_attr_low  & bit_mux) ? 1 : 0;
        uint8_t a1 = (ppu.bg_shift_attr_high & bit_mux) ? 1 : 0;
        uint8_t bg_palette = (uint8_t)((a1 << 1) | a0);

        // Left 8 pixel background clipping
        if (!(ppu.ppuMask & 0x02) && (ppu.cycle - 1) < 8) {
            bg_pixel = 0;
        }

        // SPRITE PIXEL FETCH (highest priority sprite first)
        uint8_t sprite_pixel = 0;
        uint8_t sprite_palette = 0;
        uint8_t sprite_priority = 0;
        int sprite_zero_rendering = 0;

        for (int i = 0; i < ppu.sprite_count && i < 8; i++)
        {
            if (ppu.sprite_x_counter[i] == 0)
            {
                uint8_t sp0 = (ppu.sprite_shifter_pattern_low[i] & 0x80) ? 1 : 0;
                uint8_t sp1 = (ppu.sprite_shifter_pattern_high[i] & 0x80) ? 1 : 0;

                sprite_pixel = (uint8_t)((sp1 << 1) | sp0);

                if (sprite_pixel != 0)
                {
                    sprite_palette  = (uint8_t)((ppu.sprite_attr[i] & 0x03) + 0x04);
                    sprite_priority = (uint8_t)((ppu.sprite_attr[i] & 0x20) == 0);

                    if (i == 0) sprite_zero_rendering = 1;
                    break;
                }
            }
        }

        // PIXEL MIXING LOGIC
        uint8_t final_pixel = 0;
        uint8_t final_palette = 0;

        if (bg_pixel == 0 && sprite_pixel == 0)
        {
            final_pixel = 0;
            final_palette = 0;
        }
        else if (bg_pixel == 0 && sprite_pixel != 0)
        {
            final_pixel = sprite_pixel;
            final_palette = sprite_palette;
        }
        else if (bg_pixel != 0 && sprite_pixel == 0)
        {
            final_pixel = bg_pixel;
            final_palette = bg_palette;
        }
        else
        {
            if (sprite_priority) {
                final_pixel = sprite_pixel;
                final_palette = sprite_palette;
            } else {
                final_pixel = bg_pixel;
                final_palette = bg_palette;
            }

            // Sprite 0 hit detection
            if (sprite_zero_rendering)
            {
                int left_bg_clipped = (!(ppu.ppuMask & 0x02) && (ppu.cycle - 1) < 8);
                int left_sprite_clipped = (!(ppu.ppuMask & 0x04) && (ppu.cycle - 1) < 8);

                if (!left_bg_clipped && !left_sprite_clipped)
                {
                    ppu.ppuStatus |= 0x40;
                }
            }
        }

        // Final palette lookup and framebuffer write
        int x = ppu.cycle - 1;
        int y = ppu.scanline;

        if (x >= 0 && x < PPU_WIDTH && y >= 0 && y < PPU_HEIGHT)
        {
            uint8_t palette_addr;
            if (final_pixel == 0) {
                palette_addr = 0;
            } else {
                palette_addr = (uint8_t)((final_palette << 2) | (final_pixel & 0x03));
            }

            uint8_t palette_entry = ppu.palette[palette_addr & 0x1F];
            uint32_t rgb = nes_palette[palette_entry & 0x3F];
            ppu.framebuffer[y * PPU_WIDTH + x] = rgb;
        }

        // Tile fetch pipeline every 8 cycles
        switch ((ppu.cycle - 1) % 8)
        {
            case 0:
            {
                // Load shift registers
                ppu.bg_shift_pattern_low  = (ppu.bg_shift_pattern_low  & 0xFF00) | ppu.next_tile_lsb;
                ppu.bg_shift_pattern_high = (ppu.bg_shift_pattern_high & 0xFF00) | ppu.next_tile_msb;

                // Load attribute bits into shift registers
                uint8_t attr = ppu.next_tile_attr;

                ppu.bg_shift_attr_low  = (ppu.bg_shift_attr_low  & 0xFF00) | ((attr & 0x01) ? 0xFF : 0x00);
                ppu.bg_shift_attr_high = (ppu.bg_shift_attr_high & 0xFF00) | ((attr & 0x02) ? 0xFF : 0x00);

                // Fetch next tile ID from nametable
                uint16_t nt_addr  = 0x2000 | (ppu.v & 0x0FFF);
                uint16_t nt_index = mirror_nametable_addr(nt_addr);
                ppu.next_tile_id = ppu.nametable[nt_index];
                break;
            }

            case 2:
            {
                // Attribute table address
                uint16_t attr_addr  = 0x23C0 | (ppu.v & 0x0C00) | ((ppu.v >> 4) & 0x38) | ((ppu.v >> 2) & 0x07);
                uint16_t attr_index = mirror_nametable_addr(attr_addr);
                uint8_t attr_byte   = ppu.nametable[attr_index];

                uint8_t shift = (uint8_t)(((ppu.v >> 4) & 4) | (ppu.v & 2));
                ppu.next_tile_attr = (attr_byte >> shift) & 0x03;
                break;
            }

            case 4:
            {
                uint16_t pattern_addr =
                    (uint16_t)(((ppu.ppuCtrl & 0x10) ? 0x1000 : 0x0000) + (ppu.next_tile_id * 16) + ((ppu.v >> 12) & 0x7));
                ppu.next_tile_lsb = ppu_mem_read(pattern_addr);
                break;
            }

            case 6:
            {
                uint16_t pattern_addr =
                    (uint16_t)(((ppu.ppuCtrl & 0x10) ? 0x1000 : 0x0000) + (ppu.next_tile_id * 16) + ((ppu.v >> 12) & 0x7));
                ppu.next_tile_msb = ppu_mem_read((uint16_t)(pattern_addr + 8));
                break;
            }

            case 7:
            {
                // coarse X increment
                if ((ppu.v & 0x001F) == 31)
                {
                    ppu.v &= (uint16_t)~0x001F;
                    ppu.v ^= 0x0400;
                }
                else
                {
                    ppu.v += 1;
                }
                break;
            }
        }
    }

    // Vertical increment at cycle 256 visible scanlines
    if (rendering_enabled && rendering_scanline && ppu.cycle == 256)
    {
        if ((ppu.v & 0x7000) != 0x7000)
        {
            ppu.v += 0x1000;
        }
        else
        {
            ppu.v &= (uint16_t)~0x7000;

            uint16_t y = (ppu.v & 0x03E0) >> 5;

            if (y == 29)
            {
                y = 0;
                ppu.v ^= 0x0800;
            }
            else if (y == 31)
            {
                y = 0;
            }
            else
            {
                y++;
            }

            ppu.v = (ppu.v & (uint16_t)~0x03E0) | (uint16_t)(y << 5);
        }
    }

    // Horizontal scroll reload at cycle 257 visible scanlines
    if (rendering_enabled && rendering_scanline && ppu.cycle == 257)
    {
        ppu.v = (ppu.v & (uint16_t)~0x041F) | (ppu.t & 0x041F);
        evaluate_sprites();
        load_sprite_shifters(ppu.scanline + 1);
    }

    // Vertical scroll reload
    if (rendering_enabled && ppu.scanline == 261 && ppu.cycle >= 280 && ppu.cycle <= 304)
    {
        ppu.v = (ppu.v & (uint16_t)~0x7BE0) | (ppu.t & 0x7BE0);
    }

    // VBlank & NMI Logic
    ppu.nmi = 0;

    // Enter VBlank: scanline 241, cycle 1
    if (ppu.scanline == 241 && ppu.cycle == 1)
    {
        ppu.ppuStatus |= 0x80;

        if (ppu.ppuCtrl & 0x80) {
            ppu.nmi = 1;
        }
    }

    // Clear VBlank: pre-render line (261), cycle 1
    if (ppu.scanline == 261 && ppu.cycle == 1)
    {
        ppu.ppuStatus &= (uint8_t)~0x80;
    }

    // End of scanline / frame
    if (ppu.cycle >= 341)
    {
        ppu.cycle = 0;
        ppu.scanline++;

        if (ppu.scanline >= 262)
        {
            ppu.scanline = 0;
            ppu.frame++;
        }
    }
}

uint32_t *ppu_get_framebuffer(void) {
    return ppu.framebuffer;
}

uint8_t ppu_poll_nmi(void) {
    uint8_t n = ppu.nmi;
    ppu.nmi = 0;
    return n;
}

#ifdef PPU_TEST
int main(void) {
    ppu_init();
    ppu.ppuCtrl = 0x80; // Enable NMI

    printf("Starting PPUCTRL $2000 VRAM increment test...\n");
    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);

    ppu_write(0x2000, 0x00);
    ppu_write(0x2007, 0xAA);
    printf("VRAM addr after +1 write: %04X\n", ppu.v);

    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);

    ppu_write(0x2000, 0x04);
    ppu_write(0x2007, 0xBB);
    printf("VRAM addr after +32 write: %04X\n", ppu.v);

    printf("Testing PPUADDR latch reset...\n");
    ppu_write(0x2006, 0x3F);
    ppu_read(0x2002);
    ppu_write(0x2006, 0x00);
    ppu_write(0x2006, 0x10);
    printf("PPU v after reset+latch: %04X (expected 0010)\n", ppu.v);

    printf("Testing PPUSCROLL...\n");
    ppu_read(0x2002);
    ppu_write(0x2005, 0x15);
    ppu_write(0x2005, 0x2A);
    printf("t=%04X x=%d\n", ppu.t, ppu.x);

    printf("Testing PPUDATA buffering (standalone uses ppu.vram for $0000-$1FFF)...\n");
    ppu.vram[0x0000] = 0x11;
    ppu.vram[0x0001] = 0x22;

    ppu_write(0x2006, 0x00);
    ppu_write(0x2006, 0x00);

    uint8_t r1 = ppu_read(0x2007);
    uint8_t r2 = ppu_read(0x2007);
    printf("expect junk, then 11; Buffered reads: %02X then %02X \n", r1, r2);

    printf("Testing nametable mirroring...\n");
    ppu.mirroring = 0;
    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);
    ppu_write(0x2007, 0xAA);

    ppu_write(0x2006, 0x24);
    ppu_write(0x2006, 0x00);
    (void)ppu_read(0x2007);
    uint8_t mirror_h = ppu_read(0x2007);
    printf("Horizontal mirror read (expect AA): %02X\n", mirror_h);

    ppu.mirroring = 1;
    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);
    ppu_write(0x2007, 0xBB);

    ppu_write(0x2006, 0x28);
    ppu_write(0x2006, 0x00);
    (void)ppu_read(0x2007);
    uint8_t mirror_v = ppu_read(0x2007);
    printf("Vertical mirror read (expect BB): %02X\n", mirror_v);

    printf("Testing palette mirroring...\n");
    ppu_write(0x2006, 0x3F);
    ppu_write(0x2006, 0x10);
    ppu_write(0x2007, 0x77);

    ppu_write(0x2006, 0x3F);
    ppu_write(0x2006, 0x00);
    uint8_t pal = ppu_read(0x2007);
    printf("Palette mirror read (expect 77): %02X\n", pal);

    printf("PPU Timing Test Starting...\n");
    ppu_write(0x2000, 0x80);

    int total_cycles = 341 * 262 * 2;
    for (int i = 0; i < total_cycles; i++)
    {
        ppu_clock();

        if (ppu.scanline == 241 && ppu.cycle == 1)
        {
            printf("Entered VBlank | Frame: %d | Scanline: %d | Cycle: %d | NMI: %d\n",
                   ppu.frame, ppu.scanline, ppu.cycle, ppu.nmi);
        }

        if (ppu.scanline == 261 && ppu.cycle == 1)
        {
            printf("Cleared VBlank | Frame: %d | Scanline: %d | Cycle: %d\n",
                   ppu.frame, ppu.scanline, ppu.cycle);
        }

        if (ppu.nmi)
        {
            printf("NMI Requested | Frame: %d | Scanline: %d | Cycle: %d\n",
                   ppu.frame, ppu.scanline, ppu.cycle);
        }
    }

    printf("Finished.\n");
    printf("Final Frame Count: %d\n", ppu.frame);
    return 0;
}
#endif

// Additional test for CHR-ROM/RAM read/write via cartridge interface (added by Anjelica for ROM Loader integration)
#ifdef PPU_ROM_TEST
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s path/to/rom.nes\n", argv[0]);
        return 1;
    }

    Cartridge cart;
    char err[256];

    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        printf("rom_load failed: %s\n", err);
        return 1;
    }

    ppu_init();
    ppu_connect_cartridge(&cart);

    // Test: PPUDATA buffered read at $0000 should return CHR[0] on second read (CHR-ROM carts)
    ppu_write(0x2006, 0x00);
    ppu_write(0x2006, 0x00);

    uint8_t r1 = ppu_read(0x2007);
    uint8_t r2 = ppu_read(0x2007);

    printf("PPUDATA read test @ $0000: r1=%02X r2=%02X\n", r1, r2);
    if (!cart.chr_is_ram && cart.chr_size > 0) {
        printf("Expected r2 == cart.chr[0] (%02X)\n", cart.chr[0]);


    }

     printf("Mapper=%u | PRG=%u bytes | CHR=%u bytes | CHR_is_ram=%d\n",
       cart.header.mapper,
       (unsigned)cart.prg_size,
       (unsigned)cart.chr_size,
       (int)cart.chr_is_ram);

       printf("DEBUG: about to test CHR-RAM, chr_is_ram=%d\n", (int)cart.chr_is_ram);

       if (cart.chr_is_ram) {
    printf("CHR-RAM detected: testing write/readback...\n");

    // Write 0xAB to CHR address $0000 via PPUDATA
    ppu_write(0x2006, 0x00);
    ppu_write(0x2006, 0x00);
    ppu_write(0x2007, 0xAB);

    // Reset address to $0000 and read back.
    ppu_write(0x2006, 0x00);
    ppu_write(0x2006, 0x00);
    (void)ppu_read(0x2007);          // dummy buffered read
    uint8_t back = ppu_read(0x2007); // actual read

    printf("CHR-RAM readback @ $0000: %02X (expected AB)\n", back);
}

    rom_free(&cart);
    return 0;
}
#endif
//------------------------------------------------------------------------------//