# unimpi

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Universal MPI** - A zero-overhead MPI wrapper with runtime backend loading.

`unimpi` allows you to write MPI code once and run it with **OpenMPI**, **MPICH**, **Intel-MPI**, or **MS-MPI** without recompiling. Simply set an environment variable to switch backends at runtime.

---

## Features

- **🚀 Zero Overhead**: Direct function pointer calls, single indirection
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

**Compile:**
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

Use standard MPI function names for maximum compatibility and portability:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Hello from rank %d of %d\n", rank, size);
    
    MPI_Finalize();
    return 0;
}
```

**Benefits:**
- Drop-in replacement for existing MPI code
- Standard naming familiar to all MPI programmers
- Easy to port existing applications
- IDE autocomplete works with standard names

**Compile:**
```bash
gcc -o hello hello.c -lunimpi
```

### Function Pointer Style (Advanced)

Direct vtable access for zero-overhead scenarios:

```c
#include "unimpi.h"

unimpi.init(&argc, &argv);
unimpi.send(buf, count, MPI_INT, dest, tag, UNIMPI_COMM_WORLD);
unimpi.finalize();
```

**Use when:**
- You need explicit control over backend selection
- Debugging backend loading issues
- Building higher-level abstractions

---

## Documentation

- [BUILDING.md](docs/BUILDING.md) - Detailed build instructions
- [BACKENDS.md](docs/BACKENDS.md) - Backend selection guide
- [API.md](docs/API.md) - API reference
- [WINDOWS.md](docs/WINDOWS.md) - Windows/MS-MPI guide
- [CONTRIBUTING.md](docs/CONTRIBUTING.md) - Contribution guidelines
- [design.md](docs/design.md) - Architecture and design

---

## Performance

`unimpi` adds **< 1ns overhead** per MPI call (single function pointer indirection).

Benchmark results with Intel MPI:
- Latency: ~0.28μs for small messages
- Bandwidth: ~21 GB/s for 1MB messages
- Barrier: ~0.35μs

See [tests/benchmark/](tests/benchmark/) for benchmark suite.

---

## Testing

```bash
cd build
ctest --output-on-failure
```

All tests pass with each backend:
- test_error
- test_loader
- test_p2p
- test_collective
- test_datatype

---

## License

MIT License - see [LICENSE](LICENSE) file.

---

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Acknowledgments

- OpenMPI, MPICH, Intel-MPI, MS-MPI developers
- MPI Forum for the MPI standard
