cmake_minimum_required(VERSION 4.2.0)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(
    beaverNES
    main.c
)

target_link_libraries(beaverNES PRIVATE SDL2::SDL2)