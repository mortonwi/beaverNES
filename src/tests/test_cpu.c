#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../include/memory.h"
#include "../../include/bus.h"
#include "../../include/cpu.h"
#include "../../include/opcodes.h"

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------


// Writes a reset vector into memory, ensures every test starts from a known PC
static void reset_cpu(CPU *cpu, Memory *mem, uint16_t pc) {
    mem->data[0xFFFC] = pc & 0xFF;
    mem->data[0xFFFD] = pc >> 8;
    cpu_reset(cpu);
}


static void run_single(CPU *cpu, Memory *mem,
                       uint16_t pc,
                       uint8_t opcode,
                       uint8_t op1,
                       uint8_t op2,
                       int operand_count)
{
    cpu->PC = pc;
    mem->data[pc] = opcode;
    if (operand_count > 0) mem->data[pc+1] = op1;
    if (operand_count > 1) mem->data[pc+2] = op2;
    cpu_step(cpu);
}

// ------------------------------------------------------------
// Individual opcode tests
// ------------------------------------------------------------

static void test_lda_immediate(CPU *cpu, Memory *mem) {
    cpu->A = 0;
    run_single(cpu, mem, 0x8000, 0xA9, 0x42, 0, 1);
    assert(cpu->A == 0x42);
    assert(!(cpu->P & FLAG_N));
    assert(!(cpu->P & FLAG_Z));
}

static void test_sta_zeropage(CPU *cpu, Memory *mem) {
    cpu->A = 0x55;
    mem->data[0x0040] = 0x00;
    run_single(cpu, mem, 0x8000, 0x85, 0x40, 0, 1);
    assert(mem->data[0x0040] == 0x55);
}

static void test_adc_immediate(CPU *cpu, Memory *mem) {
    cpu->A = 0x10;
    cpu->P = 0; // clear carry
    run_single(cpu, mem, 0x8000, 0x69, 0x20, 0, 1);
    assert(cpu->A == 0x30);
    assert(!(cpu->P & FLAG_C));
}

static void test_beq_taken(CPU *cpu, Memory *mem) {
    cpu->P = FLAG_Z; // BEQ should branch
    run_single(cpu, mem, 0x8000, 0xF0, 0x05, 0, 1);
    assert(cpu->PC == 0x8000 + 2 + 5);
}

static void test_beq_not_taken(CPU *cpu, Memory *mem) {
    cpu->P = 0; // zero flag clear
    run_single(cpu, mem, 0x8000, 0xF0, 0x05, 0, 1);
    assert(cpu->PC == 0x8002);
}

static void test_jsr_rts(CPU *cpu, Memory *mem) {
    // Program:
    // 8000: JSR 9000
    // 8003: LDA #0x77
    // 9000: LDA #0x55
    // 9002: RTS

    mem->data[0x8000] = 0x20; // JSR
    mem->data[0x8001] = 0x00;
    mem->data[0x8002] = 0x90;

    mem->data[0x8003] = 0xA9; // LDA #0x77
    mem->data[0x8004] = 0x77;

    mem->data[0x9000] = 0xA9; // LDA #0x55
    mem->data[0x9001] = 0x55;
    mem->data[0x9002] = 0x60; // RTS

    cpu->PC = 0x8000;

    cpu_step(cpu); // JSR
    assert(cpu->PC == 0x9000);

    cpu_step(cpu); // LDA #55
    assert(cpu->A == 0x55);

    cpu_step(cpu); // RTS
    assert(cpu->PC == 0x8003);

    cpu_step(cpu); // LDA #77
    assert(cpu->A == 0x77);
}

static void test_inc_zeropage(CPU *cpu, Memory *mem) {
    mem->data[0x0040] = 0x7F;
    run_single(cpu, mem, 0x8000, 0xE6, 0x40, 0, 1);
    assert(mem->data[0x0040] == 0x80);
    assert(cpu->P & FLAG_N);
}

static void test_ldx_immediate(CPU *cpu, Memory *mem) {
    cpu->X = 0;
    run_single(cpu, mem, 0x8000, 0xA2, 0x80, 0, 1);
    assert(cpu->X == 0x80);
    assert(cpu->P & FLAG_N);
}

static void test_ldy_immediate(CPU *cpu, Memory *mem) {
    cpu->Y = 0;
    run_single(cpu, mem, 0x8000, 0xA0, 0x00, 0, 1);
    assert(cpu->Y == 0x00);
    assert(cpu->P & FLAG_Z);
}

static void test_jmp_absolute(CPU *cpu, Memory *mem) {
    run_single(cpu, mem, 0x8000, 0x4C, 0x00, 0x90, 2);
    assert(cpu->PC == 0x9000);
}


// static void test_forced_failure(CPU *cpu, Memory *mem) {
//     // This will always fail
//     assert(cpu->A == 0xDE);
// }

// ------------------------------------------------------------
// Test harness
// ------------------------------------------------------------

typedef void (*TestFn)(CPU*, Memory*);

typedef struct {
    const char *name;
    TestFn fn;
} TestCase;

static TestCase tests[] = {
    {"LDA immediate",      test_lda_immediate},
    {"STA zeropage",       test_sta_zeropage},
    {"ADC immediate",      test_adc_immediate},
    {"BEQ taken",          test_beq_taken},
    {"BEQ not taken",      test_beq_not_taken},
    {"JSR/RTS",            test_jsr_rts},
    {"INC zeropage",       test_inc_zeropage},
    {"LDX immediate", test_ldx_immediate}, 
    {"LDY immediate", test_ldy_immediate},
    {"JMP absolute", test_jmp_absolute},
    //{"FORCED FAILURE", test_forced_failure},
};

int main(void) {
    init_opcode_table();

    int num_tests = sizeof(tests) / sizeof(TestCase);

    for (int i = 0; i < num_tests; i++) {
        Memory *mem = memory_create();
        Bus *bus = bus_create(mem);
        CPU *cpu = cpu_create(bus);
        bus->cpu = cpu;

        reset_cpu(cpu, mem, 0x8000);

        tests[i].fn(cpu, mem);

        free(cpu);
        free(bus);
        free(mem);

        printf("[OK] %s\n", tests[i].name);
    }

    printf("All tests passed.\n");
    return 0;
}
