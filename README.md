# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.

# Online Resources/links:
https://www.copetti.org/writings/consoles/nes/
https://www.nesdev.org/NESDoc.pdf
https://www.nesdev.org/wiki/NES_reference_guide


# beaverNES – ROM Loading & Input Foundations

This branch focuses on building and validating the **ROM loading pipeline** for beaverNES, along with early groundwork for controller input. The goal is to ensure NES game cartridges are loaded, parsed, and mapped correctly before deeper CPU/PPU integration.

---

## ROM Loading

beaverNES currently supports NES game files using the **iNES format**, which is the standard ROM layout used by most NES cartridges.

### Implemented Functionality
- Reads and validates the 16-byte iNES header  
- Verifies correct ROM magic (`"NES" 0x1A`)  
- Extracts:
  - PRG-ROM bank count  
  - CHR-ROM bank count  
  - Mapper number  
  - Mirroring mode  
  - Trainer flag  
- Loads PRG-ROM and CHR-ROM data into memory buffers  
- Graceful error handling for invalid or unsupported ROMs  

### Cartridge Model
ROM data is represented using a `Cartridge` structure, which stores:
- Parsed iNES header data  
- PRG-ROM and CHR-ROM buffers  
- Mapper number and mirroring configuration  

This structure acts as the software representation of a physical NES cartridge.

---

## Memory Mapping (Mapper 0 – NROM)

Initial mapper support targets **Mapper 0 (NROM)**, which requires no bank switching and is ideal for early emulator development.

### Current Mapper 0 Support
- PRG-ROM mapped into CPU address space (`$8000–$FFFF`)  
  - 16 KiB PRG-ROM is mirrored into the upper bank  
  - 32 KiB PRG-ROM is mapped directly  
- CHR-ROM mapped into PPU address space (`$0000–$1FFF`)  
- CPU and PPU read helper functions validate address ranges and mirror behavior  

ROM loading and mapping have been tested using real NES ROMs (e.g., *Super Mario Bros.*), confirming correct mapper detection and memory behavior.

---

## Input Handling (Planned / In Progress)

NES controllers use a **serial shift register**, where button states are read one bit at a time through memory-mapped registers.

### Current Status
- Input handling design is defined but not fully implemented yet  
- SDL2 will be used for cross-platform keyboard and controller input  
- Planned button mapping:
  - `A`, `B`, `Select`, `Start`  
  - `Up`, `Down`, `Left`, `Right`  

### Planned NES Controller Emulation
- Memory-mapped registers:
  - `$4016` – Controller 1  
  - `$4017` – Controller 2  
- Emulation of strobe and serial shift behavior to match original hardware  
- Controller state stored as an 8-bit value and read by the CPU  

---

## Building and Testing

### Build Requirements
- C compiler (C11 or later)  
- `make`  
- SDL2 (for upcoming input implementation)  

### Example Test Run
```bash
make
./beavernes path/to/game.nes

