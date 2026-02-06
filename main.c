#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "src/apu.h"

#define CPU_FREQ    1789773  // frequency of a NTCS CPU

#define REGION      0        // 0:NTSC 1:PAL

int main() {
    APU apu;
    
    // init apu with NTSC
    init_apu(&apu, REGION);
    
    // test pulse wave
    apu_write(&apu, 0x4015, 0x03);  // enable both pulse waves
    
    // pulse1
    apu_write(&apu, 0x4000, 0x3F);  // duty=0, constant volume=1, volume=0
    apu_write(&apu, 0x4002, 0x0F);  // low
    apu_write(&apu, 0x4003, 0x01);  // high

    // pulse2
    apu_write(&apu, 0x4004, 0x3F);  // duty=0, constant volume=1, volume=0
    apu_write(&apu, 0x4006, 0x0F);  // low
    apu_write(&apu, 0x4007, 0x01);  // high

    // get buffer from audio output
    float sample = apu_tick(&apu, REGION);
    if (sample > 0.0f) {
        printf("Sample generated : %f\n", sample);
    } else {
        printf("Unable to generate non-zero audio sample\n");
    }

    // handle and send to SDL2

    // TODOOOOOOOOOOOOO

    return 0;
}