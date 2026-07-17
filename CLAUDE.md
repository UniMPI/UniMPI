# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**unimpi** is a Universal MPI wrapper library that provides runtime backend loading for OpenMPI, MPICH, Intel-MPI, and MS-MPI. It allows writing MPI code once and running it with any supported MPI implementation without recompiling.

**Key Characteristics:**
- Zero-overhead design (single function pointer indirection, <1ns overhead)
- Runtime backend detection and loading via `dlopen`/`dlsym`
- Full MPI-3 API coverage (400+ functions)
- Cross-platform: Linux, macOS, Windows
- Dual API style: function pointer (`unimpi.send`) or standard MPI macros (`MPI_Send`)

## Architecture

### Core Components

```
include/unimpi.h          # Public API header
include/unimpi_vtable.h    # Vtable structure with 400+ function pointers
include/unimpi_platform.h  # Platform abstraction (dlopen, dlclose, dlsym)
include/unimpi_loader.h    # Backend detection and loading

src/core.c                 # Initialization and cleanup
src/loader.c              # Backend auto-detection, library loading
src/vtable.c              # Vtable management and backend dispatch
src/platform_posix.c       # POSIX implementation (dlfcn.h)
src/platform_windows.c     # Windows implementation (LoadLibrary)

src/backends/
  openmpi.c               # OpenMPI-specific initialization
  mpich.c                 # MPICH initialization (also base for Intel-MPI)
  intelmpi.c              # Intel-MPI specific (derived from MPICH)
  msmpi.c                 # MS-MPI initialization (Windows, MPICH-derived)
```

### Backend Detection Priority

1. `UNIMPI_BACKEND` environment variable (manual selection)
2. `UNIMPI_LIBRARY` environment variable (custom path)
3. Auto-detection: OpenMPI → Intel-MPI → MPICH → MS-MPI (Windows only)

### Key Implementation Details

**Communicator Values (backend-specific):**
- OpenMPI: Uses pointers (e.g., `ompi_mpi_comm_world`)
- MPICH/Intel-MPI: Uses integers (`MPI_COMM_WORLD = 91`)
- MS-MPI: Uses hardcoded values (`0x44000000` for WORLD, `0x44000001` for SELF)

**Vtable Population:** Each backend fills the global `unimpi_vtable_t unimpi` structure with function pointers loaded from the backend library.

## Build Commands

### Quick Build (Linux/macOS)
```bash
cmake -B build .
cmake --build build
```

### Windows (MinGW)
```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### Windows (Visual Studio)
```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Build Options
```bash
# Minimal build (no examples/tests)
cmake -B build . -DUNIMPI_BUILD_EXAMPLES=OFF -DUNIMPI_BUILD_TESTS=OFF

# Debug build
cmake -B build . -DCMAKE_BUILD_TYPE=Debug

# Enable standard MPI macros by default
cmake -B build . -DUNIMPI_ENABLE_STD_MACROS=ON
```

## Test Commands

### Run All Tests
```bash
cd build && ctest --output-on-failure
```

### Run Single Test
```bash
# Unit tests (no MPI runtime needed)
./build/tests/test_error
./build/tests/test_loader

# MPI tests (requires mpirun/mpiexec)
mpirun -np 2 ./build/tests/test_p2p
mpirun -np 4 ./build/tests/test_collective
mpiexec -np 2 ./build/tests/test_datatype
```

### Test with Specific Backend
```bash
# Linux/macOS
UNIMPI_BACKEND=openmpi mpirun -np 4 ./build/examples/minimal
UNIMPI_BACKEND=mpich mpirun -np 4 ./build/examples/minimal

# Windows
set UNIMPI_BACKEND=msmpi
mpiexec -np 4 .\build\examples\minimal.exe
```

## Backend Development Guidelines

### Adding a New Backend

1. **Add backend detection** in `src/loader.c`:
   - Add symbol check in `unimpi_loader_identify_backend()`
   - Add library name to `unimpi_backends[]` array

2. **Create backend file** `src/backends/<new>.c`:
   - Implement `unimpi_vtable_init_<new>()`
   - Load all MPI function symbols
   - Set communicator constants if backend-specific

3. **Update vtable dispatch** in `src/vtable.c`:
   - Add forward declaration
   - Add case in switch statement

4. **Test with all MPI function categories**:
   - Point-to-point, Collectives, Datatypes
   - RMA (one-sided), Parallel I/O, Dynamic processes

### Backend-Specific Constants

Different MPI implementations use different internal representations:

```c
// OpenMPI: pointers to internal structures
// MPICH: small integers (91, 92)
// MS-MPI: hardcoded magic numbers (0x44000000)
```

Always verify communicator values match the backend when implementing new backends.

## Common Development Tasks

### Running Examples
```bash
# Build examples
cmake -B build . -DUNIMPI_BUILD_EXAMPLES=ON

# Run with auto-detection
./build/examples/minimal

# Run with specific backend
UNIMPI_BACKEND=openmpi ./build/examples/minimal
UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so ./build/examples/minimal
```

### Debugging Backend Loading
```bash
# Enable verbose output (unimpi prints to stderr)
UNIMPI_BACKEND=openmpi ./build/examples/minimal
# Output: [unimpi] Loading backend library: libmpi.so.40
```

### Checking Function Coverage
Compare function implementations across backends:
```bash
grep -c "unimpi_platform_dlsym" src/backends/openmpi.c
grep -c "unimpi_platform_dlsym" src/backends/msmpi.c
```

## Platform Considerations

### Windows/MS-MPI
- Library: `msmpi.dll` in `C:\Windows\System32`
- Communicator values: `0x44000000` (WORLD), `0x44000001` (SELF)
- Build with MinGW or Visual Studio
- No `dlopen` - uses `LoadLibrary`/`GetProcAddress`

### Linux
- Standard `dlopen`/`dlsym` with `-ldl`
- Library paths vary by distribution
- Versioned libraries (`.so.40` for OpenMPI v4.x)

### macOS
- Same as Linux but with `.dylib` suffix
- Homebrew paths for MPI installations

## API Styles

### Standard MPI Style (Recommended)
```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

MPI_Init(&argc, &argv);
MPI_Send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
MPI_Finalize();
```

### Function Pointer Style
```c
#include "unimpi.h"

unimpi.init(&argc, &argv);
unimpi.send(buf, count, MPI_INT, dest, tag, UNIMPI_COMM_WORLD);
unimpi.finalize();
```

Use standard style for portability, function pointer style for debugging or explicit control.
