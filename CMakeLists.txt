cmake_minimum_required(VERSION 3.31.6)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(beaverNES main.c)

# Different links for Windows (WIN32) vs Linux
if(WIN32)
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2main SDL2::SDL2)
    set_target_properties(beaverNES PROPERTIES WIN32_EXECUTABLE ON)
else()
    target_link_libraries(beaverNES PRIVATE SDL2::SDL2)
endif()
