cmake_minimum_required(VERSION 3.31.6)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- SDL2 / SDL2_ttf ---
# Use CONFIG packages so this works cleanly with vcpkg on Windows (MSVC),
# and also works on Linux when the -dev packages are installed.
if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)
else()
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
endif()

# Header include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/apu/include
    ${CMAKE_SOURCE_DIR}/cpu/include
    ${CMAKE_SOURCE_DIR}/ppu/include
    ${CMAKE_SOURCE_DIR}/Rom_loader/include
    ${CMAKE_SOURCE_DIR}/controller/include
)

# Sources
set(APU
    ${CMAKE_SOURCE_DIR}/apu/src/apu.c
)

set(CPU
    ${CMAKE_SOURCE_DIR}/cpu/src/bus.c
    ${CMAKE_SOURCE_DIR}/cpu/src/cpu.c
    ${CMAKE_SOURCE_DIR}/cpu/src/memory.c
    ${CMAKE_SOURCE_DIR}/cpu/src/opcodes.c
)

set(PPU
    ${CMAKE_SOURCE_DIR}/ppu/src/ppu.c
)

set(ROM_LOADER
    ${CMAKE_SOURCE_DIR}/Rom_loader/src/cartridge.c
    ${CMAKE_SOURCE_DIR}/Rom_loader/src/mapper.c
    ${CMAKE_SOURCE_DIR}/Rom_loader/src/rom_loader.c
    ${CMAKE_SOURCE_DIR}/Rom_loader/src/mapper_0.c
    ${CMAKE_SOURCE_DIR}/Rom_loader/src/mapper_2.c
)

set(CONTROLLER
    ${CMAKE_SOURCE_DIR}/controller/src/controller.c
)

add_executable(beaverNES
    ${APU}
    ${CPU}
    ${PPU}
    ${ROM_LOADER}
    ${CONTROLLER}
    ${CMAKE_SOURCE_DIR}/main.c
)

# Link libraries
if(WIN32)
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2main SDL2::SDL2 SDL2_ttf::SDL2_ttf)
    set_target_properties(beaverNES PROPERTIES WIN32_EXECUTABLE ON)
else()
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)
endif()

# Copy DLLs on Windows (vcpkg) so the exe runs from the build folder
if(WIN32)
    add_custom_command(TARGET beaverNES POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:beaverNES>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2_ttf::SDL2_ttf>
            $<TARGET_FILE_DIR:beaverNES>
    )
endif()