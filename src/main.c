#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "cpu.h"
#include "bus.h"
#include "memory.h"
#include "opcodes.h"

// main.c sets up the CPU, memory, and bus, loads a small program into memory,

// Load program using cpu_write to go through bus
static void load_program(CPU *cpu, uint16_t addr) {
    // LDA #$10
    cpu_write8(cpu, addr++, 0xA9);
    cpu_write8(cpu, addr++, 0x10);

    // STA $20
    cpu_write8(cpu, addr++, 0x85);
    cpu_write8(cpu, addr++, 0x20);

    // LDA #$01
    cpu_write8(cpu, addr++, 0xA9);
    cpu_write8(cpu, addr++, 0x01);

    // ADC $20
    cpu_write8(cpu, addr++, 0x65);
    cpu_write8(cpu, addr++, 0x20);

    // STA $21
    cpu_write8(cpu, addr++, 0x85);
    cpu_write8(cpu, addr++, 0x21);

    // INC $21
    cpu_write8(cpu, addr++, 0xE6);
    cpu_write8(cpu, addr++, 0x21);

    // LDY $21
    cpu_write8(cpu, addr++, 0xA4);
    cpu_write8(cpu, addr++, 0x21);

    // INY
    cpu_write8(cpu, addr++, 0xC8);

    // INX
    cpu_write8(cpu, addr++, 0xE8);

    // BRK
    cpu_write8(cpu, addr++, 0x00);
}

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
    // uint8_t program[] = {
    //     0xA9, 0x10, // LDA #$10
    //     0x85, 0x10, // STA $10
    //     0xA5, 0x10, // LDA $10
    //     0xE8,       // INX
    //     0xEA,       // NOP
    // };
    // uint8_t program[] = {
    //     0xa9, 0x10,     // LDA #$10     -> A = #$10 
    //     0x85, 0x20,     // STA $20      -> Memory Address $20 = #$10 
    //     0xa9, 0x01,     // LDA #$1      -> A = #$1
    //     0x65, 0x20,     // ADC $20      -> Reads Address $20 = #$10, Adds +$01, A = #$11
    //     0x85, 0x21,     // STA $21      -> Memory Address $21 = #$11
    //     0xe6, 0x21,     // INC $21      -> Reads Address $21 = #$11, INC +1, $21=#$12
    //     0xa4, 0x21,     // LDY $21      -> Y=#$12
    //     0xc8,           // INY          -> Y=#$13
    //     0xe8,           // INX          -> X=#$1
    //     0x00,           // BRK 
    // };

    load_program(cpu, load_addr);

    // set reset vector to point to 0x8000 to determine where to start executing code
    // point reset vector to 0x8000 where we loaded our program
    cpu_write8(cpu, 0xFFFC, (uint8_t)(load_addr & 0xFF));
    cpu_write8(cpu, 0xFFFD, (uint8_t)(load_addr >> 8));

    // Reset initializes the CPU state
    cpu_reset(cpu);

    // run 5 instructions (each call to cpu_step executes one opcode)
    for (int i = 0; i < 50; ++i) {
        printf("PC: %04X  A:%02X X:%02X Y:%02X P:%02X SP:%02X cycles:%llu\n",
               cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->P, cpu->SP, cpu->cycles);
        uint8_t opcode = bus_read(cpu->bus, cpu->PC); 
        if (opcode == 0x00) break; // stop on BRK
        cpu_step(cpu);
    }

    // print memory at zero page 0x10
    printf("Memory[0x10] = %02X\n", memory_read(mem, 0x10));
    printf("Memory[0x20] = %02X\n", memory_read(mem, 0x20));
    printf("Memory[0x21] = %02X\n", memory_read(mem, 0x21));

    // cleanup (in real emulator you would free)
    free(cpu);
    free(bus);
    free(mem);

    return 0;
}
