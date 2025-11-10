#ifndef OPCODES_H
#define OPCODES_H

#include "cpu.h"
#include "bus.h"
#include <stdint.h>

// opcodes are 1 byte instructions that tell the CPU what to do


// Addresing modes - defines how the operand is interpreted or fetched
typedef enum {
    AM_NONE,
    AM_IMMEDIATE,
    AM_ZEROPAGE,
    AM_ZEROPAGE_X,
    AM_ABSOLUTE,
    AM_RELATIVE,
    AM_IMPLIED
} addr_mode_t;

typedef struct Op {
    const char *name;
    void (*operate)(CPU*, struct Bus*);
    addr_mode_t addr_mode;
    uint8_t cycles;
} Op;

void init_opcode_table(void);
extern Op opcode_table[256];

#endif // OPCODES_H
