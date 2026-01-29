cmake_minimum_required(VERSION 3.31.6)
project(beaverNES VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Sources
set(SRC
    main.c
    src/apu.c
)

add_executable(beaverNES 
    ${SRC}
)
