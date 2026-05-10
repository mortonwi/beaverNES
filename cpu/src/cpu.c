#include "../include/cpu.h"
#include "../include/bus.h"
#include "../include/opcodes.h"
#include <stdlib.h>
#include <stdio.h>
#include "mapper.h"

CPU *cpu_create(void *bus) {
    CPU *c = (CPU*)malloc(sizeof(CPU));
    if (!c) return NULL;

    c->A = 0;
    c->X = 0;
    c->Y = 0;

    c->PC = 0x0000;
    c->SP = 0xFD;
    c->P  = FLAG_U;

    c->page_crossed = 0;
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
    else cpu->P &= (uint8_t)~flag;
}

bool get_flag(CPU *cpu, uint8_t flag) {
    return (cpu->P & flag) != 0;
}

void cpu_write8(CPU *cpu, uint16_t addr, uint8_t val) {
    Bus *bus = (Bus*)cpu->bus;
    bus_write(bus, addr, val);
}

uint8_t cpu_read8(CPU *cpu, uint16_t addr) {
    Bus *bus = (Bus*)cpu->bus;
    return bus_read(bus, addr);
}

void cpu_reset(CPU *cpu) {
    // Reset vector at $FFFC/$FFFD
    uint16_t lo = cpu_read8(cpu, 0xFFFC);
    uint16_t hi = cpu_read8(cpu, 0xFFFD);
    cpu->PC = (uint16_t)((hi << 8) | lo);

    cpu->SP = 0xFD;
    cpu->P  = (uint8_t)(FLAG_U | FLAG_I);

    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;

    cpu->fetched = 0;
    cpu->absolute_addr = 0;
    cpu->addr_mode = AM_NONE;
    cpu->page_crossed = 0;

    // Reset consumes cycles
    cpu->cycles = 7;
}

/*
 * Execute ONE opcode and return TOTAL cycles consumed by it.
 * Total = base opcode cycles + any extra cycles your handlers add via cpu->cycles++
 */
int cpu_step(CPU *cpu) {
    Bus *bus = (Bus*)cpu->bus;

    // Handle mapper-generated IRQs. MMC3/mapper4 scanline IRQ support added by elvis-dev
    if (bus->rom && bus->rom->mapper && bus->rom->mapper->irq_pending && 
        bus->rom->mapper->irq_pending(bus->rom->mapper) && !get_flag(cpu, FLAG_I)) {
            
    cpu_irq(cpu);
    bus->rom->mapper->clear_irq(bus->rom->mapper);
    return 7;
    }

    uint64_t start_cycles = cpu->cycles;

    // Fetch opcode byte
    uint8_t opcode = bus_read(bus, cpu->PC++);
    Op *op = &opcode_table[opcode];

    // Your handlers rely on cpu->addr_mode
    cpu->addr_mode = (uint8_t)op->addr_mode;

    // Reset per-instruction penalty flag
    cpu->page_crossed = 0;

    // Execute instruction (handlers resolve operands internally)
    op->operate(cpu, bus);

    // Add base cycles
    cpu->cycles += (uint64_t)op->cycles;

    // Return cycles used by this instruction
    uint64_t used64 = cpu->cycles - start_cycles;
    int used = (int)used64;
    if (used < 1) used = 1;

    if (bus->rom) {
        bus->rom->cpu_cycle += used;
    }
    return used;
}

// --- Stack helper for interrupts ---
static void cpu_push(CPU *cpu, uint8_t value) {
    cpu_write8(cpu, (uint16_t)(0x0100 | cpu->SP), value);
    cpu->SP--;
}

// --- NMI handler ---
void cpu_nmi(CPU *cpu) {
    // Push PC high then low
    cpu_push(cpu, (uint8_t)((cpu->PC >> 8) & 0xFF));
    cpu_push(cpu, (uint8_t)(cpu->PC & 0xFF));

    // Push status with B cleared, U set
    set_flag(cpu, FLAG_B, false);
    set_flag(cpu, FLAG_U, true);
    set_flag(cpu, FLAG_I, true);
    cpu_push(cpu, cpu->P);

    // Jump to NMI vector at $FFFA/$FFFB
    uint16_t lo = cpu_read8(cpu, 0xFFFA);
    uint16_t hi = cpu_read8(cpu, 0xFFFB);
    cpu->PC = (uint16_t)((hi << 8) | lo);

    cpu->cycles += 7;
}

//IRQ handling added by elvis-dev for mapper 4 support.
void cpu_irq(CPU *cpu) {
    // Ignore IRQ if Interrupt Disable flag is set
    if (get_flag(cpu, FLAG_I)) {
        return;
    }

    // Push PC high then low
    cpu_push(cpu, (uint8_t)((cpu->PC >> 8) & 0xFF));
    cpu_push(cpu, (uint8_t)(cpu->PC & 0xFF));

    // Push status with B cleared, U set
    set_flag(cpu, FLAG_B, false);
    set_flag(cpu, FLAG_U, true);
    cpu_push(cpu, cpu->P);

    // Set Interrupt Disable flag
    set_flag(cpu, FLAG_I, true);

    // IRQ vector at $FFFE/$FFFF
    uint16_t lo = cpu_read8(cpu, 0xFFFE);
    uint16_t hi = cpu_read8(cpu, 0xFFFF);
    cpu->PC = (uint16_t)((hi << 8) | lo);

    cpu->cycles += 7;
}