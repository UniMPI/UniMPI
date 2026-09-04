# Negative-compile gate check for the MPI-3.0 vtable layout.
#
# Proves that the MPI-3.0 vtable fields are PHYSICALLY removed when building
# against a 2.2 target. It compiles tests/internal/test_vtable_strict.c with
# -DUNIMPI_MPI_TARGET_VERSION=2 -DUNIMPI_MPI_TARGET_SUBVERSION=2; that TU
# references gated fields (unimpi.ibcast etc.) which must NOT compile. A clean
# compile here means the guard is broken; a compile error is the expected pass.
#
# Usage:
#   cmake -DUNIMPI_SOURCE_DIR=/path/to/source \
#         -DUNIMPI_C_COMPILER=/path/to/cc \
#         -P check_vtable_strict.cmake

cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED UNIMPI_SOURCE_DIR)
    message(FATAL_ERROR "UNIMPI_SOURCE_DIR is required")
endif()
if(NOT DEFINED UNIMPI_C_COMPILER)
    message(FATAL_ERROR "UNIMPI_C_COMPILER is required")
endif()

set(SRC "${UNIMPI_SOURCE_DIR}/tests/internal/test_vtable_strict.c")
set(OUT "${CMAKE_CURRENT_BINARY_DIR}/test_vtable_strict_probe.o")

execute_process(
    COMMAND "${UNIMPI_C_COMPILER}"
            "-DUNIMPI_USE_STD_NAMES"
            "-DUNIMPI_MPI_TARGET_VERSION=2"
            "-DUNIMPI_MPI_TARGET_SUBVERSION=2"
            "-I${UNIMPI_SOURCE_DIR}/include"
            -c "${SRC}" -o "${OUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)

if(result EQUAL 0)
    message(FATAL_ERROR
        "test_vtable_strict: MPI-3.0 vtable fields COMPILED at target 2.2 "
        "(e.g. unimpi.ibcast still present) -> version guards are BROKEN.\n"
        "Compiler output:\n${out}${err}")
endif()

message(STATUS
    "test_vtable_strict: compile correctly FAILED at target 2.2 "
    "(MPI-3.0 fields absent -> gates active)")
