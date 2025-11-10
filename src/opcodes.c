#include "opcodes.h"
#include <stdio.h>
#include <string.h>

// 256 possible opcodes in an 8-bit system
Op opcode_table[256];

// forward declarations of a single 6502 instruction
static void op_nop(CPU *cpu, Bus *bus); // no operation
static void op_lda(CPU *cpu, Bus *bus); // load accumulator
static void op_sta(CPU *cpu, Bus *bus); // store accumumlator
static void op_inx(CPU *cpu, Bus *bus); // increment x register

// addressing mode helpers to handle how operands are fetched

static uint8_t fetch_immediate(CPU *cpu, Bus *bus) {
    uint8_t v = bus_read(bus, cpu->PC++); // read byte at PC, then increment PC
    cpu->fetched = v; // store for logging n debugging
    return v;
}

static uint16_t fetch_zeropage_addr(CPU *cpu, Bus *bus) { // fetches an address from the "zero page", ideal for frequently accessed data (variabkles/pointers)
    uint8_t zp = bus_read(bus, cpu->PC++);  // read address byte
    cpu->absolute_addr = zp; // effective address = 0x00 + zp
    return zp;
}


// populates opcode_table[] with the correct handler for for each supported instruction
void init_opcode_table(void) {
    // default to NOP (no operation)
    for (int i = 0; i < 256; ++i) {
        opcode_table[i].name = "NOP";
        opcode_table[i].operate = op_nop;
        opcode_table[i].addr_mode = AM_IMPLIED;
        opcode_table[i].cycles = 2;
    }

    // Implemented opcodes:

    // LDA copies a value into the A register
    // LDA immediate: 0xA9
    opcode_table[0xA9].name = "LDA";
    opcode_table[0xA9].operate = op_lda;
    opcode_table[0xA9].addr_mode = AM_IMMEDIATE;
    opcode_table[0xA9].cycles = 2;

    // LDA zero page: 0xA5
    opcode_table[0xA5].name = "LDA";
    opcode_table[0xA5].operate = op_lda;
    opcode_table[0xA5].addr_mode = AM_ZEROPAGE;
    opcode_table[0xA5].cycles = 3;

    // STA zero page 0x85, writes contents of A into memory
    opcode_table[0x85].name = "STA";
    opcode_table[0x85].operate = op_sta;
    opcode_table[0x85].addr_mode = AM_ZEROPAGE;
    opcode_table[0x85].cycles = 3;

    // INX implied 0xE8, increase X register
    opcode_table[0xE8].name = "INX";
    opcode_table[0xE8].operate = op_inx;
    opcode_table[0xE8].addr_mode = AM_IMPLIED;
    opcode_table[0xE8].cycles = 2;

    // NOP 0xEA (explicit), no operation
    opcode_table[0xEA].name = "NOP";
    opcode_table[0xEA].operate = op_nop;
    opcode_table[0xEA].addr_mode = AM_IMPLIED;
    opcode_table[0xEA].cycles = 2;
}

// Helpers to set flags new value based on last fetched result
// Updates the Zero (Z) and Negative (N) flags
static void update_zero_and_negative_flags(CPU *cpu, uint8_t val) {
    // Zero (set if value == 0)
    if (val == 0) cpu->P |= FLAG_Z;
    else cpu->P &= ~FLAG_Z;
    // Negative (set if bit 7 is 1)
    if (val & 0x80) cpu->P |= FLAG_N;
    else cpu->P &= ~FLAG_N;
}

// opcode implementations 

static void op_nop(CPU *cpu, Bus *bus) {
    // do absolutely nothing other than consume CPU cycles.
    (void)cpu; (void)bus;
}

// Loads a value from memory (or immediate) into the A register.
// Affects: Zero (Z), Negative (N)
static void op_lda(CPU *cpu, Bus *bus) {
    uint8_t opcode = bus_read(bus, cpu->PC - 1);
    Op *op = &opcode_table[opcode];

    if (op->addr_mode == AM_IMMEDIATE) {
        uint8_t val = fetch_immediate(cpu, bus);
        cpu->A = val;
        update_zero_and_negative_flags(cpu, cpu->A);
    } else if (op->addr_mode == AM_ZEROPAGE) {
        uint16_t addr = fetch_zeropage_addr(cpu, bus);
        uint8_t val = bus_read(bus, addr);
        cpu->A = val;
        update_zero_and_negative_flags(cpu, cpu->A);
    } else {
        // TODO: implement other address modes (absolute, indirect, etc.)
    }
}

// Writes the value in A into memory at the specified address.
static void op_sta(CPU *cpu, Bus *bus) {
    uint8_t opcode = bus_read(bus, cpu->PC - 1);
    Op *op = &opcode_table[opcode];

    if (op->addr_mode == AM_ZEROPAGE) {
        uint16_t addr = fetch_zeropage_addr(cpu, bus);
        bus_write(bus, addr, cpu->A);
    } else {
        // TODO: implement other addressing modes
    }
}

// Increments X by 1. Updates Zero and Negative flags.
// Affects: Z, N
static void op_inx(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->X = cpu->X + 1;
    update_zero_and_negative_flags(cpu, cpu->X);
}
