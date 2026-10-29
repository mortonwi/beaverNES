#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef struct Memory {
    uint8_t data[0x10000]; // 64 KB of memory - just like the 6502 CPU in the NES 

    // Should be a range of memory specific to each part of NES
    // For now, one large array

} Memory;

Memory *memory_create(void);
void memory_write(Memory *mem, uint16_t addr, uint8_t val); // writes a single byte to a specific memory address
uint8_t memory_read(Memory *mem, uint16_t addr); // reads a byte from a specific memory address

#endif // MEMORY_H
