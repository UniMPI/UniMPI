cmake_minimum_required(VERSION 3.10)
project(unimpi VERSION 0.1.0 LANGUAGES C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror=implicit-function-declaration)
endif()

# Options
option(UNIMPI_BUILD_EXAMPLES "Build example programs" ON)
option(UNIMPI_BUILD_TESTS "Build tests" ON)
option(UNIMPI_ENABLE_STD_MACROS "Enable standard MPI naming macros" OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Platform detection
if(WIN32)
    set(UNIMPI_PLATFORM_WINDOWS TRUE)
else()
    set(UNIMPI_PLATFORM_POSIX TRUE)
endif()

# Source files
set(UNIMPI_SOURCES
    src/core.c
    src/loader.c
    src/vtable.c
    src/error.c
)

if(UNIMPI_PLATFORM_POSIX)
    list(APPEND UNIMPI_SOURCES src/platform_posix.c)
elseif(UNIMPI_PLATFORM_WINDOWS)
    list(APPEND UNIMPI_SOURCES src/platform_windows.c)
endif()

# Backend sources
set(UNIMPI_BACKEND_SOURCES
    src/backends/openmpi.c
    src/backends/mpich.c
    src/backends/intelmpi.c
    src/backends/msmpi.c
)

# Create static library
add_library(unimpi STATIC ${UNIMPI_SOURCES} ${UNIMPI_BACKEND_SOURCES})

# Platform libraries
if(UNIMPI_PLATFORM_POSIX)
    target_link_libraries(unimpi dl)
endif()

# Definitions
if(UNIMPI_ENABLE_STD_MACROS)
    target_compile_definitions(unimpi PUBLIC UNIMPI_USE_STD_NAMES)
endif()

# Examples
if(UNIMPI_BUILD_EXAMPLES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
        add_subdirectory(examples)
    endif()
endif()

# Tests
if(UNIMPI_BUILD_TESTS)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()
