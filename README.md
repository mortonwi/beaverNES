<h1 align="center">
  <br>
  beaverNES
  <br>
</h1>

<h4 align="center">2026 Oregon State University Capstone Project</h4>
<h4 align="center">2026 Oregon State University Capstone Project</h4>


<p align="center">
  <img src="https://github.com/mortonwi/beaverNES/blob/main/media/gif/gameplay.gif" alt="beaverNES Demo Gif" />
</p>

## Introduction

beaverNES is an NES emulator designed by the 2026 beaverNES capstone team at Oregon State University. This emulator was created in C and features fully functional simulated NES hardware such as the CPU, GPU, PPU, and APU. Currently only a handleful of mappers are supported. Please see our [Supported Mappers](#supported-mappers) section for supported mappers and titles. Additionally, the emulator features configurable controller and keyboard input.


## Getting Started

The latest installable release can be [downloaded](https://github.com/mortonwi/beaverNES/releases) for Windows 10/11 from the releases page. 

> [!NOTE]
> Mac and Linux support is deprecated after the Beta release. Support is planned to return in future releases.

Simply extract the release folder and run the `beaverNES.exe` file to launch the emulator.

To play a supported title, do the following:
1. Click `File`
2. Click `Open ROM`
3. Using the folder view, navigate to a supported `.nes` ROM file
4. Select the file and open using the folder view

> [!TIP]
> If your ROM doesn't open, it might not be supported. Please see the [Supported Mappers](#supported-mappers) section for the full compatibility list.

To play with a contoller, simply connect your device. Controller connections are handled by the emulator automatically! 

Custom controls can be set in the `Controls` tab of the emulator.


## Supported Mappers

An NES mapper refers to the memory mapping of circuits and hardware on original game cartridges. Cartridges were often designed to extend the capabilities of the NES with features like more RAM or audio channels, which require different memory mappings to work. In the emulation setting, software mappers mimic these original cartridge configurations to support classic titles.

The following mappers are supported:
- Mapper 0
- Mapper 1
- Mapper 2
- Mapper 3
- Mapper 4
- Mapper 7
- Mapper 9

> [!NOTE]
> A common mapper list can be viewed [here](https://www.nesdev.org/wiki/List_of_mappers). Increased mapper support is planned for future releases.

> [!NOTE]
> A single mapper can support hundreds of games. The full list of currently supported games is too large to be displayed here. To verify ROM support, confirm that your ROM's mapper is contained in the list above.


## Credits

A list of open source projects and documents used or referenced for this project.

- [Copetti](https://www.copetti.org/writings/consoles/nes/)
- [NESDev](https://www.nesdev.org/NESDoc.pdf)
- [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/)


## Bug Reporting

This project is far from perfect. Please report any bug by creating an [issue](https://github.com/mortonwi/beaverNES/issues). Your help on this little hobby project is appreciated.

