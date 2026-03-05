cmake_minimum_required(VERSION 3.31.6)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# SDL2_TTF
if(WIN32)
    set(SDL2_TTF_INCLUDE_DIR $ENV{SDL2_TTF}/include/SDL2)
    set(SDL2_TTF_LIBRARY $ENV{SDL2_TTF}/lib/libSDL2_ttf.dll.a)
    include_directories(${SDL2_TTF_INCLUDE_DIR})
else()
    find_package(SDL2_ttf REQUIRED)
    include_directories(${SDL2_TTF_INCLUDE_DIRS})
endif()

# Header files
include_directories(
    ${CMAKE_SOURCE_DIR}/apu/include/
    ${CMAKE_SOURCE_DIR}/cpu/include/
    ${CMAKE_SOURCE_DIR}/ppu/include/
    ${CMAKE_SOURCE_DIR}/Rom_loader/include/
)

# Sources
set(APU
    apu/src/apu.c
)

set(CPU
    cpu/src/bus.c
    cpu/src/cpu.c
    cpu/src/memory.c
    cpu/src/opcodes.c
)

set(PPU
    ppu/src/ppu.c
)

set(ROM_LOADER
    Rom_loader/src/cartridge.c
    Rom_loader/src/mapper.c
    Rom_loader/src/rom_loader.c
    Rom_loader/src/mapper_0.c
    Rom_loader/src/mapper_2.c
)

add_executable(beaverNES 
    ${APU}
    ${CPU}
    ${PPU}
    ${ROM_LOADER}
    main.c
)

# Different links for Windows (WIN32) vs Linux
if(WIN32)
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2main SDL2::SDL2 ${SDL2_TTF_LIBRARY})
    set_target_properties(beaverNES PROPERTIES WIN32_EXECUTABLE ON)

    # Copy DLLs to output directory
    add_custom_command(TARGET beaverNES POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $ENV{SDL2}/lib/x64/SDL2.dll
            $<TARGET_FILE_DIR:beaverNES>
    )
    add_custom_command(TARGET beaverNES POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $ENV{SDL2_TTF}/bin/SDL2_ttf.dll
            $<TARGET_FILE_DIR:beaverNES>
    )
else()
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)
endif()