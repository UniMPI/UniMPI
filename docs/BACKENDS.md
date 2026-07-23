# Backend Selection

UniMPI loads one native MPI implementation into a process. Backend selection
does not convert one implementation's ABI into another, and the launcher must
match the selected library.

## Supported combinations

| Backend | Linux | macOS | Windows | Native library |
|---|---:|---:|---:|---|
| Open MPI | Yes | Yes | No | `libmpi.so*` / `libmpi.dylib` |
| MPICH | Yes | Yes | No | `libmpi.so*` / `libmpi.dylib` |
| Intel MPI | Yes | No | No | `libmpi.so` |
| MS-MPI | No | No | Yes | `msmpi.dll` |

This table states the intended platform/backend combinations. API-category
verification is tracked separately in [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).

## Selection priority

The loader evaluates:

1. `UNIMPI_LIBRARY`;
2. `UNIMPI_BACKEND`;
3. the platform fallback name.

If both environment variables are set, `UNIMPI_LIBRARY` wins.

```bash
export UNIMPI_LIBRARY=/absolute/path/to/libmpi.so
export UNIMPI_BACKEND=mpich

# The exact path above is used; UNIMPI_BACKEND is ignored.
./my_program
```

`UNIMPI_BACKEND` maps names to compiled-in basenames:

| Value | Basename |
|---|---|
| `openmpi` | `libmpi.so.40` |
| `mpich` | `libmpi.so` |
| `intelmpi` | `libmpi.so` |
| `msmpi` | `msmpi.dll` |

Those names can be ambiguous or unavailable on a particular installation.
Therefore, an exact `UNIMPI_LIBRARY` path is the recommended production and CI
configuration.

The automatic fallback is `msmpi.dll` on Windows and `libmpi.so.40` on POSIX.
It is not a comprehensive search. On macOS, set an exact `.dylib` path.

## Open MPI

Linux:

```bash
MPIEXEC=/absolute/openmpi/bin/mpirun
export UNIMPI_LIBRARY="$(readlink -f /absolute/openmpi/lib/libmpi.so)"

"$MPIEXEC" --oversubscribe -np 4 ./my_program
```

Homebrew macOS:

```bash
MPI_PREFIX="$(brew --prefix open-mpi)"
export UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.dylib"

"$MPI_PREFIX/bin/mpirun" --oversubscribe -np 4 ./my_program
```

Open MPI uses pointer-style predefined handles. UniMPI resolves those runtime
objects in the Open MPI backend adapter; applications must not substitute
numeric values from another implementation.

## MPICH

Linux:

```bash
MPIEXEC=/absolute/mpich/bin/mpiexec
export UNIMPI_LIBRARY="$(readlink -f /absolute/mpich/lib/libmpi.so)"

"$MPIEXEC" -np 4 ./my_program
```

Homebrew macOS:

```bash
MPI_PREFIX="$(brew --prefix mpich)"
export UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.dylib"

"$MPI_PREFIX/bin/mpiexec" -np 4 ./my_program
```

Do not point `UNIMPI_LIBRARY` at `libmpich.so` merely because its filename
contains "mpich". Select the library that provides the standard MPI C symbols
used by the matching launcher, normally `libmpi`.

## Intel MPI

Intel MPI is supported on Linux. Source its environment so dependent oneAPI
libraries are discoverable:

```bash
source /opt/intel/oneapi/mpi/latest/env/vars.sh

MPI_PREFIX=/opt/intel/oneapi/mpi/latest
export UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.so"

"$MPI_PREFIX/bin/mpiexec" -bootstrap fork -np 4 ./my_program
```

Some releases place the runtime under `lib/release`. Resolve the existing file
rather than assuming one layout:

```bash
find "$MPI_PREFIX/lib" -name libmpi.so -print
```

Intel MPI is MPICH-derived, but UniMPI identifies it separately and initializes
the Intel backend adapter.

## MS-MPI

MS-MPI is the supported Windows backend. The runtime executable and DLL are
normally in different directories:

```powershell
$MPIEXEC = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"

& $MPIEXEC -np 4 .\my_program.exe
```

The MS-MPI SDK is needed for CMake's real-MPI discovery and native MPI
development. The runtime package provides `mpiexec.exe` and `msmpi.dll`.
Executable and DLL architectures must match.

Linux and WSL processes cannot load a Windows DLL. Build and run a Windows
executable when using MS-MPI.

See [WINDOWS.md](WINDOWS.md) for detailed checks.

## Diagnosing a selection

The `backend_info` example prints the selected backend and library path:

```bash
UNIMPI_LIBRARY=/absolute/mpi/lib/libmpi.so \
  /absolute/mpi/bin/mpirun -np 1 ./build/examples/backend_info
```

Also verify the tuple outside UniMPI:

```bash
/absolute/mpi/bin/mpirun --version
ls -l /absolute/mpi/lib/libmpi*
ldd /absolute/mpi/lib/libmpi.so
```

Use `otool -L` on macOS and `dumpbin /DEPENDENTS` or PowerShell file/version
inspection on Windows.

## Standard MPI ABI libraries

Library names containing `mpi_abi` or `mpi-abi` are rejected. UniMPI currently
loads native implementation ABIs because communicator, datatype, operation,
status, and other predefined objects are initialized by backend-specific code.
