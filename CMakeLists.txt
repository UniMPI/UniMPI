cmake_minimum_required(VERSION 3.10)
project(unimpi VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror=implicit-function-declaration)
endif()

# Options
option(UNIMPI_BUILD_EXAMPLES "Build example programs" ON)
option(UNIMPI_BUILD_TESTS "Build tests" ON)
option(UNIMPI_BUILD_MPI_TESTS "Register tests that require a real MPI runtime" OFF)
option(UNIMPI_ENABLE_STD_MACROS "Enable standard MPI naming macros" OFF)

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

# Create library
add_library(unimpi ${UNIMPI_SOURCES} ${UNIMPI_BACKEND_SOURCES})
add_library(unimpi::unimpi ALIAS unimpi)

target_include_directories(unimpi
    PUBLIC
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
