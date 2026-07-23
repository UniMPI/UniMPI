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

### Initialization Flow

The library follows a strict initialization sequence:

1. **Environment Check** (`src/loader.c:unimpi_loader_detect_backend`)
   - Check `UNIMPI_BACKEND` environment variable (manual selection)
   - Check `UNIMPI_LIBRARY` environment variable (custom path)
   - Auto-detect based on platform

2. **Library Loading** (`src/loader.c:unimpi_loader_load`)
   - Reject standard MPI ABI libraries (not supported)
   - Platform-specific `dlopen`/`LoadLibrary`
   - Returns handle for symbol resolution

3. **Backend Identification** (`src/loader.c:unimpi_loader_identify_backend`)
   - OpenMPI: Check for `ompi_mpi_comm_world` symbol
   - Intel-MPI: Check for `__I_MPI___cpu_core_type` symbol
   - MPICH: Check for `MPIR_Err_create_code` or `MPIR_Dup_fn` symbols
   - MS-MPI: Check for `MSMPI_Get_version` symbol (Windows only)

4. **Platform Support Check** (`src/loader.c:unimpi_loader_check_platform_support`)
   - Windows: Only MS-MPI supported
   - macOS: OpenMPI and MPICH supported
   - Linux: OpenMPI, MPICH, and Intel-MPI supported

5. **Vtable Population** (`src/vtable.c:unimpi_vtable_init`)
   - Validate core symbols (MPI_Init, MPI_Finalize, MPI_Comm_size, MPI_Comm_rank)
   - Dispatch to backend-specific initialization
   - Each backend fills 400+ function pointers via `unimpi_platform_dlsym`
   - Set communicator constants specific to backend ABI

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

# Enable real MPI tests (requires mpirun/mpiexec)
cmake -B build . -DUNIMPI_BUILD_MPI_TESTS=ON
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

## Development Guidelines

### Always Use Native MPI Style in Applications
**Important**: Applications using unimpi should always use native MPI naming style (`MPI_*`) rather than unimpi-specific prefixes (`UNIMPI_*`).

**Correct:**
```c
#include "unimpi.h"  // This header automatically provides MPI_* macros

MPI_Send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
MPI_Bcast(buf, count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
MPI_Reduce(sendbuf, recvbuf, count, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
```

**Incorrect:**
```c
// Don't use UNIMPI_* prefixes in application code
unimpi.send(buf, count, UNIMPI_INT, dest, tag, UNIMPI_COMM_WORLD);
```

The `UNIMPI_*` prefixes are for internal implementation only. The public API should always present as standard MPI to users.

### Coding Standards

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style
- **Line length**: Max 100 characters
- **Naming**:
  - Functions: `snake_case`
  - Macros: `UPPER_CASE`
  - Types: `snake_case_t`
  - Global vtable: `unimpi`

### Commit Messages

Follow conventional commits format:
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation
- `test:` - Tests
- `refactor:` - Code refactoring
- `perf:` - Performance
- `chore:` - Maintenance

Example: `test: add extended P2P communication tests (ssend, rsend, sendrecv, waitall, test)`
