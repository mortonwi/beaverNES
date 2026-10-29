cmake_minimum_required(VERSION 3.31.6)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)


if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
else()
    find_package(SDL2 REQUIRED)
endif()

# Header include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/src/apu/include
    ${CMAKE_SOURCE_DIR}/src/cpu/include
    ${CMAKE_SOURCE_DIR}/src/ppu/include
    ${CMAKE_SOURCE_DIR}/src/rom_loader/include
    ${CMAKE_SOURCE_DIR}/src/controller/include
    ${CMAKE_SOURCE_DIR}/src/external
    ${CMAKE_SOURCE_DIR}/src/config/include
    ${CMAKE_SOURCE_DIR}/src/state/include
)

# Sources
set(APU
    ${CMAKE_SOURCE_DIR}/src/apu/src/apu.c
)

set(STATE
    ${CMAKE_SOURCE_DIR}/src/state/src/state.c
)

set(CPU
    ${CMAKE_SOURCE_DIR}/src/cpu/src/bus.c
    ${CMAKE_SOURCE_DIR}/src/cpu/src/cpu.c
    ${CMAKE_SOURCE_DIR}/src/cpu/src/memory.c
    ${CMAKE_SOURCE_DIR}/src/cpu/src/opcodes.c
)

set(PPU
    ${CMAKE_SOURCE_DIR}/src/ppu/src/ppu.c
)

set(ROM_LOADER
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/cartridge.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/rom_loader.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_0.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_2.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_1.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_3.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_7.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_9.c
    ${CMAKE_SOURCE_DIR}/src/rom_loader/src/mapper_4.c
)

set(CONTROLLER
    ${CMAKE_SOURCE_DIR}/src/controller/src/controller.c
)

set(CONFIG
    ${CMAKE_SOURCE_DIR}/src/config/src/config.c
)

add_executable(beaverNES
    ${APU}
    ${CPU}
    ${PPU}
    ${ROM_LOADER}
    ${CONTROLLER}
    ${STATE}

    ${CONFIG}
    ${CMAKE_SOURCE_DIR}/external/tinyfiledialogs.c
    ${CMAKE_SOURCE_DIR}/main.c
)

# Link libraries
if(WIN32)
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2main SDL2::SDL2)
    set_target_properties(beaverNES PROPERTIES WIN32_EXECUTABLE ON)
else()
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2 m)
endif()

# Copy DLLs on Windows (vcpkg) so the exe runs from the build folder
if(WIN32)
    add_custom_command(TARGET beaverNES POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:beaverNES>
    )
endif()