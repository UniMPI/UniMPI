# Building unimpi

This guide covers building `unimpi` from source on various platforms.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Build](#quick-build)
- [Linux / macOS](#linux--macos)
- [Windows](#windows)
- [CMake Options](#cmake-options)
- [Installation](#installation)
- [Consuming with CMake](#consuming-with-cmake)
- [Real MPI Tests](#real-mpi-tests)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required

- CMake >= 3.10
- C99 compatible compiler (gcc, clang, MSVC)

> **Note:** Building `unimpi` itself **does NOT require MPI**. It is a runtime-loading wrapper that works with any MPI implementation.
> MPI is only needed when:
> - Running real MPI tests (`-DUNIMPI_BUILD_MPI_TESTS=ON`)
> - Running applications that call MPI functions

### Optional (for testing)

- OpenMPI, MPICH, or Intel-MPI (Linux/macOS)
- MS-MPI (Windows)

### Supported Compilers

| Compiler | Version | Platform |
|----------|---------|----------|
| GCC | >= 7.0 | Linux, macOS |
| Clang | >= 6.0 | Linux, macOS |
| MSVC | >= 2017 | Windows |
| MinGW-w64 | >= 8.0 | Windows |

---

## Quick Build

```bash
# Clone repository
git clone https://github.com/yourusername/unimpi.git
cd unimpi

# Configure and build
cmake -B build .
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

On Windows with Visual Studio:
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest -C Release --output-on-failure
```

---

## Linux / macOS

### Standard Build

```bash
cmake -B build .
cmake --build build
```

### With Specific MPI

```bash
# With OpenMPI
cmake -B build . -DCMAKE_C_COMPILER=mpicc

# With Intel MPI (after sourcing setvars.sh)
source /opt/intel/oneapi/setvars.sh
cmake -B build . -DCMAKE_C_COMPILER=mpiicc

# With MPICH
cmake -B build . \
    -DCMAKE_C_COMPILER=/usr/local/mpich/bin/mpicc
```

### Static vs Shared Library

```bash
# Static library (default)
cmake -B build . -DBUILD_SHARED_LIBS=OFF

# Shared library
cmake -B build . -DBUILD_SHARED_LIBS=ON
```

---

## Windows

### Using Visual Studio

1. Install MS-MPI SDK from https://www.microsoft.com/download/details.aspx?id=57467
2. Build:

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Using MinGW-w64

```bash
# Ensure MinGW is in PATH
cmake -B build -G "MinGW Makefiles" \
    -DCMAKE_C_COMPILER=gcc
cmake --build build
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `UNIMPI_BUILD_EXAMPLES` | ON | Build example programs |
| `UNIMPI_BUILD_TESTS` | ON | Build test suite |
| `UNIMPI_BUILD_MPI_TESTS` | OFF | Register tests requiring MPI runtime |
| `UNIMPI_ENABLE_STD_MACROS` | OFF | Enable standard MPI naming macros |
| `BUILD_SHARED_LIBS` | OFF | Build shared instead of static library |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation prefix |

### Examples

```bash
# Minimal build (library only)
cmake -B build . \
    -DUNIMPI_BUILD_EXAMPLES=OFF \
    -DUNIMPI_BUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release

# Debug build with all tests
cmake -B build . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUNIMPI_BUILD_TESTS=ON \
    -DUNIMPI_BUILD_MPI_TESTS=ON
```

---

## Installation

### System-wide Installation

```bash
cmake --build build
sudo cmake --install build
```

### Local Installation

```bash
cmake -B build . -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
cmake --install build

# Update PATH
export PATH=$HOME/.local/bin:$PATH
export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
```

### Uninstall

```bash
sudo cmake --build build --target uninstall
```

---

## Consuming with CMake

After installation, downstream projects can use:

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

find_package(unimpi CONFIG REQUIRED)
add_executable(myapp myapp.c)
target_link_libraries(myapp PRIVATE unimpi::unimpi)
```

Configure with the install prefix:

```bash
cmake -B build . -DCMAKE_PREFIX_PATH=$HOME/.local
cmake --build build
```

---

## Real MPI Tests

By default, only fixture-based unit tests run. To enable real MPI tests:

```bash
cmake -B build . \
    -DUNIMPI_BUILD_MPI_TESTS=ON \
    -DMPIEXEC_EXECUTABLE=/usr/bin/mpirun

cmake --build build
ctest --output-on-failure
```

For specific backends:

```bash
# OpenMPI with oversubscribe
cmake -B build . \
    -DUNIMPI_BUILD_MPI_TESTS=ON \
    -DMPIEXEC_EXECUTABLE=/usr/bin/mpirun \
    -DMPIEXEC_PREFLAGS=--oversubscribe

# Intel MPI
cmake -B build . \
    -DUNIMPI_BUILD_MPI_TESTS=ON \
    -DMPIEXEC_EXECUTABLE=/opt/intel/oneapi/mpi/latest/bin/mpiexec

# Windows MS-MPI
cmake -B build . \
    -DUNIMPI_BUILD_MPI_TESTS=ON \
    -DMPIEXEC_EXECUTABLE="C:/Program Files/Microsoft MPI/Bin/mpiexec.exe"
```

---

## Troubleshooting

### CMake cannot find MPI

```bash
# Specify MPI compiler explicitly
cmake -B build . -DCMAKE_C_COMPILER=/usr/bin/mpicc

# Or specify paths
cmake -B build . \
    -DMPI_C_INCLUDE_DIRS=/usr/include/openmpi \
    -DMPI_C_LIBRARIES=/usr/lib/x86_64-linux-gnu/libmpi.so
```

### Linker errors on Linux (dlopen)

Should be automatic, but if needed:
```bash
cmake -B build . -DCMAKE_C_FLAGS="-ldl"
```

### Windows: Cannot find msmpi.dll

Download and install MS-MPI from:
https://www.microsoft.com/download/details.aspx?id=57467

### Tests fail to find MPI library

```bash
# Set library path for tests
export LD_LIBRARY_PATH=/path/to/mpi/lib:$LD_LIBRARY_PATH
cd build && ctest
```

### Windows: "The system cannot find the file specified"

Ensure MS-MPI Bin directory is in PATH:
```cmd
set PATH=C:\Program Files\Microsoft MPI\Bin;%PATH%
```

---

## Platform-Specific Notes

### Ubuntu/Debian

```bash
sudo apt-get install cmake build-essential
sudo apt-get install libopenmpi-dev    # or libmpich-dev
```

### CentOS/RHEL/Fedora

```bash
sudo yum install cmake gcc
sudo yum install openmpi-devel         # or mpich-devel
```

### macOS (Homebrew)

```bash
brew install cmake
brew install openmpi                   # or mpich
```

---

## Cross-Compilation

### Building for Windows from Linux (MinGW)

```bash
# Install cross-compiler
sudo apt-get install mingw-w64

# Create toolchain file
cat > mingw-w64.cmake << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
EOF

# Configure and build
cmake -B build . -DCMAKE_TOOLCHAIN_FILE=mingw-w64.cmake
```

See [WINDOWS.md](WINDOWS.md) for more Windows-specific build instructions.
