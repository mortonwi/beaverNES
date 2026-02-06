/*
PPU prototype
compile with: gcc -Wall -Wextra -DPPU_TEST ppu.c -o ppu_test
*/ 


#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PPU_WIDTH 256
#define PPU_HEIGHT 240

// PPU STRUCTURE
typedef struct {
    uint8_t vram[0x4000];   // PPU address space
    uint8_t oam[256];       // Sprite RAM

    // PPU registers
    uint8_t ppuCtrl;           // $2000 PPUCTRL: NMI enable, increment mode, pattern table select
    uint8_t ppuMask;           // $2001 PPUMASK: rendering enable, color emphasis
    uint8_t ppuStatus;         // $2002 PPUSTATUS: vblank, sprite 0 hit, overflow (read clears vblank)
    uint8_t oamAddr;       // $2003 OAMADDR: sprite memory address

    // Internal registers
    uint16_t vram_addr;
    uint8_t addr_latch;
} PPU;

static PPU ppu;

// power-up state
void ppu_init(void) {
    memset(&ppu, 0, sizeof(PPU));
    ppu.ppuStatus = 0xA0; 
}


// REGISTER ACCESS
void ppu_write(uint16_t addr, uint8_t value) {
    switch (addr & 0x2007) {
        case 0x2000: // PPUCTRL
            ppu.ppuCtrl = value;
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

        case 0x2006: // PPUADDR
            if (!ppu.addr_latch) {
                ppu.vram_addr = (value & 0x3F) << 8;
                ppu.addr_latch = 1;
            } else {
                ppu.vram_addr |= value;
                ppu.addr_latch = 0;
            }
            break;

        case 0x2007: // PPUDATA
            ppu.vram[ppu.vram_addr & 0x3FFF] = value;
            ppu.vram_addr += (ppu.ppuCtrl & 0x04) ? 32 : 1;
            break;
    }
}

uint8_t ppu_read(uint16_t addr) {
    uint8_t result = 0;

    switch (addr & 0x2007) {
        case 0x2002: // PPUSTATUS
            result = ppu.ppuStatus;
            ppu.ppuStatus &= ~0x80; // clear vblank
            ppu.addr_latch = 0;
            break;

        case 0x2007: // PPUDATA
            result = ppu.vram[ppu.vram_addr & 0x3FFF];
            ppu.vram_addr += (ppu.ppuCtrl & 0x04) ? 32 : 1;
            break;
    }

    return result;
}

/*
 BACKGROUND RENDERER (PROTOTYPE)
This function is a temporary background renderer used for testing.
It does NOT emulate real NES timing, scanlines, or VBlank behavior.
For now it Validates VRAM layout, pattern table tile decoding, nametable → tile → pixel mapping
Iterates over the 32x30 tile grid
For each tile, read its pattern data (8x8 pixels), convert pattern bits into pixels in a framebuffer
This will be replaced later by a cycle-accurate PPU renderer.
*/
void ppu_render_frame(uint32_t *framebuffer) {
    memset(framebuffer, 0, PPU_WIDTH * PPU_HEIGHT * sizeof(uint32_t));

    uint16_t nametable_base = 0x2000;
    uint16_t pattern_base = (ppu.ppuCtrl & 0x10) ? 0x1000 : 0x0000;
    //loop through 32x30 grid
    for (int tile_y = 0; tile_y < 30; tile_y++) {
        for (int tile_x = 0; tile_x < 32; tile_x++) {

            //read title index from nametable
            uint8_t tile_index =
                ppu.vram[nametable_base + tile_y * 32 + tile_x];
            
            //each tile is 16 bytes in the pattern table
            uint16_t tile_addr = pattern_base + tile_index * 16;

            //each tile with a height of 8 pixels
            for (int row = 0; row < 8; row++) {
                uint8_t lo = ppu.vram[tile_addr + row];
                uint8_t hi = ppu.vram[tile_addr + row + 8];

                //each tile 8 pixels wide
                for (int col = 0; col < 8; col++) {
                    uint8_t bit = 7 - col;
                    uint8_t color =
                        ((hi >> bit) & 1) << 1 |
                        ((lo >> bit) & 1);

                    int x = tile_x * 8 + col;
                    int y = tile_y * 8 + row;

                    if (color) {
                        framebuffer[y * PPU_WIDTH + x] = 0xFFFFFFFF;
                    }
                }
            }
        }
    }
}


/*
Ttest main function populates VRAM with a pattern and nametable data to validate 
background tile decoding logic without requiring a CPU, ROM, or display output.
*/
#ifdef PPU_TEST
int main(void) {
    uint32_t framebuffer[PPU_WIDTH * PPU_HEIGHT];

    ppu_init();

    // Fake pattern table (checkerboard)
    for (int i = 0; i < 256; i++) {
        ppu.vram[i * 16] = 0xAA;
        ppu.vram[i * 16 + 8] = 0x55;
    }

    // Fake nametable
    for (int i = 0; i < 960; i++) {
        ppu.vram[0x2000 + i] = i % 256;
    }

    ppu_render_frame(framebuffer);
    printf("PPU prototype ran successfully.\n");
    return 0;
}
#endif
