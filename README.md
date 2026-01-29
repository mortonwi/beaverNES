# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.


## Online Resources/links:
- https://www.copetti.org/writings/consoles/nes/
- https://www.nesdev.org/NESDoc.pdf
- https://www.nesdev.org/wiki/NES_reference_guide


## APU Component 

### Description

The Audio Processing Unit (APU) is a core hardware component of the Nintendo Entertainment System (NES) responsible for generating all sound output. It operates by reading values written to specific memory-mapped registers by the CPU and uses those values to control how audio is produced.    

### Audio Channels

The APU produces sound through five audio channels, each designed for a specific type of sound. By mixing these channels, the NES can create music, sound effects, and other audio cues used by ROMS.

**Channels:**
- Two Pulse Wave Channels: Both channels are used together to create melodies and harmonies. 
- Triangle Wave Channel: Used for bass lines. 
- Noise Channel: Used for percussion and sound effects. 
- Delta Modulation Channel: Plays sample-based audio for voice and complex sounds like recordings. 

### Registers

**Register Table:**

| Address      | Register Name       | 7654 321  | Function                                       |
|--------------|---------------------|-----------|------------------------------------------------|
| 0x4000       | Pulse 1 Control     | DDLC NNNN | Duty cycle, envelope, and volume control       |
| 0x4001       | Pulse 1 Sweep       | EPPP NSSS | Sweep unit configuration                       |
| 0x4002       | Pulse 1 Timer Low   | LLLL LLLL | Lower 8 bits of timer period                   |
| 0x4003       | Pulse 1 Timer High  | LLLL LHHH | Upper 3 bits of timer, length counter load     |
| 0x4004       | Pulse 2 Control     | DDLC NNNN | Duty cycle, envelope, and volume control       |
| 0x4005       | Pulse 2 Sweep       | EPPP NSSS | Sweep unit configuration                       |
| 0x4006       | Pulse 2 Timer Low   | LLLL LLLL | Lower 8 bits of timer period                   |
| 0x4007       | Pulse 2 Timer High  | LLLL LHHH | Upper 3 bits of timer, length counter load     |
| 0x4008       | Triangle Control    | CRRR RRRR | Linear counter control                         |
| 0x4009       | Unused              | ---- ---- |                                                |
| 0x400A       | Triangle Timer Low  | LLLL LLLL | Lower 8 bits of timer period                   |
| 0x400B       | Triangle Timer High | LLLL LHHH | Upper 3 bits of timer, length counter load     |
| 0x400C       | Noise Control       | --LC NNNN | Envelope and volume control                    |
| 0x400D       | Unused              | ---- ---- |                                                |
| 0x400E       | Noise Period        | L--- PPPP | Noise frequency and mode                       |
| 0x400F       | Noise Length        | LLLL L--- | Length counter load                            |
| 0x4010       | DMC Control         | IL-- FFFF | IRQ enable, loop flag, rate                    |
| 0x4011       | DMC Direct Load     | -DDD DDDD | Output level                                   |
| 0x4012       | DMC Sample Address  | AAAA AAAA | Sample start address                           |
| 0x4013       | DMC Sample Length   | LLLL LLLL | Sample length                                  |
| 0x4015 READ  | APU Status          | ---D NT21 | Control: DMC enable, length counter enables    |
| 0x4015 WRITE | APU Status          | IF-D NT21 | Status: DMC interrupt, length counter status   |
| 0x4017       | Frame Counter       | SD-- ---- | 5-frame write sequence, disable fram interrupt |  

