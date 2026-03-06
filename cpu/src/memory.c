#include "memory.h"
#include <stdint.h>
#include <stdlib.h>

Memory *memory_create(void) {
    Memory *mem = malloc(sizeof(Memory));
    if (mem) {
        for (int i = 0; i < 0x10000; i++) mem->data[i] = 0;
    }
    return mem;
}

void memory_write(Memory *mem, uint16_t addr, uint8_t val) {
    mem->data[addr] = val;
}

uint8_t memory_read(Memory *mem, uint16_t addr) {
    return mem->data[addr];
}
