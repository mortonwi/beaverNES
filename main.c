#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "src/apu.h"

#define CPU_FREQ    1789773  // frequency of a NTCS CPU

int main() {
    APU apu;

    // init apu with NTSC
    init_apu(&apu, 0);
    printf("Initialized APU with Pulse Wave Channels\n");

    return 0;
}