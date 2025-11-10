#include "cpu.h"
#include "bus.h"
#include "opcodes.h"
#include <stdlib.h>
#include <stdio.h>

CPU *cpu_create(void *bus) {
    CPU *c = (CPU*)malloc(sizeof(CPU));
    if (!c) return NULL;
    c->A = c->X = c->Y = 0;
    c->PC = 0x0000;
    c->SP = 0xFD; // typical reset value
    c->P = FLAG_U; // unused bit set
    c->cycles = 0;
    c->bus = bus;
    c->fetched = 0;
    c->absolute_addr = 0;
    c->addr_mode = AM_NONE;
    return c;
}

void cpu_connect_bus(CPU *cpu, void *bus) {
    cpu->bus = bus;
}

void set_flag(CPU *cpu, uint8_t flag, bool value) {
    if (value) cpu->P |= flag;
    else cpu->P &= ~flag;
}

bool get_flag(CPU *cpu, uint8_t flag) {
    return (cpu->P & flag) != 0;
}

static uint8_t read8(CPU *cpu, uint16_t addr) {
    Bus *bus = (Bus*)cpu->bus;
    return bus_read(bus, addr);
}

static void write8(CPU *cpu, uint16_t addr, uint8_t val) {
    Bus *bus = (Bus*)cpu->bus;
    bus_write(bus, addr, val);
}

void cpu_reset(CPU *cpu) {
    // typical reset vector at 0xFFFC
    uint16_t lo = read8(cpu, 0xFFFC);
    uint16_t hi = read8(cpu, 0xFFFD);
    cpu->PC = (hi << 8) | lo;
    cpu->SP = 0xFD;
    cpu->P = FLAG_U;
    cpu->A = cpu->X = cpu->Y = 0;
    cpu->cycles = 7; // reset takes time
}

void cpu_step(CPU *cpu) {
    Bus *bus = (Bus*)cpu->bus;
    // fetch opcode
    uint8_t opcode = bus_read(bus, cpu->PC++);
    Op *op = &opcode_table[opcode];

    // Save the addressing mode for debugging
    cpu->addr_mode = (uint8_t)op->addr_mode;

    // Execute
    op->operate(cpu, bus);

    // cycle accounting
    cpu->cycles += op->cycles;
}

