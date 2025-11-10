#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

// CPU manages registers and flags, interact with memory through the bus, maintains clock cycles to change its state

typedef struct CPU CPU;

struct CPU {
    // Registers
    uint8_t A;    // Accumulator
    uint8_t X;    // X index
    uint8_t Y;    // Y index
    uint16_t PC;  // Program Counter, points to next instruction
    uint8_t SP;   // Stack Pointer, points to current poisiton in stack
    uint8_t P;    // Status flags

    // cycles count
    uint64_t cycles; // 

    // pointer to memory/bus
    void *bus;

    // last fetched data/address (helper variables)
    uint8_t fetched; // most recently fetched data from memory
    uint16_t absolute_addr; // computed address from addressing modes 
    uint8_t addr_mode; // for debugging
};

// P flag bits
// Individual bits that store the status of recent operations and processor state
// https://www.nesdev.org/wiki/Status_flags

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

CPU *cpu_create(void *bus); // creates cpu, connects to bus
void cpu_reset(CPU *cpu); // sets registers, clears data, loads inital PC (program counter)
void cpu_step(CPU *cpu); // executes a single CPU cycle
void cpu_connect_bus(CPU *cpu, void *bus); // connects cpu to bus

// helpers to access status register
void set_flag(CPU *cpu, uint8_t flag, bool value);
bool get_flag(CPU *cpu, uint8_t flag);

#endif // CPU_H
