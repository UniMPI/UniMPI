# unimpi

Universal MPI wrapper with runtime backend loading for OpenMPI, MPICH, Intel-MPI, and MS-MPI.

## Features

- **Zero Overhead**: Direct function pointer calls, no extra layers
- **Multi-Backend**: Runtime loading of OpenMPI, MPICH, Intel-MPI, MS-MPI
- **MPI-3 Compatible**: Full MPI-3 API exposure
- **Dual API Style**: Function pointer style or standard MPI macros

## Quick Start

```c
#include "unimpi.h"

int main(int argc, char **argv) {
    /* Initialize (auto-detects backend) */
    unimpi_init(&argc, &argv);

    /* Use MPI through vtable */
    int rank, size;
    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);

    printf("Hello from %d of %d\n", rank, size);

    unimpi_finalize();
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
- `UNIMPI_BACKEND=openmpi` - Use specific backend
- `UNIMPI_LIBRARY=/path/to/libmpi.so` - Use custom library path

## API Styles

### Function Pointer Style (Zero Overhead)
```c
unimpi.send(buf, count, MPI_INT, dest, tag, UNIMPI_COMM_WORLD);
```

### Standard MPI Style (with macros)
```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"
MPI_Send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
```

## License

MIT
