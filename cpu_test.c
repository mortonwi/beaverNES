// NES CPU test harness for nestest.nes with nestest.log-style tracing

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "beaverNES-anjelica-dev/rom_loader.h"
#include "beaverNES-anjelica-dev/cartridge.h"

#include "include/memory.h"
#include "include/bus.h"
#include "include/cpu.h"
#include "include/opcodes.h"

typedef enum {
    AM_IMP,  // implied
    AM_ACC,  // accumulator
    AM_IMM,  // #$nn
    AM_ZP,   // $nn
    AM_ZPX,  // $nn,X
    AM_ZPY,  // $nn,Y
    AM_ABS,  // $nnnn
    AM_ABX,  // $nnnn,X
    AM_ABY,  // $nnnn,Y
    AM_IND,  // ($nnnn)
    AM_INX,  // ($nn,X)
    AM_INY,  // ($nn),Y
    AM_REL   // branch
} AddrMode;

// Length table (1–3 bytes)
static const uint8_t OPCODE_LEN[256] = {
  1,2,0,0,0,2,2,0, 1,2,1,0,0,3,3,0,
  2,2,0,0,0,2,2,0, 1,3,1,0,0,3,3,0,
  3,2,0,0,2,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,0,2,2,0, 1,3,1,0,0,3,3,0,
  1,2,0,0,0,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,0,2,2,0, 1,3,1,0,0,3,3,0,
  1,2,0,0,0,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,0,2,2,0, 1,3,1,0,0,3,3,0,
  2,2,0,0,2,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,2,2,2,0, 1,3,1,0,0,3,3,0,
  2,2,2,0,2,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,2,2,2,0, 1,3,1,0,3,3,3,0,
  2,2,2,0,2,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,0,2,2,0, 1,3,1,0,0,3,3,0,
  2,2,0,0,2,2,2,0, 1,2,1,0,3,3,3,0,
  2,2,0,0,0,2,2,0, 1,3,1,0,0,3,3,0
};

// Mnemonics
static const char *MNEMONIC[256] = {
    "BRK","ORA","???","???","???","ORA","ASL","???",
    "PHP","ORA","ASL","???","???","ORA","ASL","???",
    "BPL","ORA","???","???","???","ORA","ASL","???",
    "CLC","ORA","???","???","???","ORA","ASL","???",
    "JSR","AND","???","???","BIT","AND","ROL","???",
    "PLP","AND","ROL","???","BIT","AND","ROL","???",
    "BMI","AND","???","???","???","AND","ROL","???",
    "SEC","AND","???","???","???","AND","ROL","???",
    "RTI","EOR","???","???","???","EOR","LSR","???",
    "PHA","EOR","LSR","???","JMP","EOR","LSR","???",
    "BVC","EOR","???","???","???","EOR","LSR","???",
    "CLI","EOR","???","???","???","EOR","LSR","???",
    "RTS","ADC","???","???","???","ADC","ROR","???",
    "PLA","ADC","ROR","???","JMP","ADC","ROR","???",
    "BVS","ADC","???","???","???","ADC","ROR","???",
    "SEI","ADC","???","???","???","ADC","ROR","???",
    "???","STA","???","???","STY","STA","STX","???",
    "DEY","???","TXA","???","STY","STA","STX","???",
    "BCC","STA","???","???","STY","STA","STX","???",
    "TYA","STA","TXS","???","???","STA","???","???",
    "LDY","LDA","LDX","???","LDY","LDA","LDX","???",
    "TAY","LDA","TAX","???","LDY","LDA","LDX","???",
    "BCS","LDA","???","???","LDY","LDA","LDX","???",
    "CLV","LDA","TSX","???","LDY","LDA","LDX","???",
    "CPY","CMP","???","???","CPY","CMP","DEC","???",
    "INY","CMP","DEX","???","CPY","CMP","DEC","???",
    "BNE","CMP","???","???","???","CMP","DEC","???",
    "CLD","CMP","???","???","???","CMP","DEC","???",
    "CPX","SBC","???","???","CPX","SBC","INC","???",
    "INX","SBC","NOP","???","CPX","SBC","INC","???",
    "BEQ","SBC","???","???","???","SBC","INC","???",
    "SED","SBC","???","???","???","SBC","INC","???",
};

// Addressing modes per opcode (only legal opcodes used by nestest matter)
static const AddrMode ADDR_MODE[256] = {
    AM_IMP,AM_INX,AM_IMP,AM_IMP,AM_IMP,AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_ACC,AM_IMP,AM_IMP,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_IMP,

    AM_ABS,AM_INX,AM_IMP,AM_IMP,AM_ZP, AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_ACC,AM_IMP,AM_ABS,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_IMP,

    AM_IMP,AM_INX,AM_IMP,AM_IMP,AM_IMP,AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_ACC,AM_IMP,AM_ABS,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_IMP,

    AM_IMP,AM_INX,AM_IMP,AM_IMP,AM_IMP,AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_ACC,AM_IMP,AM_IND,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_IMP,

    AM_IMM,AM_INX,AM_IMM,AM_IMP,AM_ZP, AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_IMP,AM_IMP,AM_ABS,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_ZPY,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_ABY,AM_IMP,

    AM_IMM,AM_INX,AM_IMM,AM_IMP,AM_ZP, AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_IMP,AM_IMP,AM_ABS,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_ZPY,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_ABY,AM_IMP,

    AM_IMM,AM_INX,AM_IMP,AM_IMP,AM_ZP, AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_IMP,AM_IMP,AM_ABS,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_IMP,

    AM_IMM,AM_INX,AM_IMP,AM_IMP,AM_ZP, AM_ZP, AM_ZP, AM_IMP,
    AM_IMP,AM_IMM,AM_IMP,AM_IMP,AM_ABS,AM_ABS,AM_ABS,AM_IMP,
    AM_REL,AM_INY,AM_IMP,AM_IMP,AM_IMP,AM_ZPX,AM_ZPX,AM_IMP,
    AM_IMP,AM_ABY,AM_IMP,AM_IMP,AM_IMP,AM_ABX,AM_ABX,AM_IMP,
};

// Format operand string for tracing
// Converts raw instruction bytes into human-readable operands exactly like nestest.log
static void format_operand(char *buf, size_t sz,
                           AddrMode mode,
                           uint16_t pc,
                           uint8_t b2, uint8_t b3,
                           CPU *cpu, Bus *bus)
{
    uint16_t addr, eff;
    uint8_t val;

    switch (mode) {
    case AM_IMM:
        snprintf(buf, sz, "#$%02X", b2);
        break;
    case AM_ZP:
        addr = b2;
        val = bus_read(bus, addr);
        snprintf(buf, sz, "$%02X = %02X", addr, val);
        break;
    case AM_ZPX:
        addr = b2;
        eff  = (uint8_t)(addr + cpu->X);
        val  = bus_read(bus, eff);
        snprintf(buf, sz, "$%02X,X @ %02X = %02X", addr, eff, val);
        break;
    case AM_ZPY:
        addr = b2;
        eff  = (uint8_t)(addr + cpu->Y);
        val  = bus_read(bus, eff);
        snprintf(buf, sz, "$%02X,Y @ %02X = %02X", addr, eff, val);
        break;
    case AM_ABS:
        addr = (uint16_t)(b2 | (b3 << 8));
        val  = bus_read(bus, addr);
        snprintf(buf, sz, "$%04X = %02X", addr, val);
        break;
    case AM_ABX:
        addr = (uint16_t)(b2 | (b3 << 8));
        eff  = addr + cpu->X;
        val  = bus_read(bus, eff);
        snprintf(buf, sz, "$%04X,X @ %04X = %02X", addr, eff, val);
        break;
    case AM_ABY:
        addr = (uint16_t)(b2 | (b3 << 8));
        eff  = addr + cpu->Y;
        val  = bus_read(bus, eff);
        snprintf(buf, sz, "$%04X,Y @ %04X = %02X", addr, eff, val);
        break;
    case AM_INX: {
        uint8_t zp = (uint8_t)(b2 + cpu->X);
        uint16_t ptr = zp;
        uint16_t lo = bus_read(bus, ptr);
        uint16_t hi = bus_read(bus, (uint8_t)(ptr + 1));
        eff = (hi << 8) | lo;
        val = bus_read(bus, eff);
        snprintf(buf, sz, "($%02X,X) @ %02X = %04X = %02X", b2, zp, eff, val);
        break;
    }
    case AM_INY: {
        uint8_t zp = b2;
        uint16_t lo = bus_read(bus, zp);
        uint16_t hi = bus_read(bus, (uint8_t)(zp + 1));
        addr = (hi << 8) | lo;
        eff  = addr + cpu->Y;
        val  = bus_read(bus, eff);
        snprintf(buf, sz, "($%02X),Y = %04X @ %04X = %02X", b2, addr, eff, val);
        break;
    }
    case AM_IND:
        addr = (uint16_t)(b2 | (b3 << 8));
        eff  = (uint16_t)(bus_read(bus, addr) |
                          (bus_read(bus, (uint16_t)((addr & 0xFF00) | ((addr + 1) & 0x00FF))) << 8));
        snprintf(buf, sz, "($%04X) = %04X", addr, eff);
        break;
    case AM_REL: {
        int8_t off = (int8_t)b2;
        uint16_t target = (uint16_t)(pc + 2 + off);
        snprintf(buf, sz, "$%04X", target);
        break;
    }
    case AM_ACC:
        snprintf(buf, sz, "A");
        break;
    case AM_IMP:
    default:
        buf[0] = '\0';
        break;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s rom/nestest.nes\n", argv[0]);
        return 1;
    }

    // --- Load ROM ---
    Cartridge cart;
    char err[256];
    if (!rom_load(argv[1], &cart, err, sizeof(err))) {
        fprintf(stderr, "ROM load failed: %s\n", err);
        return 1;
    }

    // --- Create memory + bus + CPU ---
    Memory *mem = memory_create();
    Bus *bus = bus_create(mem);
    bus->rom = &cart;     // attach ROM ONCE

    CPU *cpu = cpu_create(bus);

    init_opcode_table();
    cpu_reset(cpu);

    // nestest entry point
    cpu->PC = 0xC000;

    // clear result byte
    bus_write(bus, 0x6000, 0x00);

    const int max_steps = 2000000;

    for (int step = 0; step < max_steps; ++step) {

        uint16_t pc = cpu->PC;
        uint8_t op = bus_read(bus, pc);
        uint8_t len = OPCODE_LEN[op];
        AddrMode mode = ADDR_MODE[op];
        const char *mn = MNEMONIC[op];

        uint8_t b1 = op;
        uint8_t b2 = (len >= 2) ? bus_read(bus, pc + 1) : 0;
        uint8_t b3 = (len == 3) ? bus_read(bus, pc + 2) : 0;

        char operand[64] = {0};

        if (op == 0x4C || op == 0x20) {
            uint16_t addr = b2 | (b3 << 8);
            snprintf(operand, sizeof(operand), "$%04X", addr);
        } else {
            format_operand(operand, sizeof(operand), mode, pc, b2, b3, cpu, bus);
        }

        // --- Print nestest format ---
        printf("%04X  ", pc);

        printf("%02X ", b1);
        if (len >= 2) printf("%02X ", b2); else printf("   ");
        if (len == 3) printf("%02X", b3); else printf("  ");

        printf("  %-3s %-28s", mn, operand);

        printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X ",
            cpu->A, cpu->X, cpu->Y, cpu->P, cpu->SP);

        printf("PPU:%3d,%3d ", 0, 0);
        printf("CYC:%llu\n", (unsigned long long)cpu->cycles);

        if (op == 0x04) {
    printf("CPU DEBUG: op=04 mode=%d\n",
           cpu->addr_mode);
}


        cpu_step(cpu);

        uint8_t status = bus_read(bus, 0x6000);
        if (status == 0x80) {
            fprintf(stderr, "nestest PASSED\n");
            break;
        }
        if (status == 0x01) {
            fprintf(stderr, "nestest FAILED\n");
            break;
        }
    }

    rom_free(&cart);
    free(cpu);
    return 0;
}
