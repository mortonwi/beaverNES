#include "opcodes.h"
#include <stdio.h>
#include <string.h>

// 256 possible opcodes in an 8-bit system
Op opcode_table[256];

// forward declarations of a single 6502 core instruction
// https://www.masswerk.at/6502/6502_instruction_set.html

// illegal / unimplemented opcode handler
static void op_XXX();

// 1. Transfers: load, store, interresiter transfer
static void op_lda(CPU *cpu, Bus *bus);
static void op_ldx(CPU *cpu, Bus *bus);
static void op_ldy(CPU *cpu, Bus *bus);
static void op_sta(CPU *cpu, Bus *bus);
static void op_stx(CPU *cpu, Bus *bus);
static void op_sty(CPU *cpu, Bus *bus);
static void op_tax(CPU *cpu, Bus *bus);
static void op_tay(CPU *cpu, Bus *bus);
static void op_tsx(CPU *cpu, Bus *bus);
static void op_txa(CPU *cpu, Bus *bus);
static void op_txs(CPU *cpu, Bus *bus);
static void op_tya(CPU *cpu, Bus *bus);

// 2. Stack Operations
static void op_pha(CPU *cpu, Bus *bus);
static void op_php(CPU *cpu, Bus *bus);
static void op_pla(CPU *cpu, Bus *bus);
static void op_plp(CPU *cpu, Bus *bus);

// 3. Increments / Decrements
static void op_dec(CPU *cpu, Bus *bus);
static void op_dex(CPU *cpu, Bus *bus);
static void op_dey(CPU *cpu, Bus *bus);
static void op_inc(CPU *cpu, Bus *bus);
static void op_inx(CPU *cpu, Bus *bus);
static void op_iny(CPU *cpu, Bus *bus);

// 4. Arithmetic
static void op_adc(CPU *cpu, Bus *bus);
static void op_sbc(CPU *cpu, Bus *bus);

// 5. Logical
static void op_and(CPU *cpu, Bus *bus);
static void op_eor(CPU *cpu, Bus *bus);
static void op_ora(CPU *cpu, Bus *bus);

// 6. Shift & Rotate
static void op_asl(CPU *cpu, Bus *bus);
static void op_lsr(CPU *cpu, Bus *bus);
static void op_rol(CPU *cpu, Bus *bus);
static void op_ror(CPU *cpu, Bus *bus);

// 7. Flag Operations
static void op_clc(CPU *cpu, Bus *bus);
static void op_cld(CPU *cpu, Bus *bus);
static void op_cli(CPU *cpu, Bus *bus);
static void op_clv(CPU *cpu, Bus *bus);
static void op_sec(CPU *cpu, Bus *bus);
static void op_sed(CPU *cpu, Bus *bus);
static void op_sei(CPU *cpu, Bus *bus);

// 8. Comparisons
static void op_cmp(CPU *cpu, Bus *bus);
static void op_cpx(CPU *cpu, Bus *bus);
static void op_cpy(CPU *cpu, Bus *bus);

// 9. Bit Test
static void op_bit(CPU *cpu, Bus *bus);

// 10. Branches
static void op_bcc(CPU *cpu, Bus *bus);
static void op_bcs(CPU *cpu, Bus *bus);
static void op_beq(CPU *cpu, Bus *bus);
static void op_bmi(CPU *cpu, Bus *bus);
static void op_bne(CPU *cpu, Bus *bus);
static void op_bpl(CPU *cpu, Bus *bus);
static void op_bvc(CPU *cpu, Bus *bus);
static void op_bvs(CPU *cpu, Bus *bus);

// 11. Jumps & Subroutines
static void op_jmp(CPU *cpu, Bus *bus);
static void op_jsr(CPU *cpu, Bus *bus);
static void op_rts(CPU *cpu, Bus *bus);

// 12. Interupts
static void op_brk(CPU *cpu, Bus *bus);
static void op_rti(CPU *cpu, Bus *bus);

// no‑operation
static void op_nop(CPU *cpu, Bus *bus);


static const OpSpec op_specs[] = {
    {0x00,"BRK",op_brk,AM_IMPLIED,7},   {0x01,"ORA",op_ora,AM_INDIRECT_X,6},     {0x02,"???",op_XXX,AM_IMPLIED,2},      {0x03,"???",op_XXX,AM_IMPLIED,8},   {0x04,"???",op_XXX,AM_IMPLIED,3},    {0x05,"ORA",op_ora,AM_ZEROPAGE,3},     {0x06,"ASL",op_asl,AM_ZEROPAGE,5},       {0x07,"???",op_XXX,AM_IMPLIED,5},   {0x08,"PHP",op_php,AM_IMPLIED,3},     {0x09,"ORA",op_ora,AM_IMMEDIATE,2},     {0x0A,"ASL",op_asl,AM_ACCUMULATOR,2},        {0x0B,"???",op_XXX,AM_IMPLIED,2},   {0x0C,"???",op_XXX,AM_IMPLIED,4},     {0x0D,"ORA",op_ora,AM_ABSOLUTE,4},     {0x0E,"ASL",op_asl,AM_ABSOLUTE,6},      {0x0F,"???",op_XXX,AM_IMPLIED,6}, 
    {0x10,"BPL",op_bpl,AM_RELATIVE,2},  {0x11,"ORA",op_ora,AM_INDIRECT_Y,5},     {0x12,"???",op_XXX,AM_IMPLIED,2},      {0x13,"???",op_XXX,AM_IMPLIED,8},   {0x14,"???",op_XXX,AM_IMPLIED,4},    {0x15,"ORA",op_ora,AM_ZEROPAGE_X,4},   {0x16,"ASL",op_asl,AM_ZEROPAGE_X,6},     {0x17,"???",op_XXX,AM_IMPLIED,6},   {0x18,"CLC",op_clc,AM_IMPLIED,2},     {0x19,"ORA",op_ora,AM_ABSOLUTE_Y,4},    {0x1A,"???",op_XXX,AM_IMPLIED,2},            {0x1B,"???",op_XXX,AM_IMPLIED,7},   {0x1C,"???",op_XXX,AM_IMPLIED,4},     {0x1D,"ORA",op_ora,AM_ABSOLUTE_X,4},   {0x1E,"ASL",op_asl,AM_ABSOLUTE_X,7},    {0x1F,"???",op_XXX,AM_IMPLIED,7},
    {0x20,"JSR",op_jsr,AM_ABSOLUTE,6},  {0x21,"AND",op_and,AM_INDIRECT_X,6},     {0x22,"???",op_XXX,AM_IMPLIED,2},      {0x23,"???",op_XXX,AM_IMPLIED,8},   {0x24,"BIT",op_bit,AM_ZEROPAGE,3},   {0x25,"AND",op_and,AM_ZEROPAGE,3},     {0x26,"ROL",op_rol,AM_ZEROPAGE,5},       {0x27,"???",op_XXX,AM_IMPLIED,5},   {0x28,"PLP",op_plp,AM_IMPLIED,4},     {0x29,"AND",op_and,AM_IMMEDIATE,2},     {0x2A,"ROL",op_rol,AM_ACCUMULATOR,2},        {0x2B,"???",op_XXX,AM_IMPLIED,2},   {0x2C,"BIT",op_bit,AM_ABSOLUTE,4},    {0x2D,"AND",op_and,AM_ABSOLUTE,4},     {0x2E,"ROL",op_rol,AM_ABSOLUTE,6},      {0x2F,"???",op_XXX,AM_IMPLIED,6},
    {0x30,"BMI",op_bmi,AM_RELATIVE,2},  {0x31,"AND",op_and,AM_INDIRECT_Y,5},     {0x32,"???",op_XXX,AM_IMPLIED,2},      {0x33,"???",op_XXX,AM_IMPLIED,8},   {0x34,"???",op_XXX,AM_IMPLIED,4},    {0x35,"AND",op_and,AM_ZEROPAGE_X,4},   {0x36,"ROL",op_rol,AM_ZEROPAGE_X,6},     {0x37,"???",op_XXX,AM_IMPLIED,6},   {0x38,"SEC",op_sec,AM_IMPLIED,2},     {0x39,"AND",op_and,AM_ABSOLUTE_Y,4},    {0x3A,"???",op_XXX,AM_IMPLIED,2},            {0x3B,"???",op_XXX,AM_IMPLIED,7},   {0x3C,"???",op_XXX,AM_IMPLIED,4},     {0x3D,"AND",op_and,AM_ABSOLUTE_X,4},   {0x3E,"ROL",op_rol,AM_ABSOLUTE_X,7},    {0x3F,"???",op_XXX,AM_IMPLIED,7},
    {0x40,"RTI",op_rti,AM_IMPLIED,6},   {0x41,"EOR",op_eor,AM_INDIRECT_X,6},     {0x42,"???",op_XXX,AM_IMPLIED,2},      {0x43,"???",op_XXX,AM_IMPLIED,8},   {0x44,"???",op_XXX,AM_IMPLIED,3},    {0x45,"EOR",op_eor,AM_ZEROPAGE,3},     {0x46,"LSR",op_lsr,AM_ZEROPAGE,5},       {0x47,"???",op_XXX,AM_IMPLIED,5},   {0x48,"PHA",op_pha,AM_IMPLIED,3},     {0x49,"EOR",op_eor,AM_IMMEDIATE,2},     {0x4A,"LSR",op_lsr,AM_ACCUMULATOR,2},        {0x4B,"???",op_XXX,AM_IMPLIED,2},   {0x4C,"JMP",op_jmp,AM_ABSOLUTE,3},    {0x4D,"EOR",op_eor,AM_ABSOLUTE,4},     {0x4E,"LSR",op_lsr,AM_ABSOLUTE,6},      {0x4F,"???",op_XXX,AM_IMPLIED,6},
    {0x50,"BVC",op_bvc,AM_RELATIVE,2},  {0x51,"EOR",op_eor,AM_INDIRECT_Y,5},     {0x52,"???",op_XXX,AM_IMPLIED,2},      {0x53,"???",op_XXX,AM_IMPLIED,8},   {0x54,"???",op_XXX,AM_IMPLIED,4},    {0x55,"EOR",op_eor,AM_ZEROPAGE_X,4},   {0x56,"LSR",op_lsr,AM_ZEROPAGE_X,6},     {0x57,"???",op_XXX,AM_IMPLIED,6},   {0x58,"CLI",op_cli,AM_IMPLIED,2},     {0x59,"EOR",op_eor,AM_ABSOLUTE_Y,4},    {0x5A,"???",op_XXX,AM_IMPLIED,2},            {0x5B,"???",op_XXX,AM_IMPLIED,7},   {0x5C,"???",op_XXX,AM_IMPLIED,4},     {0x5D,"EOR",op_eor,AM_ABSOLUTE_X,4},   {0x5E,"LSR",op_lsr,AM_ABSOLUTE_X,7},    {0x5F,"???",op_XXX,AM_IMPLIED,7},
    {0x60,"RTS",op_rts,AM_IMPLIED,6},   {0x61,"ADC",op_adc,AM_INDIRECT_X,6},     {0x62,"???",op_XXX,AM_IMPLIED,2},      {0x63,"???",op_XXX,AM_IMPLIED,8},   {0x64,"???",op_XXX,AM_IMPLIED,3},    {0x65,"ADC",op_adc,AM_ZEROPAGE,3},     {0x66,"ROR",op_ror,AM_ZEROPAGE,5},       {0x67,"???",op_XXX,AM_IMPLIED,5},   {0x68,"PLA",op_pla,AM_IMPLIED,4},     {0x69,"ADC",op_adc,AM_IMMEDIATE,2},     {0x6A,"ROR",op_ror,AM_ACCUMULATOR,2},        {0x6B,"???",op_XXX,AM_IMPLIED,2},   {0x6C,"JMP",op_jmp,AM_INDIRECT,5},    {0x6D,"ADC",op_adc,AM_ABSOLUTE,4},     {0x6E,"ROR",op_ror,AM_ABSOLUTE,6},      {0x6F,"???",op_XXX,AM_IMPLIED,6},
    {0x70,"BVS",op_bvs,AM_RELATIVE,2},  {0x71,"ADC",op_adc,AM_INDIRECT_Y,5},     {0x72,"???",op_XXX,AM_IMPLIED,2},      {0x73,"???",op_XXX,AM_IMPLIED,8},   {0x74,"???",op_XXX,AM_IMPLIED,4},    {0x75,"ADC",op_adc,AM_ZEROPAGE_X,4},   {0x76,"ROR",op_ror,AM_ZEROPAGE_X,6},     {0x77,"???",op_XXX,AM_IMPLIED,6},   {0x78,"SEI",op_sei,AM_IMPLIED,2},     {0x79,"ADC",op_adc,AM_ABSOLUTE_Y,4},    {0x7A,"???",op_XXX,AM_IMPLIED,2},            {0x7B,"???",op_XXX,AM_IMPLIED,7},   {0x7C,"???",op_XXX,AM_IMPLIED,4},     {0x7D,"ADC",op_adc,AM_ABSOLUTE_X,4},   {0x7E,"ROR",op_ror,AM_ABSOLUTE_X,7},    {0x7F,"???",op_XXX,AM_IMPLIED,7},
    {0x80,"???",op_XXX,AM_IMPLIED,2},   {0x81,"STA",op_sta,AM_INDIRECT_X,6},     {0x82,"???",op_XXX,AM_IMPLIED,2},      {0x83,"???",op_XXX,AM_IMPLIED,6},   {0x84,"STY",op_sty,AM_ZEROPAGE,3},   {0x85,"STA",op_sta,AM_ZEROPAGE,3},     {0x86,"STX",op_stx,AM_ZEROPAGE,3},       {0x87,"???",op_XXX,AM_IMPLIED,3},   {0x88,"DEY",op_dey,AM_IMPLIED,2},     {0x89,"???",op_XXX,AM_IMPLIED,2},       {0x8A,"TXA",op_txa,AM_IMPLIED,2},            {0x8B,"???",op_XXX,AM_IMPLIED,2},   {0x8C,"STY",op_sty,AM_ABSOLUTE,4},    {0x8D,"STA",op_sta,AM_ABSOLUTE,4},     {0x8E,"STX",op_stx,AM_ABSOLUTE,4},      {0x8F,"???",op_XXX,AM_IMPLIED,4},
    {0x90,"BCC",op_bcc,AM_RELATIVE,2},  {0x91,"STA",op_sta,AM_INDIRECT_Y,6},     {0x92,"???",op_XXX,AM_IMPLIED,2},      {0x93,"???",op_XXX,AM_IMPLIED,6},   {0x94,"STY",op_sty,AM_ZEROPAGE_X,4}, {0x95,"STA",op_sta,AM_ZEROPAGE_X,4},   {0x96,"STX",op_stx,AM_ZEROPAGE_Y,4},     {0x97,"???",op_XXX,AM_IMPLIED,4},   {0x98,"TYA",op_tya,AM_IMPLIED,2},     {0x99,"STA",op_sta,AM_ABSOLUTE_Y,5},    {0x9A,"TXS",op_txs,AM_IMPLIED,2},            {0x9B,"???",op_XXX,AM_IMPLIED,5},   {0x9C,"???",op_XXX,AM_IMPLIED,5},     {0x9D,"STA",op_sta,AM_ABSOLUTE_X,5},   {0x9E,"???",op_XXX,AM_ABSOLUTE_X,5},    {0x9F,"???",op_XXX,AM_IMPLIED,5},
    {0xA0,"LDY",op_ldy,AM_IMMEDIATE,2}, {0xA1,"LDA",op_lda,AM_INDIRECT_X,6},     {0xA2,"LDX",op_ldx,AM_IMMEDIATE,2},    {0xA3,"???",op_XXX,AM_IMPLIED,6},   {0xA4,"LDY",op_ldy,AM_ZEROPAGE,3},   {0xA5,"LDA",op_lda,AM_ZEROPAGE,3},     {0xA6,"LDX",op_ldx,AM_ZEROPAGE,3},       {0xA7,"???",op_XXX,AM_IMPLIED,3},   {0xA8,"TAY",op_tay,AM_IMPLIED,2},     {0xA9,"LDA",op_lda,AM_IMMEDIATE,2},     {0xAA,"TAX",op_tax,AM_IMPLIED,2},            {0xAB,"???",op_XXX,AM_IMPLIED,2},   {0xAC,"LDY",op_ldy,AM_ABSOLUTE,4},    {0xAD,"LDA",op_lda,AM_ABSOLUTE,4},     {0xAE,"LDX",op_ldx,AM_ABSOLUTE,4},      {0xAF,"???",op_XXX,AM_IMPLIED,4},
    {0xB0,"BCS",op_bcs,AM_RELATIVE,2},  {0xB1,"LDA",op_lda,AM_INDIRECT_Y,5},     {0xB2,"???",op_XXX,AM_IMPLIED,2},      {0xB3,"???",op_XXX,AM_IMPLIED,5},   {0xB4,"LDY",op_ldy,AM_ZEROPAGE_X,4}, {0xB5,"LDA",op_lda,AM_ZEROPAGE_X,4},   {0xB6,"LDX",op_ldx,AM_ZEROPAGE_Y,4},     {0xB7,"???",op_XXX,AM_IMPLIED,4},   {0xB8,"CLV",op_clv,AM_IMPLIED,2},     {0xB9,"LDA",op_lda,AM_ABSOLUTE_Y,4},    {0xBA,"TSX",op_tsx,AM_IMPLIED,2},            {0xBB,"???",op_XXX,AM_IMPLIED,4},   {0xBC,"LDY",op_ldy,AM_ABSOLUTE_X,4},  {0xBD,"LDA",op_lda,AM_ABSOLUTE_X,4},   {0xBE,"LDX",op_ldx,AM_ABSOLUTE_Y,4},    {0xBF,"???",op_XXX,AM_IMPLIED,4},
    {0xC0,"CPY",op_cpy,AM_IMMEDIATE,2}, {0xC1,"CMP",op_cmp,AM_INDIRECT_X,6},     {0xC2,"???",op_XXX,AM_IMPLIED,2},      {0xC3,"???",op_XXX,AM_IMPLIED,8},   {0xC4,"CPY",op_cpy,AM_ZEROPAGE,3},   {0xC5,"CMP",op_cmp,AM_ZEROPAGE,3},     {0xC6,"DEC",op_dec,AM_ZEROPAGE,5},       {0xC7,"???",op_XXX,AM_IMPLIED,5},   {0xC8,"INY",op_iny,AM_IMPLIED,2},     {0xC9,"CMP",op_cmp,AM_IMMEDIATE,2},     {0xCA,"DEX",op_dex,AM_IMPLIED,2},            {0xCB,"???",op_XXX,AM_IMPLIED,2},   {0xCC,"CPY",op_cpy,AM_ABSOLUTE,4},    {0xCD,"CMP",op_cmp,AM_ABSOLUTE,4},     {0xCE,"DEC",op_dec,AM_ABSOLUTE,6},      {0xCF,"???",op_XXX,AM_IMPLIED,6},
    {0xD0,"BNE",op_bne,AM_RELATIVE,2},  {0xD1,"CMP",op_cmp,AM_INDIRECT_Y,5},     {0xD2,"???",op_XXX,AM_IMPLIED,2},      {0xD3,"???",op_XXX,AM_IMPLIED,8},   {0xD4,"???",op_XXX,AM_IMPLIED,4},    {0xD5,"CMP",op_cmp,AM_ZEROPAGE_X,4},   {0xD6,"DEC",op_dec,AM_ZEROPAGE_X,6},     {0xD7,"???",op_XXX,AM_IMPLIED,6},   {0xD8,"CLD",op_cld,AM_IMPLIED,2},     {0xD9,"CMP",op_cmp,AM_ABSOLUTE_Y,4},    {0xDA,"???",op_XXX,AM_IMPLIED,2},            {0xDB,"???",op_XXX,AM_IMPLIED,7},   {0xDC,"???",op_XXX,AM_IMPLIED,4},     {0xDD,"CMP",op_cmp,AM_ABSOLUTE_X,4},   {0xDE,"DEC",op_dec,AM_ABSOLUTE_X,7},    {0xDF,"???",op_XXX,AM_IMPLIED,7},
    {0xE0,"CPX",op_cpx,AM_IMMEDIATE,2}, {0xE1,"SBC",op_sbc,AM_INDIRECT_X,6},     {0xE2,"???",op_XXX,AM_IMPLIED,2},      {0xE3,"???",op_XXX,AM_IMPLIED,8},   {0xE4,"CPX",op_cpx,AM_ZEROPAGE,3},   {0xE5,"SBC",op_sbc,AM_ZEROPAGE,3},     {0xE6,"INC",op_inc,AM_ZEROPAGE,5},       {0xE7,"???",op_XXX,AM_IMPLIED,5},   {0xE8,"INX",op_inx,AM_IMPLIED,2},     {0xE9,"SBC",op_sbc,AM_IMMEDIATE,2},     {0xEA,"NOP",op_nop,AM_IMPLIED,2},            {0xEB,"???",op_XXX,AM_IMPLIED,2},   {0xEC,"CPX",op_cpx,AM_ABSOLUTE,4},    {0xED,"SBC",op_sbc,AM_ABSOLUTE,4},     {0xEE,"INC",op_inc,AM_ABSOLUTE,6},      {0xEF,"???",op_XXX,AM_IMPLIED,6},
    {0xF0,"BEQ",op_beq,AM_RELATIVE,2},  {0xF1,"SBC",op_sbc,AM_INDIRECT_Y,5},     {0xF2,"???",op_XXX,AM_IMPLIED,2},      {0xF3,"???",op_XXX,AM_IMPLIED,8},   {0xF4,"???",op_XXX,AM_IMPLIED,4},    {0xF5,"SBC",op_sbc,AM_ZEROPAGE_X,4},   {0xF6,"INC",op_inc,AM_ZEROPAGE_X,6},     {0xF7,"???",op_XXX,AM_IMPLIED,6},   {0xF8,"SED",op_sed,AM_IMPLIED,2},     {0xF9,"SBC",op_sbc,AM_ABSOLUTE_Y,4},    {0xFA,"???",op_XXX,AM_IMPLIED,2},            {0xFB,"???",op_XXX,AM_IMPLIED,7},   {0xFC,"???",op_XXX,AM_IMPLIED,4},     {0xFD,"SBC",op_sbc,AM_ABSOLUTE_X,4},   {0xFE,"INC",op_inc,AM_ABSOLUTE_X,7},    {0xFF,"???",op_XXX,AM_IMPLIED,7},
};

// addressing mode helpers to handle how operands are fetched

static uint8_t fetch_immediate(CPU *cpu, Bus *bus) {
    uint8_t v = bus_read(bus, cpu->PC++); // read byte at PC, then increment PC
    cpu->fetched = v; // store for logging n debugging
    return v; 
}

static uint8_t fetch_accumulator(CPU *cpu) {
    cpu->fetched = cpu->A; // operand is the accumulator
    return cpu->A;
}

static uint16_t fetch_zeropage_addr(CPU *cpu, Bus *bus) { // fetches an address from the "zero page", ideal for frequently accessed data (variabkles/pointers)
    uint8_t zp = bus_read(bus, cpu->PC++);  // read address byte
    cpu->absolute_addr = zp; // effective address = 0x00 + zp
    return zp;
}

static uint16_t fetch_zeropage_x_addr(CPU *cpu, Bus *bus) {
    uint8_t base = bus_read(bus, cpu->PC++);
    uint8_t addr = (base + cpu->X) & 0xFF; // zero page + X
    cpu->absolute_addr = addr;
    return addr;
}

static uint16_t fetch_zeropage_y_addr(CPU *cpu, Bus *bus) {
    uint8_t base = bus_read(bus, cpu->PC++);
    uint8_t addr = (base + cpu->Y) & 0xFF; // zero page + Y
    cpu->absolute_addr = addr;
    return addr;
}

static uint16_t fetch_absolute_addr(CPU *cpu, Bus *bus) {
    uint8_t lo = bus_read(bus, cpu->PC++);
    uint8_t hi = bus_read(bus, cpu->PC++);
    uint16_t addr = (hi << 8) | lo; // combines and forms a full 16 bit address
    cpu->absolute_addr = addr; 
    return addr;
}

static uint16_t fetch_absolute_x_addr(CPU *cpu, Bus *bus) {
    uint16_t base = fetch_absolute_addr(cpu, bus);
    uint16_t addr = base + cpu->X;

    cpu->page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
    cpu->absolute_addr = addr;

    return addr;
}

static uint16_t fetch_absolute_y_addr(CPU *cpu, Bus *bus) {
    uint16_t base = fetch_absolute_addr(cpu, bus);
    uint16_t addr = base + cpu->Y;

    cpu->page_crossed = ((base & 0xFF00) != (addr & 0xFF00));
    cpu->absolute_addr = addr;

    return addr;
}


static uint16_t fetch_indirect_addr(CPU *cpu, Bus *bus) {
    uint8_t lo_ptr = bus_read(bus, cpu->PC++);
    uint8_t hi_ptr = bus_read(bus, cpu->PC++);
    uint16_t ptr = (hi_ptr << 8) | lo_ptr;  // combined 16 bit pointer, 

    uint8_t lo = bus_read(bus, ptr);
    uint8_t hi = bus_read(bus, (ptr & 0xFF00) | ((ptr + 1) & 0x00FF));

    return (hi << 8) | lo; // reads actual address from the pointer
}

static uint16_t fetch_indirect_x_addr(CPU *cpu, Bus *bus) { // indexed indrect x: add x, read pointer
    uint8_t zp = bus_read(bus, cpu->PC++);
    uint8_t ptr = (zp + cpu->X) & 0xFF; //zp base address + X

    uint8_t lo = bus_read(bus, ptr);
    uint8_t hi = bus_read(bus, (ptr + 1) & 0xFF);

    uint16_t addr = (hi << 8) | lo;
    cpu->absolute_addr = addr;
    return addr;
}

static uint16_t fetch_indirect_y_addr(CPU *cpu, Bus *bus) {
    // Read zero-page pointer
    uint8_t zp = bus_read(bus, cpu->PC++);

    // Read the 16-bit base address from zero page (wraps at 0xFF)
    uint8_t lo = bus_read(bus, zp);
    uint8_t hi = bus_read(bus, (zp + 1) & 0xFF);

    uint16_t base = (hi << 8) | lo;

    // Add Y to form the effective address
    uint16_t addr = base + cpu->Y;

    // Detect page crossing (base and addr in different 256-byte pages)
    cpu->page_crossed = ((base & 0xFF00) != (addr & 0xFF00));

    // Store effective address
    cpu->absolute_addr = addr;

    return addr;
}

static uint16_t resolve_address(CPU *cpu, Bus *bus, addr_mode_t mode) {
    switch (mode) {
        case AM_IMMEDIATE:
            fetch_immediate(cpu, bus);
            return 0;

        case AM_ACCUMULATOR:
            fetch_accumulator(cpu);
            return 0;

        case AM_ZEROPAGE:
            return fetch_zeropage_addr(cpu, bus);

        case AM_ZEROPAGE_X:
            return fetch_zeropage_x_addr(cpu, bus);

        case AM_ZEROPAGE_Y:
            return fetch_zeropage_y_addr(cpu, bus);

        case AM_ABSOLUTE:
            return fetch_absolute_addr(cpu, bus);

        case AM_ABSOLUTE_X:
            return fetch_absolute_x_addr(cpu, bus);

        case AM_ABSOLUTE_Y:
            return fetch_absolute_y_addr(cpu, bus);

        case AM_INDIRECT:
            return fetch_indirect_addr(cpu, bus);

        case AM_INDIRECT_X:
            return fetch_indirect_x_addr(cpu, bus);

        case AM_INDIRECT_Y:
            return fetch_indirect_y_addr(cpu, bus);

        case AM_RELATIVE:
            return (int8_t)fetch_immediate(cpu, bus);

        case AM_IMPLIED:
        case AM_NONE:
        default:
            return 0;
    }
}

// populates opcode_table[] with the correct handler for for each supported instruction
void init_opcode_table(void) {
    // default: NOP
    for (int i = 0; i < 256; ++i) {
        opcode_table[i].name      = "NOP";
        opcode_table[i].operate   = op_nop;
        opcode_table[i].addr_mode = AM_IMPLIED;
        opcode_table[i].cycles    = 2;
    }

    // overlay implemented opcodes
    size_t count = sizeof(op_specs) / sizeof(op_specs[0]);
    for (size_t i = 0; i < count; ++i) {
        uint8_t code = op_specs[i].code;
        opcode_table[code].name      = op_specs[i].name;
        opcode_table[code].operate   = op_specs[i].operate;
        opcode_table[code].addr_mode = op_specs[i].mode;
        opcode_table[code].cycles    = op_specs[i].cycles;
    }
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

static void op_XXX() {
    // do absolutely nothing, captures illegal opcodes
}

static void op_nop(CPU *cpu, Bus *bus) {
    // do absolutely nothing other than consume CPU cycles.
    (void)cpu; (void)bus;
}

// 1. Transfers

// Loads a value from memory (or immediate) into the A register.
// Affects: Zero (Z), Negative (N)
static void op_lda(CPU *cpu, Bus *bus) {

    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Apply page-crossing penalty for read instructions 
    if (cpu->page_crossed && 
        (cpu->addr_mode == AM_ABSOLUTE_X || 
         cpu->addr_mode == AM_ABSOLUTE_Y || 
         cpu->addr_mode == AM_INDIRECT_Y)) { 
        cpu->cycles++; 
    }

    // Fetch the value based on addressing mode 
    uint8_t value; 
    if (cpu->addr_mode == AM_IMMEDIATE) { 
        value = cpu->fetched; 
    } else { 
        value = bus_read(bus, addr);
    }

    cpu->A = value;
    update_zero_and_negative_flags(cpu, cpu->A);
}

static void op_ldx(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Page-crossing penalty for LDX absolute,Y
    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_Y)) {
        cpu->cycles++;
    }

    uint8_t value = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    cpu->X = value;
    update_zero_and_negative_flags(cpu, cpu->X);
}


static void op_ldy(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Page-crossing penalty for LDY absolute,X
    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X)) {
        cpu->cycles++;
    }

    uint8_t value = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    cpu->Y = value;
    update_zero_and_negative_flags(cpu, cpu->Y);
}


// Writes the value in A into memory at the specified address.
static void op_sta(CPU *cpu, Bus *bus) {

    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
    bus_write(bus, addr, cpu->A);
}

static void op_stx(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
    bus_write(bus, addr, cpu->X);
}

static void op_sty(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
    bus_write(bus, addr, cpu->Y);
}

static void op_tsx(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->X = cpu->SP;
    update_zero_and_negative_flags(cpu, cpu->X);
}

static void op_tax(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->X = cpu->A;
    update_zero_and_negative_flags(cpu, cpu->X);
}

static void op_tay(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->Y = cpu->A;
    update_zero_and_negative_flags(cpu, cpu->Y);
}

static void op_txa(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->A = cpu->X;
    update_zero_and_negative_flags(cpu, cpu->A);
}

static void op_txs(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->SP = cpu->X;
}

static void op_tya(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->A = cpu->Y;
    update_zero_and_negative_flags(cpu, cpu->A);
}

// 2. Stack Operations

// Stack Helpers

static void push(CPU *cpu, Bus *bus, uint8_t value) {
    bus_write(bus, 0x0100 | cpu->SP, value);
    cpu->SP--;
}

static uint8_t pull(CPU *cpu, Bus *bus) {
    cpu->SP++;
    return bus_read(bus, 0x0100 | cpu->SP);
}

static void op_pha(CPU *cpu, Bus *bus) {
    push(cpu, bus, cpu->A);
}

static void op_php(CPU *cpu, Bus *bus) {
    uint8_t flags = cpu->P | 0x30;  // bit 4 and 5 always set when pushed
    push(cpu, bus, flags);
}

static void op_pla(CPU *cpu, Bus *bus) {
    cpu->A = pull(cpu, bus);
    update_zero_and_negative_flags(cpu, cpu->A);
}

static void op_plp(CPU *cpu, Bus *bus) {
    uint8_t value = pull(cpu, bus);
    cpu->P = (value & 0xEF) | 0x20;  // bit 5 forced to 1, bit 4 ignored
}

// 3. Increments/Decrements

static void op_dec(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    uint8_t v = bus_read(bus, addr);
    v -= 1;
    bus_write(bus, addr, v);

    update_zero_and_negative_flags(cpu, v);
}

static void op_dex(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->X -= 1;
    update_zero_and_negative_flags(cpu, cpu->X);
}

static void op_dey(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->Y -= 1;
    update_zero_and_negative_flags(cpu, cpu->Y);
}

static void op_inc(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    uint8_t v = bus_read(bus, addr);
    v += 1;
    bus_write(bus, addr, v);

    update_zero_and_negative_flags(cpu, v);
}

// Increments X by 1. Updates Zero and Negative flags.
// Affects: Z, N
static void op_inx(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->X = cpu->X + 1;
    update_zero_and_negative_flags(cpu, cpu->X);
}

static void op_iny(CPU *cpu, Bus *bus) {
    (void)bus;
    cpu->Y += 1;
    update_zero_and_negative_flags(cpu, cpu->Y);
}

// 4. Arithmetic

static void op_adc(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Page-crossing penalty for read instructions
    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X ||
         cpu->addr_mode == AM_ABSOLUTE_Y ||
         cpu->addr_mode == AM_INDIRECT_Y)) {
        cpu->cycles++;
    }

    uint8_t value = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    uint16_t sum = cpu->A + value + (cpu->P & FLAG_C ? 1 : 0);

    set_flag(cpu, FLAG_C, sum > 0xFF);
    set_flag(cpu, FLAG_V, (~(cpu->A ^ value) & (cpu->A ^ sum) & 0x80));

    cpu->A = (uint8_t)sum;
    update_zero_and_negative_flags(cpu, cpu->A);
}

static void op_sbc(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Page-crossing penalty for read instructions
    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X ||
         cpu->addr_mode == AM_ABSOLUTE_Y ||
         cpu->addr_mode == AM_INDIRECT_Y)) {
        cpu->cycles++;
    }

    uint8_t value = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    uint16_t diff = cpu->A - value - (cpu->P & FLAG_C ? 0 : 1);

    set_flag(cpu, FLAG_C, diff < 0x100);
    set_flag(cpu, FLAG_V, ((cpu->A ^ value) & (cpu->A ^ diff) & 0x80));

    cpu->A = (uint8_t)diff;
    update_zero_and_negative_flags(cpu, cpu->A);
}

// 5. Logical
static void op_and(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Page-crossing penalty for read instructions
    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X ||
         cpu->addr_mode == AM_ABSOLUTE_Y ||
         cpu->addr_mode == AM_INDIRECT_Y)) {
        cpu->cycles++;
    }

    uint8_t v = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    cpu->A &= v;
    update_zero_and_negative_flags(cpu, cpu->A);
}


static void op_ora(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X ||
         cpu->addr_mode == AM_ABSOLUTE_Y ||
         cpu->addr_mode == AM_INDIRECT_Y)) {
        cpu->cycles++;
    }

    uint8_t v = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    cpu->A |= v;
    update_zero_and_negative_flags(cpu, cpu->A);
}


static void op_eor(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X ||
         cpu->addr_mode == AM_ABSOLUTE_Y ||
         cpu->addr_mode == AM_INDIRECT_Y)) {
        cpu->cycles++;
    }

    uint8_t v = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    cpu->A ^= v;
    update_zero_and_negative_flags(cpu, cpu->A);
}


// 6. Shift & Rotate
static void op_asl(CPU *cpu, Bus *bus) {

    if (cpu->addr_mode == AM_ACCUMULATOR) {
        uint8_t v = cpu->A;
        set_flag(cpu, FLAG_C, v & 0x80);
        v <<= 1;
        cpu->A = v;
        update_zero_and_negative_flags(cpu, v);
    } else {
        uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
        uint8_t v = bus_read(bus, addr);
        set_flag(cpu, FLAG_C, v & 0x80);
        v <<= 1;
        bus_write(bus, addr, v);
        update_zero_and_negative_flags(cpu, v);
    }
}

static void op_lsr(CPU *cpu, Bus *bus) {

    if (cpu->addr_mode == AM_ACCUMULATOR) {
        uint8_t v = cpu->A;
        set_flag(cpu, FLAG_C, v & 0x01);
        v >>= 1;
        cpu->A = v;
        update_zero_and_negative_flags(cpu, v);
    } else {
        uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
        uint8_t v = bus_read(bus, addr);
        set_flag(cpu, FLAG_C, v & 0x01);
        v >>= 1;
        bus_write(bus, addr, v);
        update_zero_and_negative_flags(cpu, v);
    }
}

static void op_rol(CPU *cpu, Bus *bus) {

    uint8_t carry_in = (cpu->P & FLAG_C) ? 1 : 0;

    if (cpu->addr_mode == AM_ACCUMULATOR) {
        uint8_t v = cpu->A;
        set_flag(cpu, FLAG_C, v & 0x80);
        v = (v << 1) | carry_in;
        cpu->A = v;
        update_zero_and_negative_flags(cpu, v);
    } else {
        uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
        uint8_t v = bus_read(bus, addr);
        set_flag(cpu, FLAG_C, v & 0x80);
        v = (v << 1) | carry_in;
        bus_write(bus, addr, v);
        update_zero_and_negative_flags(cpu, v);
    }
}

static void op_ror(CPU *cpu, Bus *bus) {

    uint8_t carry_in = (cpu->P & FLAG_C) ? 0x80 : 0;

    if (cpu->addr_mode == AM_ACCUMULATOR) {
        uint8_t v = cpu->A;
        set_flag(cpu, FLAG_C, v & 0x01);
        v = (v >> 1) | carry_in;
        cpu->A = v;
        update_zero_and_negative_flags(cpu, v);
    } else {
        uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);
        uint8_t v = bus_read(bus, addr);
        set_flag(cpu, FLAG_C, v & 0x01);
        v = (v >> 1) | carry_in;
        bus_write(bus, addr, v);
        update_zero_and_negative_flags(cpu, v);
    }
}

// 7. Flag Operators
static void op_clc(CPU *cpu, Bus *bus) { (void)bus; cpu->P &= ~FLAG_C; }
static void op_sec(CPU *cpu, Bus *bus) { (void)bus; cpu->P |=  FLAG_C; }

static void op_cli(CPU *cpu, Bus *bus) { (void)bus; cpu->P &= ~FLAG_I; }
static void op_sei(CPU *cpu, Bus *bus) { (void)bus; cpu->P |=  FLAG_I; }

static void op_cld(CPU *cpu, Bus *bus) { (void)bus; cpu->P &= ~FLAG_D; }
static void op_sed(CPU *cpu, Bus *bus) { (void)bus; cpu->P |=  FLAG_D; }

static void op_clv(CPU *cpu, Bus *bus) { (void)bus; cpu->P &= ~FLAG_V; }

// 8. Comparisons

static void op_cmp(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    // Page-crossing penalty for CMP (same as LDA)
    if (cpu->page_crossed &&
        (cpu->addr_mode == AM_ABSOLUTE_X ||
         cpu->addr_mode == AM_ABSOLUTE_Y ||
         cpu->addr_mode == AM_INDIRECT_Y)) {
        cpu->cycles++;
    }

    uint8_t v = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    uint8_t r = cpu->A - v;

    set_flag(cpu, FLAG_C, cpu->A >= v);
    update_zero_and_negative_flags(cpu, r);
}

static void op_cpx(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    uint8_t v = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    uint8_t r = cpu->X - v;

    set_flag(cpu, FLAG_C, cpu->X >= v);
    update_zero_and_negative_flags(cpu, r);
}

static void op_cpy(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    uint8_t v = (cpu->addr_mode == AM_IMMEDIATE)
        ? cpu->fetched
        : bus_read(bus, addr);

    uint8_t r = cpu->Y - v;

    set_flag(cpu, FLAG_C, cpu->Y >= v);
    update_zero_and_negative_flags(cpu, r);
}

// 9. Bit Test
static void op_bit(CPU *cpu, Bus *bus) {
    uint16_t addr = resolve_address(cpu, bus, cpu->addr_mode);

    uint8_t v = bus_read(bus, addr);

    set_flag(cpu, FLAG_Z, (cpu->A & v) == 0);
    set_flag(cpu, FLAG_V, v & 0x40);
    set_flag(cpu, FLAG_N, v & 0x80);
}

// 10. Branches

// Helper
static void branch_if(CPU *cpu, bool condition) {
    Bus *bus = (Bus*)cpu->bus;

    // Read the signed offset from the instruction stream
    int8_t offset = (int8_t)bus_read(bus, cpu->PC++);

    if (condition) {
        cpu->cycles++;
        uint16_t old_pc = cpu->PC;
        cpu->PC += offset;

        // Add cycle if page boundary crossed
        if ((old_pc & 0xFF00) != (cpu->PC & 0xFF00)) {
            cpu->cycles++;
        }
    }
}

static void op_bcc(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu, !(cpu->P & FLAG_C)); }
static void op_bcs(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu,  (cpu->P & FLAG_C)); }
static void op_beq(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu,  (cpu->P & FLAG_Z)); }
static void op_bmi(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu,  (cpu->P & FLAG_N)); }
static void op_bne(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu, !(cpu->P & FLAG_Z)); }
static void op_bpl(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu, !(cpu->P & FLAG_N)); }
static void op_bvc(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu, !(cpu->P & FLAG_V)); }
static void op_bvs(CPU *cpu, Bus *bus) { (void)bus; branch_if(cpu,  (cpu->P & FLAG_V)); }

// 11. Jumps & Subroutines
static void op_jmp(CPU *cpu, Bus *bus) {
    cpu->PC = resolve_address(cpu, bus, cpu->addr_mode);
}

static void op_jsr(CPU *cpu, Bus *bus) {
    uint16_t addr = fetch_absolute_addr(cpu, bus);
    uint16_t ret = cpu->PC - 1;

    bus_write(bus, 0x0100 | cpu->SP--, (ret >> 8) & 0xFF);
    bus_write(bus, 0x0100 | cpu->SP--, ret & 0xFF);

    cpu->PC = addr;
}

static void op_rts(CPU *cpu, Bus *bus) {
    cpu->SP++;
    uint8_t lo = bus_read(bus, 0x0100 | cpu->SP);
    cpu->SP++;
    uint8_t hi = bus_read(bus, 0x0100 | cpu->SP);

    cpu->PC = ((uint16_t)hi << 8 | lo) + 1;
}

// 12. Interrupts

static void op_brk(CPU *cpu, Bus *bus) {
    cpu->PC++;  // skip padding byte (BRK is 2 bytes)

    // push PC (high then low)
    bus_write(bus, 0x0100 | cpu->SP--, (cpu->PC >> 8) & 0xFF);
    bus_write(bus, 0x0100 | cpu->SP--, cpu->PC & 0xFF);

    // push status with B and U set
    uint8_t flags = cpu->P | FLAG_B | FLAG_U;
    bus_write(bus, 0x0100 | cpu->SP--, flags);

    // set interrupt disable
    cpu->P |= FLAG_I;

    // load IRQ/BRK vector
    uint8_t lo = bus_read(bus, 0xFFFE);
    uint8_t hi = bus_read(bus, 0xFFFF);
    cpu->PC = ((uint16_t)hi << 8) | lo;
}


static void op_rti(CPU *cpu, Bus *bus) {
    cpu->SP++;
    cpu->P = (bus_read(bus, 0x0100 | cpu->SP) & 0xEF) | 0x20;

    cpu->SP++;
    uint8_t lo = bus_read(bus, 0x0100 | cpu->SP);

    cpu->SP++;
    uint8_t hi = bus_read(bus, 0x0100 | cpu->SP);

    cpu->PC = ((uint16_t)hi << 8) | lo;
}
