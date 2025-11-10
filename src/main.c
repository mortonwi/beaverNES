#include <stdio.h>
#include <stdint.h>
#include "cpu.h"
#include "bus.h"
#include "memory.h"

// main.c sets up the CPU, memory, and bus, loads a small program into memory,

int main(void) {
    // initialize memory and bus
    Memory *mem = memory_create();
    Bus *bus = bus_create(mem);

    // initialize opcode table
    init_opcode_table();

    // create cpu and attach it to the bus
    CPU *cpu = cpu_create(bus);
    bus->cpu = cpu;

    // write a small program into memory starting at 0x8000 (typical reset vector location)
    // Example program:
    // LDA #$10     - load 0x10 (data, a literal value) into accumulator A
    // STA $10      - store A into zero page 0x10 (address)
    // LDA $10      - load from zero page 0x10 (address) into A
    // INX          - X = X + 1
    // NOP          - do nothing
    uint16_t load_addr = 0x8000;
    uint8_t program[] = {
        0xA9, 0x10, // LDA #$10
        0x85, 0x10, // STA $10
        0xA5, 0x10, // LDA $10
        0xE8,       // INX
        0xEA,       // NOP
    };

    // write bytes (above) into memory starting at 0x8000
    for (size_t i = 0; i < sizeof(program); ++i) {
        memory_write(mem, load_addr + i, program[i]);
    }

    // set reset vector to point to 0x8000 to determine where to start executing code
    // point reset vector to 0x8000 where we loaded our program
    memory_write(mem, 0xFFFC, (uint8_t)(load_addr & 0xFF));
    memory_write(mem, 0xFFFD, (uint8_t)(load_addr >> 8));

    // Reset initializes the CPU state
    cpu_reset(cpu);

    // run 5 instructions (each call to cpu_step executes one opcode)
    for (int i = 0; i < 5; ++i) {
        printf("PC: %04X  A:%02X X:%02X Y:%02X P:%02X SP:%02X cycles:%llu\n",
               cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->P, cpu->SP, cpu->cycles);
        cpu_step(cpu);
    }

    // print memory at zero page 0x10
    printf("Memory[0x10] = %02X\n", memory_read(mem, 0x10));

    // cleanup (in real emulator you would free)
    free(cpu);
    free(bus);
    free(mem);

    return 0;
}
