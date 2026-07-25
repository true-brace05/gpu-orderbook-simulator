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
# Optional CUDA Support
# ---------------------------------------
option(ENABLE_CUDA "Enable CUDA acceleration" ON)

if(ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)

    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)

        # Set CUDA standard
        set(CMAKE_CUDA_STANDARD 17)
        set(CMAKE_CUDA_STANDARD_REQUIRED ON)

        # Set explicit CUDA target architectures (Turing, Ampere, Ada Lovelace, Hopper)
        if(NOT CMAKE_CUDA_ARCHITECTURES)
            set(CMAKE_CUDA_ARCHITECTURES 75 80 86 89 90)
        endif()

        message(STATUS "CUDA support enabled. Compiler: ${CMAKE_CUDA_COMPILER}")
        message(STATUS "CUDA Target Architectures: ${CMAKE_CUDA_ARCHITECTURES}")
    else()
        message(STATUS "CUDA compiler not found. Building CPU-only mode.")
        set(ENABLE_CUDA OFF CACHE BOOL "Enable CUDA acceleration" FORCE)
    endif()
else()
    message(STATUS "CUDA support disabled by configuration.")
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