cmake_minimum_required(VERSION 3.31.6)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# SDL2_TTF
set(SDL2_TTF_INCLUDE_DIR $ENV{SDL2_TTF}/include/SDL2)
set(SDL2_TTF_LIBRARY $ENV{SDL2_TTF}/lib/libSDL2_ttf.dll.a)
include_directories(${SDL2_TTF_INCLUDE_DIR})

add_executable(beaverNES main.c)

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
    # Fix linux toolchain
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2 ${SDL2_TTF_LIBRARY})
endif()
