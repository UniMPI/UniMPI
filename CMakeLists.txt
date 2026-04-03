cmake_minimum_required(VERSION 3.10)
project(TFTK_MPI VERSION 0.1.0 LANGUAGES C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(TFTK_MPI_BUILD_EXAMPLES "Build example programs" ON)
option(TFTK_MPI_BUILD_TESTS "Build tests" ON)
option(TFTK_MPI_ENABLE_STD_MACROS "Enable standard MPI naming macros" OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Platform detection
if(WIN32)
    set(TFTK_MPI_PLATFORM_WINDOWS TRUE)
else()
    set(TFTK_MPI_PLATFORM_POSIX TRUE)
endif()

# Source files
set(TFTK_MPI_SOURCES
    src/core.c
    src/loader.c
    src/vtable.c
    src/error.c
)

if(TFTK_MPI_PLATFORM_POSIX)
    list(APPEND TFTK_MPI_SOURCES src/platform_posix.c)
elseif(TFTK_MPI_PLATFORM_WINDOWS)
    list(APPEND TFTK_MPI_SOURCES src/platform_windows.c)
endif()

# Backend sources
set(TFTK_MPI_BACKEND_SOURCES
    src/backends/openmpi.c
    src/backends/mpich.c
    src/backends/intelmpi.c
    src/backends/msmpi.c
)

# Create static library
add_library(tftk_mpi STATIC ${TFTK_MPI_SOURCES} ${TFTK_MPI_BACKEND_SOURCES})

# Platform libraries
if(TFTK_MPI_PLATFORM_POSIX)
    target_link_libraries(tftk_mpi dl)
endif()

# Definitions
if(TFTK_MPI_ENABLE_STD_MACROS)
    target_compile_definitions(tftk_mpi PUBLIC TFTK_MPI_USE_STD_NAMES)
endif()

# Examples
if(TFTK_MPI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(TFTK_MPI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
