# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.


# Online Resources/links:
https://www.copetti.org/writings/consoles/nes/
https://www.nesdev.org/NESDoc.pdf
https://www.nesdev.org/wiki/NES_reference_guide

compile with this:
gcc -g -O0 -Wall -Wextra -std=c11 cpu/src/*.c apu/src/*.c Rom_loader/src/*.c ppu/src/*.c main.c -Icpu/include -Iapu/include -IRom_loader/include -Ippu/include $(sdl2-config --cflags --libs) -o beaverNES