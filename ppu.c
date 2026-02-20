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
    uint8_t oam[256];      // Sprite RAM

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
    uint16_t v;   // v: Current VRAM address (15 bits)
    uint16_t t;            // t: Temporary VRAM address (15 bits)
    uint8_t  x;           // x: Fine X scroll (3 bits)
    uint8_t  w;          // w: Write toggle for $2005/$2006
    uint8_t  buffer;    // Buffered read for PPUDATA

    //nametable and palette RAM (for testing without a cartridge)
    uint8_t nametable[0x800];   // 2 KB internal nametable RAM
    uint8_t palette[0x20];      // palette RAM (placeholder)
    uint8_t mirroring;          // 0 = horizontal, 1 = vertical

    // Timing state (for future cycle-accurate rendering)
    int cycle;        // 0–340
    int scanline;     // -1–260 
    int frame;
    uint8_t nmi;      // NMI request flag

    //Background shift registers
    uint16_t bg_shift_pattern_low;
    uint16_t bg_shift_pattern_high;
    uint16_t bg_shift_attr_low;
    uint16_t bg_shift_attr_high;
    uint8_t next_tile_id;
    uint8_t next_tile_attr;
    uint8_t next_tile_lsb;
    uint8_t next_tile_msb;
} PPU;



static PPU ppu;

// power-up state
void ppu_init(void) {
    memset(&ppu, 0, sizeof(PPU));
    ppu.ppuStatus = 0xA0; 
    ppu.mirroring = 0; // horizontal by default

}

static uint16_t mirror_nametable_addr(uint16_t addr) {
    // addr expected in $2000–$2FFF
    uint16_t offset = (addr - 0x2000) & 0x0FFF;
    uint16_t table = offset / 0x400;   // 0–3
    uint16_t index = offset & 0x3FF;

    if (ppu.mirroring == 1) {
        // vertical: NT0,NT2 | NT1,NT3
        table &= 1;
    } else {
        // horizontal: NT0,NT1 | NT2,NT3
        table >>= 1;
    }

    return (table * 0x400) + index;
}


// Read REGISTER address assignments and behavior.
void ppu_write(uint16_t addr, uint8_t value) {
    switch (addr & 0x2007) {
        case 0x2000: // PPUCTRL
            ppu.ppuCtrl = value;
            // Update nametable select bits in temp VRAM address (t)
            //store two bits from $2000 into temp VRAM address to be used later.
            ppu.t = (ppu.t & 0xF3FF) | ((value & 0x03) << 10);
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

        case 0x2005: // PPUSCROLL
            if (!ppu.w) {
                // First write: horizontal scroll
                ppu.x = value & 0x07;                 // fine X (3 bits)
                ppu.t = (ppu.t & 0x7FE0)              // keep coarse Y + fine Y + NT
                    | ((value >> 3) & 0x1F);        // coarse X
                ppu.w = 1;
            } else {
                // Second write: vertical scroll
                ppu.t = (ppu.t & 0x0C1F)              // keep coarse X + NT
                    | ((value & 0x07) << 12)        // fine Y
                    | ((value & 0xF8) << 2);        // coarse Y
                ppu.w = 0;
            }
            break;

        case 0x2006: // PPUADDR
            if (!ppu.w) {
                // First write: high byte (bits 8–13)
                ppu.t = (ppu.t & 0x00FF) | ((value & 0x3F) << 8);
                ppu.w = 1;
            } else {
                // Second write: low byte, then copy t → v
                ppu.t = (ppu.t & 0xFF00) | value;
                ppu.v = ppu.t;
                ppu.w = 0;
            }
            break;


        case 0x2007: // PPUDATA
            uint16_t addr = ppu.v & 0x3FFF;

                if (addr < 0x2000) {
                    ppu.vram[addr] = value; // CHR placeholder
                } 
                
                else if (addr < 0x3F00) {
                    uint16_t nt = mirror_nametable_addr(addr);
                    ppu.nametable[nt] = value;
                
                } 
                
                else {
                    uint16_t pal_addr = addr & 0x1F;

                    // Palette mirroring: $3F10/$14/$18/$1C → $3F00/$04/$08/$0C
                    if ((pal_addr & 0x13) == 0x10) {
                        pal_addr &= ~0x10;
                    }

                    ppu.palette[pal_addr] = value;
                }

            ppu.v += (ppu.ppuCtrl & 0x04) ? 32 : 1;
            break;
    }
}

uint8_t ppu_read(uint16_t addr) {
    uint8_t result = 0;

    switch (addr & 0x2007) {

        case 0x2002: // PPUSTATUS
            result = ppu.ppuStatus;
            ppu.ppuStatus &= ~0x80; // clear VBlank
            ppu.w = 0;
            ppu.nmi = 0; // Clear NMI request on status read
            break;

        case 0x2007: { // PPUDATA
            uint16_t a = ppu.v & 0x3FFF;
            uint8_t value;

            if (a < 0x2000) {
                value = ppu.vram[a];
            } else if (a < 0x3F00) {
                uint16_t nt = mirror_nametable_addr(a);
                value = ppu.nametable[nt];
            } else {
                uint16_t pal_addr = a & 0x1F;

                // Palette mirroring: $3F10/$14/$18/$1C → $3F00/$04/$08/$0C
                if ((pal_addr & 0x13) == 0x10) {
                    pal_addr &= ~0x10;
                }

                value = ppu.palette[pal_addr];
            }

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

//PPU clock function to handle background rendering and VBlank timing
void ppu_clock(void)
{
    ppu.cycle++;

    // Background Rendering (visible scanlines only)
    int rendering_scanline = (ppu.scanline >= 0 && ppu.scanline < 240);
    int rendering_cycle    = (ppu.cycle >= 1 && ppu.cycle <= 256);

    if (rendering_scanline && rendering_cycle)
    {
        // Shift background registers every visible pixel
        ppu.bg_shift_pattern_low  <<= 1;
        ppu.bg_shift_pattern_high <<= 1;
        ppu.bg_shift_attr_low     <<= 1;
        ppu.bg_shift_attr_high    <<= 1;

        // Tile fetch pipeline every 8 cycles
        switch ((ppu.cycle - 1) % 8)
        {
            case 0:
            {
                // Load shift registers
                ppu.bg_shift_pattern_low  =
                    (ppu.bg_shift_pattern_low & 0xFF00) | ppu.next_tile_lsb;

                ppu.bg_shift_pattern_high =
                    (ppu.bg_shift_pattern_high & 0xFF00) | ppu.next_tile_msb;

                // Fetch next tile ID
                uint16_t nt_addr = 0x2000 | (ppu.v & 0x0FFF);
                uint16_t nt_index = mirror_nametable_addr(nt_addr);
                ppu.next_tile_id = ppu.nametable[nt_index];
                break;
            }

            case 2:
                // Attribute fetch (still placeholder)
                ppu.next_tile_attr = 0;
                break;

            case 4:
            {
                uint16_t pattern_addr =
                    ((ppu.ppuCtrl & 0x10) ? 0x1000 : 0x0000)
                    + (ppu.next_tile_id * 16)
                    + ((ppu.v >> 12) & 0x7);

                ppu.next_tile_lsb = ppu.vram[pattern_addr];
                break;
            }

            case 6:
            {
                uint16_t pattern_addr =
                    ((ppu.ppuCtrl & 0x10) ? 0x1000 : 0x0000)
                    + (ppu.next_tile_id * 16)
                    + ((ppu.v >> 12) & 0x7);

                ppu.next_tile_msb = ppu.vram[pattern_addr + 8];
                break;
            }

            case 7:
            {
                //coarse X increment
                if ((ppu.v & 0x001F) == 31)
                {
                    // coarse X = 0
                    ppu.v &= ~0x001F;

                    // Switch horizontal nametable
                    ppu.v ^= 0x0400;
                }
                else
                {
                    // coarse X++
                    ppu.v += 1;
                }
                break;
            }

        }
    }

    // Vertical increment at cycle 256 visible scanlines

    if (rendering_scanline && ppu.cycle == 256)
    {
        // If fine Y < 7, just increment fine Y
        if ((ppu.v & 0x7000) != 0x7000)
        {
            ppu.v += 0x1000;
        }
        else
        {
            // fine Y = 0
            ppu.v &= ~0x7000;

            // extract coarse Y
            uint16_t y = (ppu.v & 0x03E0) >> 5;

            if (y == 29)
            {
                y = 0;
                // switch vertical nametable
                ppu.v ^= 0x0800;
            }
            else if (y == 31)
            {
                // attribute table row skip
                y = 0;
            }
            else
            {
                y++;
            }

            // write coarse Y back
            ppu.v = (ppu.v & ~0x03E0) | (y << 5);
        }
    }

    // VBlank & NMI Logic
    ppu.nmi = 0;

    // Enter VBlank: scanline 241, cycle 1
    if (ppu.scanline == 241 && ppu.cycle == 1)
    {
        ppu.ppuStatus |= 0x80;  // Set VBlank flag

        if (ppu.ppuCtrl & 0x80)
        {
            ppu.nmi = 1;        // Pulse NMI for one cycle
        }
    }

    // Clear VBlank: pre-render line (261), cycle 1
    if (ppu.scanline == 261 && ppu.cycle == 1)
    {
        ppu.ppuStatus &= ~0x80;
    }

    // End of Scanline / Frame
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

/*
Test main function populates VRAM with a pattern and nametable data to validate 
background tile decoding logic without requiring a CPU, ROM, or display output.
*/
#ifdef PPU_TEST
int main(void) {
   //int32_t framebuffer[PPU_WIDTH * PPU_HEIGHT];

    ppu_init();
    ppu.ppuCtrl = 0x80; // Enable NMI

    //Test PPUCTRL VRAM increment behavior
    printf("Starting PPUCTRL $2000 VRAM increment test...\n");
    // Set VRAM address to $2000
    ppu_write(0x2006, 0x20); // high byte
    ppu_write(0x2006, 0x00); // low byte

    // Case 1: increment by 1
    ppu_write(0x2000, 0x00); // bit 2 = 0
    ppu_write(0x2007, 0xAA);
    printf("VRAM addr after +1 write: %04X\n", ppu.v);

    // Reset address
    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);

    // Case 2: increment by 32
    ppu_write(0x2000, 0x04); // bit 2 = 1
    ppu_write(0x2007, 0xBB);
    printf("VRAM addr after +32 write: %04X\n", ppu.v);

    //Test PPUADDR latch reset
    printf("Testing PPUADDR latch reset...\n");

    ppu_write(0x2006, 0x3F); // high
    ppu_read(0x2002);        // reset w
    ppu_write(0x2006, 0x00); // should be high again
    ppu_write(0x2006, 0x10); // low

    printf("PPU v after reset+latch: %04X (expected 0010)\n", ppu.v);

    //Test PPUSCROLL
    printf("Testing PPUSCROLL...\n");
    ppu_read(0x2002);        // reset w

    ppu_write(0x2005, 0x15); // X scroll
    ppu_write(0x2005, 0x2A); // Y scroll

    printf("t=%04X x=%d\n", ppu.t, ppu.x);

    //Test PPUDATA buffering
    printf("Testing PPUDATA buffering...\n");

    ppu.vram[0x0000] = 0x11;
    ppu.vram[0x0001] = 0x22;

    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);

    uint8_t r1 = ppu_read(0x2007);
    uint8_t r2 = ppu_read(0x2007);

    printf("expect junk, then 11; Buffered reads: %02X then %02X \n", r1, r2);

    printf("Testing nametable mirroring...\n");

    /* Horizontal mirroring (default = 0)
    $2000 mirrors $2400 */
    ppu.mirroring = 0;

    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);
    ppu_write(0x2007, 0xAA);

    ppu_write(0x2006, 0x24);
    ppu_write(0x2006, 0x00);
    (void)ppu_read(0x2007); // dummy read to reset latch
    uint8_t mirror_h = ppu_read(0x2007);

    printf("Horizontal mirror read (expect AA): %02X\n", mirror_h);

    /* Vertical mirroring
    $2000 mirrors $2800 */
    ppu.mirroring = 1;

    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);
    ppu_write(0x2007, 0xBB);

    ppu_write(0x2006, 0x28);
    ppu_write(0x2006, 0x00);
    (void)ppu_read(0x2007); // dummy read to reset latch
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
    
    printf("Testing buffered vs immediate PPUDATA reads...\n");

    ppu.vram[0x2000] = 0x11;

    ppu_write(0x2006, 0x20);
    ppu_write(0x2006, 0x00);
    uint8_t b1 = ppu_read(0x2007); // buffered (junk)
    uint8_t b2 = ppu_read(0x2007); // actual

    ppu_write(0x2006, 0x3F);
    ppu_write(0x2006, 0x00);
    uint8_t imm = ppu_read(0x2007); // immediate

    printf("Buffered reads: %02X then %02X | Immediate: %02X\n", b1, b2, imm);

    printf("PPU Timing Test Starting...\n");

    // Re-enable NMI before timing test
    ppu_write(0x2000, 0x80);

    int total_cycles = 341 * 262 * 2; // Run for 2 frames

    for (int i = 0; i < total_cycles; i++)
    {
        ppu_clock();

        // Detect VBlank entry
        if (ppu.scanline == 241 && ppu.cycle == 1)
        {
            printf("Entered VBlank | Frame: %d | Scanline: %d | Cycle: %d | NMI: %d\n",
                   ppu.frame, ppu.scanline, ppu.cycle, ppu.nmi);
        }

        // Detect VBlank clear
        if (ppu.scanline == 261 && ppu.cycle == 1)
        {
            printf("Cleared VBlank | Frame: %d | Scanline: %d | Cycle: %d\n",
                   ppu.frame, ppu.scanline, ppu.cycle);
        }

        // Detect NMI request
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
