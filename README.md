# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.

# Online Resources/links:
https://www.copetti.org/writings/consoles/nes/
https://www.nesdev.org/NESDoc.pdf
https://www.nesdev.org/wiki/NES_reference_guide


# beaverNES – ROM Loading & Input Foundations


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


## Memory Mapping (Mapper 2 - UxROM)

Mapper 2 (UxROM) supports PRG-ROM bank switching, allowing larger games to run on the NES.

### Current Mapper 2 Support

- 16 KiB switchable PRG-ROM bank mapped to `$8000-$BFFF`
- Fixed 16 KiB PRG-ROM bank mapped to `$C000-$FFFF`
- Bank switching controlled via CPU writes to `$8000-$FFFF`
- CHR-RAM (8 KiB) mapped to PPU address space `$0000-$1FFF`
- Proper header-based mapper detection

### Behavior Details

- Writes to the mapper select the lower PRG bank
- The upper PRG bank remains fixed to the last bank
- No CHR-ROM bank switching (CHR-RAM only)

ROM loading and mapping have been tested using real UxROM titles to validate bank switching behavior.

---

## Input Handling

NES controllers use a serial shift register, where button states are read one bit at a time through memory-mapped registers.

### Current Implementation

- SDL2-based cross-platform keyboard input
- Accurate NES controller shift register behavior
- Proper strobe ($4016 write) handling
- Serial bit-by-bit reads matching original hardware
- Controller state stored as 8-bit value

### Keyboard Mapping (Controller 1)

| NES Button | Keyboard Key |
|------------|--------------|
| A          | X |
| B          | Z |
| Select     | Right Shift |
| Start      | Enter |
| Up         | Arrow Up |
| Down       | Arrow Down |
| Left       | Arrow Left |
| Right      | Arrow Right |
| Quit Test  | Escape |

### Memory-Mapped Registers

- `$4016` — Controller 1

### Behavior Details

- Writing `1` then `0` to `$4016` latches button states
- CPU reads shift out bits in this order:

  `A, B, Select, Start, Up, Down, Left, Right`

- Matches original NES hardware behavior

### Build Requirements

- C compiler (C11 or later)
- `make`
- SDL2 (`libsdl2-dev` on Linux)  

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

