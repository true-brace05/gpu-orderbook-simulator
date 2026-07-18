cmake_minimum_required(VERSION 3.20)

project(
    LOBSimulator
    VERSION 1.0.0
    DESCRIPTION "High Performance Limit Order Book Simulator"
    LANGUAGES CXX
)

# ---------------------------------------
# C++ Standard
# ---------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------
# Build Type
# ---------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------
# Compiler Warnings
# ---------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# ---------------------------------------
# Enable Testing
# ---------------------------------------
enable_testing()

# ---------------------------------------
# Project Modules
# ---------------------------------------
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmark)