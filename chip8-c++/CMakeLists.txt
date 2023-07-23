# CMake necessitates setting the minimum required version
cmake_minimum_required(VERSION 3.7)

# the name of the CMake project, is required but not relevant to this project
project(chip8-c++)

# required by CMAKE to use c++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# assert requires packages exist (on Linux)
find_package(SDL2 REQUIRED)

add_library(chip8-c++
    core/core.cpp
)

# defines the executable of the project, this will be the finalized emulator program
add_executable(chip8-c++-sdl
    frontend_sdl/main.cpp
)

# tells CMake where to find the project's headers, in particular the "core.hpp" header
target_include_directories(chip8-c++        PUBLIC core)
target_include_directories(chip8-c++-sdl    PUBLIC core ${SDL2_INCLUDE_DIRS})

# tells CMake to pass some configurations to the executable, incouding warnings and optimizations.
# this project uses 03 optimization, even if it might be discouraged for a lot of projects, since there will 
# be no UB in the project or floating point operations, it can be done with concession of stability
# to gain speed. In your own project, understand a CHIP-8 implementation does not require speed 
# as it's a very computationally light spec to implement for modern computers, so this is not necessary, the -O3 can be omitted.
set(COMPILE_OPTIONS -Wall -pedantic -Wstrict-aliasing=1 -O3)
target_compile_options(chip8-c++        PUBLIC ${COMPILE_OPTIONS})
target_compile_options(chip8-c++-sdl    PUBLIC ${COMPILE_OPTIONS})

# links our frontend to the core implementation and frontend specific libraries.
target_link_libraries(chip8-c++-sdl chip8-c++ ${SDL2_LIBRARIES})