# Building unimpi

This guide covers building `unimpi` from source on various platforms.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Linux / macOS](#linux--macos)
- [Windows](#windows)
- [CMake Options](#cmake-options)
- [Installation](#installation)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required

- CMake >= 3.10
- C99 compatible compiler (gcc, clang, MSVC)
- At least one MPI implementation

### Supported Compilers

| Compiler | Version | Platform |
|----------|---------|----------|
| GCC | >= 7.0 | Linux, macOS |
| Clang | >= 6.0 | Linux, macOS |
| MSVC | >= 2017 | Windows |
| MinGW-w64 | >= 8.0 | Windows |

---

## Linux / macOS

### Quick Build

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

### With Specific MPI

```bash
# With OpenMPI
cmake -B build . \
    -DCMAKE_C_COMPILER=mpicc

# With Intel MPI (after sourcing setvars.sh)
source /opt/intel/oneapi/setvars.sh
cmake -B build . \
    -DCMAKE_C_COMPILER=mpiicc

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

1. Install MS-MPI SDK
2. Open CMake GUI or use command line:

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Using MinGW-w64

```bash
# Ensure MinGW is in PATH
cmake -B build -G "MinGW Makefiles" \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_PREFIX_PATH="C:/Program Files/Microsoft MPI/Bin"
cmake --build build
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `UNIMPI_BUILD_EXAMPLES` | ON | Build example programs |
| `UNIMPI_BUILD_TESTS` | ON | Build test suite |
| `UNIMPI_ENABLE_STD_MACROS` | OFF | Enable standard MPI naming macros |
| `BUILD_SHARED_LIBS` | OFF | Build shared instead of static library |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation prefix |

### Example: Minimal Build

```bash
cmake -B build . \
    -DUNIMPI_BUILD_EXAMPLES=OFF \
    -DUNIMPI_BUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release
```

### Example: Debug Build with Tests

```bash
cmake -B build . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUNIMPI_BUILD_TESTS=ON
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

# Update PATH and LD_LIBRARY_PATH
export PATH=$HOME/.local/bin:$PATH
export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
```

### Uninstall

```bash
sudo cmake --build build --target uninstall
```

---

## Platform-Specific Notes

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get install cmake build-essential

# Install MPI (choose one)
sudo apt-get install libopenmpi-dev    # OpenMPI
sudo apt-get install libmpich-dev      # MPICH
```

### CentOS/RHEL/Fedora

```bash
# Install dependencies
sudo yum install cmake gcc

# Install MPI (choose one)
sudo yum install openmpi-devel         # OpenMPI
sudo yum install mpich-devel           # MPICH
```

### macOS

```bash
# Using Homebrew
brew install cmake

# Install MPI (choose one)
brew install openmpi                   # OpenMPI
brew install mpich                     # MPICH
```

### Windows with MSYS2

```bash
# Install MinGW and MS-MPI
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-gcc

# Download MS-MPI SDK from Microsoft
# Set environment variables
export MSMPI_INC=/c/Program\ Files/Microsoft\ SDKs/MPI/Include
export MSMPI_LIB64=/c/Program\ Files/Microsoft\ SDKs/MPI/Lib/x64
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

### Linker errors on Linux

```bash
# If getting undefined reference to dlopen
# (should be automatic, but if needed):
cmake -B build . -DCMAKE_C_FLAGS="-ldl"
```

### Windows: Cannot find msmpi.dll

```cmd
# Ensure MS-MPI is installed
# Download from: https://www.microsoft.com/download/details.aspx?id=57467

# Set environment
cmake -B build . \
    -DMSMPI_DIR="C:/Program Files/Microsoft MPI"
```

### Tests fail to find MPI library

```bash
# Set library path for tests
export LD_LIBRARY_PATH=/path/to/mpi/lib:$LD_LIBRARY_PATH
cd build && ctest
```

---

## Verifying Build

```bash
# Check library was built
ls -la build/libunimpi.a

# Check examples were built
ls -la build/examples/minimal

# Run a quick test
export UNIMPI_BACKEND=openmpi
./build/examples/minimal
```

---

## Cross-Compilation

### Building for Windows from Linux (MinGW)

```bash
# Install MinGW cross-compiler
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
