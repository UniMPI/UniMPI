# Examples

The examples demonstrate the two UniMPI API styles and representative MPI
features. They require a runtime library even though UniMPI itself builds
without MPI development headers.

## Build

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNIMPI_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

Select an exact runtime library from the same installation as the launcher:

```bash
MPI_PREFIX="$(brew --prefix open-mpi)"
export UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.dylib"
MPIEXEC="$MPI_PREFIX/bin/mpirun"
```

On Linux, replace the values with the resolved `libmpi.so` and matching
`mpirun`/`mpiexec`. On Windows:

```powershell
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"
$MPIEXEC = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"
```

## Example inventory

| Example | API/feature | Minimum ranks | Backend requirement |
|---|---|---:|---|
| `minimal` | Direct `unimpi` vtable calls | 1 | Open MPI, MPICH, Intel MPI, or MS-MPI |
| `std_style` | Standard `MPI_*` names through macros | 1 | Any supported backend |
| `backend_info` | Selected backend, library path, MPI/library version | 1 | Any supported backend |
| `thread_init` | `unimpi_init_thread` and negotiated thread level | 1 | Backend with `MPI_Init_thread` |
| `nonblocking` | `Isend`, `Irecv`, `Test`, `Wait`, `Sendrecv`, `Ibarrier` | 2 | Backend exporting these operations |
| `collective` | Broadcast, reduce, all-reduce, gather/all-gather, scatter, scan | 2; 4 recommended | Backend exporting these collectives |
| `rma` | Window allocation, fence epochs, ring `Put`, and result validation | 2 | Backend exporting the demonstrated RMA calls |

The table describes the calls used by each executable; it is not a statement
that every operation in that MPI category is covered.

## Run

```bash
"$MPIEXEC" -np 1 ./build/examples/minimal
"$MPIEXEC" -np 1 ./build/examples/std_style
"$MPIEXEC" -np 1 ./build/examples/backend_info
"$MPIEXEC" -np 1 ./build/examples/thread_init
"$MPIEXEC" -np 2 ./build/examples/nonblocking
"$MPIEXEC" -np 4 ./build/examples/collective
"$MPIEXEC" -np 2 ./build/examples/rma
```

Open MPI may require `--oversubscribe` on a workstation:

```bash
"$MPIEXEC" --oversubscribe -np 4 ./build/examples/collective
```

Windows PowerShell:

```powershell
& $MPIEXEC -np 1 .\build\examples\Release\minimal.exe
& $MPIEXEC -np 1 .\build\examples\Release\backend_info.exe
& $MPIEXEC -np 2 .\build\examples\Release\nonblocking.exe
```

## Backend selection

`UNIMPI_LIBRARY` has precedence over `UNIMPI_BACKEND`, which has precedence over
the platform fallback. Exact paths are recommended:

```bash
UNIMPI_LIBRARY=/absolute/path/to/libmpi.so \
  /matching/path/to/mpirun -np 2 ./build/examples/nonblocking
```

Do not select one backend while launching with another implementation.

## Smoke tests

When examples and real MPI tests are enabled, CTest registers reduced example
smoke runs:

```bash
ctest --test-dir build -L example --output-on-failure
```

Smoke tests verify startup and the documented happy path. See
[`docs/SUPPORT_MATRIX.md`](../docs/SUPPORT_MATRIX.md) for the broader
verification boundary.
