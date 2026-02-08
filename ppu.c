/*
PPU prototype
compile with: gcc -Wall -Wextra -DPPU_TEST ppu.c -o ppu_test
*/ 


#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PPU_WIDTH 256
#define PPU_HEIGHT 240

// PPU Structure for state and registers
typedef struct {
    uint8_t vram[0x4000];   // PPU address space
    uint8_t oam[256];       // Sprite RAM

    // PPU registers
    uint8_t ppuCtrl;             // $2000 PPUCTRL: NMI enable, increment mode, pattern table select
    uint8_t ppuMask;            // $2001 PPUMASK: rendering enable, color emphasis
    uint8_t ppuStatus;         // $2002 PPUSTATUS: vblank, sprite 0 hit, overflow (read clears vblank)
    uint8_t oamAddr;          // $2003 OAMADDR: sprite memory address
    uint8_t oamData;         // $2004 OAMDATA: read/write sprite data
    uint8_t ppuScroll;      // $2005 PPUSCROLL: scroll position (write twice)
    uint8_t ppuAddr;       // $2006 PPUADDR: VRAM address (write twice)
    uint8_t ppuData;      // $2007 PPUDATA: VRAM data read/write
    uint8_t oamDMA;      // $4014 OAMDMA: DMA transfer to OAM (write only)

    // Internal PPU state
    uint16_t vramAddress;         // v: Current VRAM address (15 bits)
    uint8_t tempVramAddress;     // t: Temporary VRAM address (15 bits)
    uint8_t  xScroll;           // x: Fine X scroll (3 bits)
    uint8_t  wToggle;          // w: Write toggle for $2005/$2006
    uint8_t  readBuffer;      // Buffered read for PPUDATA

} PPU;

static PPU ppu;

// power-up state
void ppu_init(void) {
    memset(&ppu, 0, sizeof(PPU));
    ppu.ppuStatus = 0xA0; 
}


// Read REGISTER address assignments and behavior.
void ppu_write(uint16_t addr, uint8_t value) {
    switch (addr & 0x2007) {
        case 0x2000: // PPUCTRL
            ppu.ppuCtrl = value;
            // Update nametable select bits in temp VRAM address (t)
            //store two bits from $2000 into temp VRAM address to be used later.
            ppu.tempVramAddress = (ppu.tempVramAddress & 0xF3FF) | ((value & 0x03) << 10);
            // NMI enable bit 7 is stored but not acted on yet
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
            if (!ppu.tempVramAddress) {
                ppu.vramAddress = (value & 0x3F) << 8;
                ppu.tempVramAddress = 1;
            } else {
                ppu.vramAddress |= value;
                ppu.tempVramAddress = 0;
            }
            break;

        case 0x2007: // PPUDATA
            ppu.vram[ppu.vramAddress & 0x3FFF] = value;
            ppu.vramAddress += (ppu.ppuCtrl & 0x04) ? 32 : 1;
            break;
    }
}

uint8_t ppu_read(uint16_t addr) {
    uint8_t result = 0;

    switch (addr & 0x2007) {
        case 0x2002: // PPUSTATUS
            result = ppu.ppuStatus;
            ppu.ppuStatus &= ~0x80; // clear vblank
            ppu.tempVramAddress = 0;
            break;

        case 0x2007: // PPUDATA
            result = ppu.vram[ppu.vramAddress & 0x3FFF];
            ppu.vramAddress += (ppu.ppuCtrl & 0x04) ? 32 : 1;
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
    printf("PPU prototype running successfully.\n");

    // --- Test PPUCTRL VRAM increment behavior ---
    printf("Starting PPUCTRL $2000 VRAM increment test...\n");
    // Set VRAM address to $2000
    ppu_write(0x2006, 0x20); // high byte
    ppu_write(0x2006, 0x00); // low byte

    // Case 1: increment by 1
    ppu_write(0x2000, 0x00); // bit 2 = 0
    ppu_write(0x2007, 0xAA);
    printf("VRAM addr after +1 write: %04X\n", ppu.vramAddress);

    // Reset address
    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);

    // Case 2: increment by 32
    ppu_write(0x2000, 0x04); // bit 2 = 1
    ppu_write(0x2007, 0xBB);
    printf("VRAM addr after +32 write: %04X\n", ppu.vramAddress);
    return 0;
}
#endif
