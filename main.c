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
    
    // test pulse1 (made with AI)
    apu_write(&apu, 0x4015, 0x01);  // enable pulse1
    apu_write(&apu, 0x4000, 0x30);  // duty=0, constant volume=1, volume=0
    apu_write(&apu, 0x4002, 0x0F);  // low
    apu_write(&apu, 0x4003, 0x01);  // high
    apu_write(&apu, 0x4000, 0x3F);  // constant volume = 15
    
    // get buffer from audio output
    bool success = false;
    
    for (int i = 0; i < 2048; i++) {
        float sample = apu_tick(&apu, REGION);
        if (sample > 0.0f) {
            printf("Sample generated after %d ticks: %f\n", i, sample);
            success = true;
            break;
        }
    }

    if (!success) {
        printf("Unable to generate non-zero audio sample\n");
    }

    // handle and send to SDL2

    // TODOOOOOOOOOOOOO

    return 0;
}