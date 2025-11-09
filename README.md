# beaverNES
Oregon State Capstone (CS 46X) project revolving around NES Emulation.

## Helpful Links


## Dev Installation

### Windows VS Code
- Download and extract SDL2 from this link: [SDL2](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10).
- Add the SDL2-2.32.10/cmake folder as an environment or system variable called SDL2_DIR.
- Download and extract the latest MinGW-64 from this link: [MinGW-64](https://winlibs.com/).
- Add the mingw64/bin folder to system path.
- In visual studio code, install the CMake and CMake Tools extensions. 
- Use Ctrl + Shift + P and select CMake: Select a Kit.
- Select the MinGW-64 GCC compiler from the list.
- Using the same menu, run CMake: Build.
- Executable can be found in the generated 'build' folder.