# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.

# Online Resources/links:
https://www.copetti.org/writings/consoles/nes/
https://www.nesdev.org/NESDoc.pdf
https://www.nesdev.org/wiki/NES_reference_guide


# beaverNES – Input & ROM Loading

---

## ROM Loading

beaverNES supports NES game files using the **iNES format**.

### Current Functionality
- Reads and validates the 16-byte iNES header
- Extracts PRG-ROM and CHR-ROM bank counts
- Determines mapper number and mirroring mode

### Memory Mapping
- PRG-ROM is loaded into CPU address space (`$8000–$FFFF`)
- CHR-ROM is loaded into PPU memory for graphics
- Cartridge data is modeled using a `Cartridge` structure

### Mapper Support
- Initial support targets **Mapper 0 (NROM)**
  - Fixed memory mapping
  - Suitable for early emulator development

---

## Input Handling

NES controllers use a **serial shift register**, meaning button states are read one bit at a time by the CPU.

### Current Design
- Uses **SDL2** for cross-platform keyboard and controller input
- Maps modern input to NES buttons:
  - `A`, `B`, `Select`, `Start`
  - `Up`, `Down`, `Left`, `Right`
- Stores controller state as an 8-bit value

### NES Controller Emulation
- Emulates memory-mapped controller registers:
  - `$4016` – Controller 1
  - `$4017` – Controller 2
- Implements strobe and shift behavior to match original hardware timing
- Input is polled once per frame and exposed to the CPU via the memory bus

---

## Build Requirements
- C compiler (C11 or later)
- SDL2
- CMake

---

## Notes
This branch is under active development and focuses on correctness and clarity over performance.
