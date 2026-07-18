# unimpi

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Universal MPI** - A zero-overhead MPI wrapper with runtime backend loading.

`unimpi` allows you to write MPI code once and run it with **OpenMPI**, **MPICH**, **Intel-MPI**, or **MS-MPI** without recompiling. Simply set an environment variable to switch backends at runtime.

---

## Features

- **🚀 Zero Overhead**: Direct function pointer calls, single indirection (<1ns overhead)
- **🔄 Runtime Backend Loading**: Switch MPI implementations without recompiling
- **📦 Four Backends**: OpenMPI, MPICH, Intel-MPI, MS-MPI
- **✅ MPI-3 Compatible**: Full MPI-3 API exposure (400+ functions)
- **🎯 Dual API**: Function pointer style or standard MPI macros
- **🪟 Cross-Platform**: Linux, macOS, Windows

---

## Quick Start

### Installation

```bash
git clone https://github.com/yourusername/unimpi.git
cd unimpi
cmake -B build .
cmake --build build
sudo cmake --install build
```

### Basic Usage (Standard MPI Style)

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"
#include <stdio.h>

int main(int argc, char **argv) {
    /* Initialize (auto-detects backend) */
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d\n", rank, size);

    MPI_Finalize();
    return 0;
}
```

**Compile with CMake:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(hello C)

find_package(unimpi CONFIG REQUIRED)
add_executable(hello hello.c)
target_link_libraries(hello PRIVATE unimpi::unimpi)
```

**Compile manually:**
```bash
gcc -o hello hello.c -lunimpi
```

**Run with different backends:**
```bash
# Auto-detect
./hello

# Force OpenMPI
UNIMPI_BACKEND=openmpi ./hello

# Force Intel MPI
UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so ./hello

# Windows with MS-MPI
set UNIMPI_BACKEND=msmpi
hello.exe
```

---

## Supported Backends

| Backend | Linux | macOS | Windows | Library Name |
|---------|-------|-------|---------|--------------|
| OpenMPI | ✅ | ✅ | ❌ | `libmpi.so.40` |
| MPICH | ✅ | ✅ | ❌ | `libmpich.so` |
| Intel-MPI | ✅ | ✅ | ❌ | `libmpi.so` |
| MS-MPI | ❌ | ❌ | ✅ | `msmpi.dll` |

See [BACKENDS.md](docs/BACKENDS.md) for detailed comparison.

---

## API Styles

### Standard MPI Style ⭐ **Recommended**

Use standard MPI function names for maximum compatibility:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

MPI_Init(&argc, &argv);
MPI_Send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
MPI_Recv(buf, count, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
MPI_Finalize();
```

### Function Pointer Style

Use the vtable directly for explicit control:

```c
#include "unimpi.h"

unimpi.init(&argc, &argv);
unimpi.send(buf, count, UNIMPI_INT, dest, tag, UNIMPI_COMM_WORLD);
unimpi.recv(buf, count, UNIMPI_INT, source, tag, UNIMPI_COMM_WORLD, &status);
unimpi.finalize();
```

**Use when:**
- Debugging backend loading issues
- Building higher-level abstractions
- Need explicit control over backend selection

---

## Documentation

- [BUILDING.md](docs/BUILDING.md) - Detailed build instructions
- [BACKENDS.md](docs/BACKENDS.md) - Backend selection guide
- [API.md](docs/API.md) - API reference
- [WINDOWS.md](docs/WINDOWS.md) - Windows/MS-MPI guide
- [CONTRIBUTING.md](docs/CONTRIBUTING.md) - Contribution guidelines
- [design.md](docs/design.md) - Architecture and design

---

## Testing

```bash
cmake -B build -DUNIMPI_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

The test suite includes:
- Unit tests (error handling, loader, lifecycle)
- MPI communication tests (P2P, collectives, datatypes)
- Backend validation tests

---

## Performance

Benchmark the overhead:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target bench_overhead
./build/tests/bench_overhead
```

Typical results show <1ns overhead per call compared to native MPI.

---

## License

MIT License - see [LICENSE](LICENSE) file.

---

## Contributing

Contributions welcome! See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## Acknowledgments

- OpenMPI, MPICH, Intel-MPI, MS-MPI developers
- MPI Forum for the MPI standard
