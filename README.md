ROM Loading

beaverNES supports NES games in the iNES format.

Current functionality:

Reads and validates the 16-byte iNES header

Extracts PRG-ROM and CHR-ROM bank counts

Determines mapper number and mirroring mode

Loads:

PRG-ROM into CPU address space ($8000–$FFFF)

CHR-ROM into PPU memory for graphics

Models cartridge data using a Cartridge structure

Initial support targets Mapper 0 (NROM), which uses fixed memory mapping and is suitable for early emulator development.

Input Handling

NES controllers use a serial shift register, meaning button states are read one bit at a time by the CPU.

Current design:

Uses SDL2 for cross-platform keyboard and controller input

Maps modern input to NES buttons:

A, B, Select, Start, Up, Down, Left, Right

Stores input state as an 8-bit value

Emulates NES controller reads via memory-mapped registers:

$4016 (Controller 1)

$4017 (Controller 2)

Implements strobe and shift behavior to match original hardware timing

Input is polled each frame and exposed to the CPU through the memory bus.

Build Requirements

C compiler (C11 or later)

SDL2

CMake


Current Status

ROM loading: In progress (Mapper 0 focus)

Input handling: Research complete, implementation in progress

Error handling and edge cases: Planned
