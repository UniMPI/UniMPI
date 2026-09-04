cmake_minimum_required(VERSION 3.10)
project(unimpi VERSION 0.1.0 LANGUAGES C)

# Pre-release tag: "alpha", "beta", "rc", or "" for a final release. Together
# with project(VERSION ...) this is the single source of truth for the library
# version. configure_file renders include/unimpi_version.h.in into
# ${CMAKE_CURRENT_BINARY_DIR}/include/unimpi_version.h.
set(UNIMPI_VERSION_PRERELEASE "alpha")
if(UNIMPI_VERSION_PRERELEASE)
    set(UNIMPI_VERSION_STRING
        "${unimpi_VERSION_MAJOR}.${unimpi_VERSION_MINOR}.${unimpi_VERSION_PATCH}-${UNIMPI_VERSION_PRERELEASE}")
else()
    set(UNIMPI_VERSION_STRING
        "${unimpi_VERSION_MAJOR}.${unimpi_VERSION_MINOR}.${unimpi_VERSION_PATCH}")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/unimpi_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/unimpi_version.h"
    @ONLY
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Strict mode: all warnings as errors
    add_compile_options(-Wall -Wextra -Werror)
    # Additional strict checks
    add_compile_options(-Wstrict-prototypes)
    add_compile_options(-Wshadow -Wcast-align)
    add_compile_options(-Wunreachable-code -Wnull-dereference)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wlogical-op -Wjump-misses-init)
    endif()
endif()

# Options
option(UNIMPI_BUILD_EXAMPLES "Build example programs" ON)
option(UNIMPI_BUILD_TESTS "Build tests" ON)
option(UNIMPI_BUILD_MPI_TESTS "Register tests that require a real MPI runtime" OFF)
option(UNIMPI_BUILD_BENCHMARKS "Build MPI benchmark programs" OFF)
option(UNIMPI_ENABLE_STD_MACROS "Enable standard MPI naming macros" OFF)
option(UNIMPI_INSTALL_DOCS "Install documentation files" ON)

# MPI facade target version (single source of truth, propagated PUBLICly so the
# library and all consumers compile with the same macros).
set(UNIMPI_MPI_TARGET_VERSION 2 CACHE STRING "Min MPI major version to build the facade for")
set(UNIMPI_MPI_TARGET_SUBVERSION 2 CACHE STRING "Min MPI minor version to build the facade for")

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
    src/predef_attr.c
    src/mt_globals.c
    src/mp_constants.c
    src/mpit.c
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
    src/backends/mpich_wrappers.c
    src/backends/intelmpi.c
    src/backends/intelmpi_wrappers.c
    src/backends/msmpi.c
    src/backends/msmpi_wrappers.c
)

# Create library
add_library(unimpi ${UNIMPI_SOURCES} ${UNIMPI_BACKEND_SOURCES})
add_library(unimpi::unimpi ALIAS unimpi)

target_include_directories(unimpi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(unimpi PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Platform libraries
if(UNIMPI_PLATFORM_POSIX)
    target_link_libraries(unimpi PUBLIC ${CMAKE_DL_LIBS})
endif()

# Definitions
if(UNIMPI_ENABLE_STD_MACROS)
    target_compile_definitions(unimpi PUBLIC UNIMPI_USE_STD_NAMES)
endif()

# MPI target-version macros: unconditional PUBLIC propagation so that the
# library and unimpi::unimpi consumers are always built with identical macros
# (the vtable layout is target-version-dependent).
target_compile_definitions(unimpi PUBLIC
    UNIMPI_MPI_TARGET_VERSION=${UNIMPI_MPI_TARGET_VERSION}
    UNIMPI_MPI_TARGET_SUBVERSION=${UNIMPI_MPI_TARGET_SUBVERSION})

# Examples
if(UNIMPI_BUILD_EXAMPLES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
        add_subdirectory(examples)
    endif()
endif()

# Benchmarks are opt-in and can be built independently of the test suite.
if(UNIMPI_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Tests
if(UNIMPI_BUILD_TESTS)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

# Installation and CMake package export
set(UNIMPI_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/unimpi"
    CACHE STRING "Directory for unimpi CMake package files"
)

install(TARGETS unimpi
    EXPORT unimpiTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT unimpiTargets
    FILE unimpiTargets.cmake
    NAMESPACE unimpi::
    DESTINATION ${UNIMPI_INSTALL_CMAKEDIR}
)

configure_package_config_file(
    cmake/unimpiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/unimpiConfig.cmake"
    INSTALL_DESTINATION ${UNIMPI_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/unimpiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/unimpiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/unimpiConfigVersion.cmake"
    DESTINATION ${UNIMPI_INSTALL_CMAKEDIR}
)

# Documentation installation
if(UNIMPI_INSTALL_DOCS)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )

    # Keep the source documentation hierarchy so relative links remain valid.
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/README.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/BUILDING.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/API.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/BACKENDS.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/TESTING.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/BENCHMARKS.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/SUPPORT_MATRIX.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/CI_CD.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/CONTRIBUTING.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/MPI_SUPPORT_ANALYSIS.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/design.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/mpi-references.md
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/mpi-api-summary.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/README.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
    )

    # Install optional Windows guide if it exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/docs/WINDOWS.md)
        install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/WINDOWS.md
            DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs
        )
    endif()
endif()

# Uninstall target
configure_file(
    cmake/cmake_uninstall.cmake.in
    cmake_uninstall.cmake
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P cmake_uninstall.cmake
    COMMENT "Removing installed files"
)

# Backend parity check: validates that all backends implement all public vtable functions
set(UNIMPI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
include(tests/cmake/check_backend_parity.cmake)
