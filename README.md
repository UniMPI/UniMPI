# TFTK-MPI Wrapper

Zero-overhead MPI wrapper with runtime backend loading for OpenMPI, MPICH, Intel-MPI, and MS-MPI.

## Features

- **Zero Overhead**: Direct function pointer calls, no extra layers
- **Multi-Backend**: Runtime loading of OpenMPI, MPICH, Intel-MPI, MS-MPI
- **MPI-3 Compatible**: Full MPI-3 API exposure
- **Dual API Style**: Function pointer style or standard MPI macros

## Quick Start

```c
#include "tftk_mpi.h"

int main(int argc, char **argv) {
    /* Initialize (auto-detects backend) */
    tftk_mpi_init(&argc, &argv);

    /* Use MPI through vtable */
    int rank, size;
    tftk_mpi.comm_rank(TFTK_MPI_COMM_WORLD, &rank);
    tftk_mpi.comm_size(TFTK_MPI_COMM_WORLD, &size);

    printf("Hello from %d of %d\n", rank, size);

    tftk_mpi_finalize();
    return 0;
}
```

## Building

```bash
cmake -B build .
cmake --build build
```

## Backend Selection

Environment variables:
- `TFTK_MPI_BACKEND=openmpi` - Use specific backend
- `TFTK_MPI_LIBRARY=/path/to/libmpi.so` - Use custom library path

## API Styles

### Function Pointer Style (Zero Overhead)
```c
tftk_mpi.send(buf, count, MPI_INT, dest, tag, TFTK_MPI_COMM_WORLD);
```

### Standard MPI Style (with macros)
```c
#define TFTK_MPI_USE_STD_NAMES
#include "tftk_mpi.h"
MPI_Send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
```

## License

MIT
