#include "bus.h"
#include <stdlib.h>

Bus *bus_create(Memory *mem) {
    Bus *bus = malloc(sizeof(Bus));
    bus->mem = mem;
    bus->cpu = NULL;
    return bus;
}

void bus_write(Bus *bus, uint16_t addr, uint8_t val) {
    memory_write(bus->mem, addr, val);
}

uint8_t bus_read(Bus *bus, uint16_t addr) {
    return memory_read(bus->mem, addr);
}
