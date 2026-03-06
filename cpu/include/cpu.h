#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

// CPU manages registers and flags, interacts with memory through the bus

typedef struct CPU CPU;

struct CPU {
    // Registers
    uint8_t A;    // Accumulator
    uint8_t X;    // X index
    uint8_t Y;    // Y index
    uint16_t PC;  // Program Counter
    uint8_t SP;   // Stack Pointer
    uint8_t P;    // Status flags

    // State
    uint8_t page_crossed;
    uint64_t cycles;

    // pointer to memory/bus
    void *bus;

    // last fetched data/address (helper variables)
    uint8_t fetched;
    uint16_t absolute_addr;
    uint8_t addr_mode;
};

// P flag bits
enum {
    FLAG_C = (1 << 0), // Carry
    FLAG_Z = (1 << 1), // Zero
    FLAG_I = (1 << 2), // Interrupt Disable
    FLAG_D = (1 << 3), // Decimal
    FLAG_B = (1 << 4), // Break (No CPU effect)
    FLAG_U = (1 << 5), // Unused, always 1 (No CPU effect)
    FLAG_V = (1 << 6), // Overflow
    FLAG_N = (1 << 7)  // Negative
};

CPU *cpu_create(void *bus);
void cpu_reset(CPU *cpu);

/*
 * Execute ONE instruction and return the number of CPU cycles it consumed.
 * This is required for correct PPU/APU timing.
 */
int cpu_step(CPU *cpu);

void cpu_connect_bus(CPU *cpu, void *bus);
void cpu_nmi(CPU *cpu);

void cpu_write8(CPU *cpu, uint16_t addr, uint8_t val);
uint8_t cpu_read8(CPU *cpu, uint16_t addr);

// helpers to access status register
void set_flag(CPU *cpu, uint8_t flag, bool value);
bool get_flag(CPU *cpu, uint8_t flag);

#endif // CPU_H