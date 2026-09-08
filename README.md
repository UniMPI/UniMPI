# UniMPI

**Current release:** [v0.2.1-alpha](CHANGELOG.md) (2026-09-08) - the full
MPI-2.2 base surface **305 / 305 canonical entities exposed (100%)**, the full
MPI-3.0 C-callable roster **95 / 95**, and the ten MPI-3.1 additions
(`Comm_idup`, `Aint_add`/`Aint_diff`, the four `File_*_all` nonblocking I/O,
and the three `MPI_T_*_get_index` lookups) **10 / 10**, each under its
`UNIMPI_MPI_AT_LEAST(maj,min)` build gate.

UniMPI is a C99 runtime-dispatch layer for MPI. An application links to UniMPI
once, then loads Open MPI, MPICH, Intel MPI, or Microsoft MPI at runtime.

The current interface contains **370 MPI function-pointer fields** (the
language-level total at a 3.1 target) and **367 direct standard-name aliases**,
covering the full MPI-2.2 base surface plus MPI-3.0 basic support (neighbor
collectives, nonblocking collectives, one-sided RMA atomics, `_x` large-count
queries, communicator helpers, and the MPI_T tools interface incl. category /
enum introspection and the `get_info` slots bound with the standard 10/13-arg
signatures) and the MPI-3.1 additions. Fields are gated behind
`UNIMPI_MPI_AT_LEAST(3,0)` / `UNIMPI_MPI_AT_LEAST(3,1)` and only appear when
the target version includes them: 370 fields at 3.1, **363 at 3.0**, **299 at
the default target 2.2** (the guaranteed base). Run `tools/count_surface.py`
for the current counts, or `test_vtable_layout` for a specific target build's
exact `UNIMPI_VTABLE_COUNT`. See the
[support matrix](docs/SUPPORT_MATRIX.md) for what is exercised on each backend.

## What it provides

- Runtime backend loading through `dlopen`/`dlsym` or Windows loader APIs.
- Open MPI and MPICH on Linux and macOS, Intel MPI on Linux, and MS-MPI on
  Windows.
- A direct vtable API and optional standard `MPI_*` naming macros.
- Fake-backend unit tests plus real multi-process backend tests.
- Examples and opt-in benchmark programs.

UniMPI adds a function-pointer dispatch step. End-to-end cost depends on the MPI
operation, backend, compiler, hardware, and process placement; the project does
not claim an unmeasured fixed nanosecond overhead.

## Performance

Dispatch is a macro layer (`#define MPI_Send unimpi.send`, 367 direct
standard-name aliases) over a global table populated once at `MPI_Init` via
`dlsym`. Steady-state calls add no symbol lookup and no wrapper frame — on top
of what a dynamically linked native MPI already pays through the PLT, UniMPI
adds a single `mov` + indirect `call`, on the order of a few cycles. For any
real MPI work (buffering, matching, or data movement) the dispatch cost is a
rounding error.

See [PERFORMANCE.md](docs/PERFORMANCE.md) for the full cost model, the reasons
this is not a "slow abstraction layer", and how to measure the paired
direct-vs-vtable delta on your own hardware ([BENCHMARKS.md](docs/BENCHMARKS.md)).

## Build

```bash
git clone https://github.com/UniMPI/UniMPI.git
cd UniMPI

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build --prefix "$PWD/install"
```

Building the library does not require MPI headers. A real MPI installation is
needed to run applications, integration tests, examples, and benchmarks.
The project files support CMake 3.10. The command examples use newer CLI
conveniences: `--parallel` requires CMake 3.12, `-S`/`-B` require CMake 3.13,
and `cmake --install` requires CMake 3.15. Older-client equivalents are
documented in [BUILDING.md](docs/BUILDING.md).

## Use from CMake

```cmake
cmake_minimum_required(VERSION 3.10)
project(hello C)

find_package(unimpi CONFIG REQUIRED)
add_executable(hello hello.c)
target_link_libraries(hello PRIVATE unimpi::unimpi)
```

```c
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    int rank;
    int size;

    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        return 1;
    }
    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS ||
        MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS) {
        MPI_Finalize();
        return 1;
    }

    printf("Hello from rank %d of %d\n", rank, size);
    return MPI_Finalize() == MPI_SUCCESS ? 0 : 1;
}
```

## Select a backend

UniMPI resolves the runtime in this order:

1. `UNIMPI_LIBRARY` — exact library path;
2. `UNIMPI_BACKEND` — backend name;
3. platform fallback name.

Use an exact path in CI and whenever more than one MPI is installed:

```bash
# Homebrew Open MPI
MPI_PREFIX="$(brew --prefix open-mpi)"
export UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.dylib"
"$MPI_PREFIX/bin/mpirun" -np 2 ./hello

# Intel MPI
source /opt/intel/oneapi/mpi/latest/env/vars.sh
export UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so
/opt/intel/oneapi/mpi/latest/bin/mpiexec -np 2 ./hello
```

```powershell
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"
& "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe" -np 2 .\hello.exe
```

The launcher and library must come from the same MPI installation.

## Platform matrix

| Backend | Linux | macOS | Windows |
|---|---:|---:|---:|
| Open MPI | Yes | Yes | No |
| MPICH | Yes | Yes | No |
| Intel MPI | Yes | No | No |
| MS-MPI | No | No | Yes |

The POSIX fallback name is `libmpi.so.40`; it is not reliable on macOS and can
be ambiguous on Linux. Prefer `UNIMPI_LIBRARY`. Details and exact-path examples
are in [BACKENDS.md](docs/BACKENDS.md).

## API styles

Standard names are enabled per translation unit:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

MPI_Barrier(MPI_COMM_WORLD);
```

The direct style makes dispatch explicit:

```c
#include "unimpi.h"

unimpi.barrier(UNIMPI_COMM_WORLD);
```

Call `unimpi_init`, `unimpi_init_thread`, or their standard-name equivalents
before dereferencing runtime-populated vtable fields.

To read the source / tag / error of an `MPI_Status`, use the standard-named
accessors `MPI_Status_get_source` / `MPI_Status_get_tag` /
`MPI_Status_get_error` (mapping to `unimpi.status_get_source` / `_tag` /
`_error`). Do **not** read `status.MPI_SOURCE` directly: the status layout is
backend-dependent and not portable. See [docs/API.md](docs/API.md) "Reading
the status fields".

## Tests

Run fake-backend unit tests without a real MPI installation:

```bash
cmake -S . -B build-unit -G Ninja \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=OFF
cmake --build build-unit --parallel
ctest --test-dir build-unit -L unit --output-on-failure
```

Real integration tests require a matching launcher and `UNIMPI_LIBRARY`:

```bash
cmake -S . -B build-mpi -G Ninja \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DMPIEXEC_EXECUTABLE=/path/to/mpirun
cmake --build build-mpi --parallel
UNIMPI_LIBRARY=/absolute/path/to/libmpi.so \
  ctest --test-dir build-mpi -L integration \
  --output-on-failure --timeout 180
```

See [TESTING.md](docs/TESTING.md) for labels, process-count coverage,
sanitizers, and the backend matrix.

## Benchmarks

Benchmarks are opt-in and have no hard CI performance threshold:

```bash
cmake -S . -B build-bench -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DUNIMPI_BUILD_BENCHMARKS=ON \
  -DMPIEXEC_EXECUTABLE=/path/to/mpirun
cmake --build build-bench --parallel

UNIMPI_LIBRARY=/absolute/path/to/libmpi.so \
  /path/to/mpirun -np 2 ./build-bench/benchmarks/bench_latency \
  --smoke --format text
```

See [BENCHMARKS.md](docs/BENCHMARKS.md) for methodology and CSV output.

## Documentation

- [Documentation index](docs/README.md)
- [Build and install](docs/BUILDING.md)
- [Backend selection](docs/BACKENDS.md)
- [Testing](docs/TESTING.md)
- [Benchmarks](docs/BENCHMARKS.md)
- [Support and verification matrix](docs/SUPPORT_MATRIX.md)
- [Examples](examples/README.md)
- [Public API](docs/API.md)
- [Windows and MS-MPI](docs/WINDOWS.md)
- [Architecture](docs/design.md)
- [Contributing](docs/CONTRIBUTING.md)

## Contributing

Contributions are welcome. New backend or API work should include unit/fake
coverage, focused real-backend tests where applicable, and documentation of
the verified boundary. See [CONTRIBUTING.md](docs/CONTRIBUTING.md).
