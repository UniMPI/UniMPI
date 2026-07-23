# Building UniMPI

UniMPI is a C99 project built with CMake 3.10 or newer. The library resolves
MPI at runtime, so an MPI SDK is not required for a library-only build. A real
MPI installation is required for integration tests, examples, and benchmarks.

The commands below use `cmake --build ... --parallel` (CMake 3.12+),
`cmake -S ... -B ...` (CMake 3.13+), and `cmake --install` (CMake 3.15+).
With CMake 3.10 or 3.11, omit `--parallel`. With CMake 3.10 through 3.12,
create and enter the build directory, then run `cmake /path/to/unimpi`.
With CMake 3.10 through 3.14, configure
`-DCMAKE_INSTALL_PREFIX=/desired/prefix` and install with
`cmake --build . --target install` from that build directory.

## Quick build

```bash
git clone https://github.com/UniMPI/UniMPI.git
cd UniMPI

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Visual Studio is a multi-configuration generator:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

## CMake options

| Option | Default | Purpose |
|---|---:|---|
| `UNIMPI_BUILD_EXAMPLES` | `ON` | Build programs under `examples/` |
| `UNIMPI_BUILD_TESTS` | `ON` | Build and register the test suite |
| `UNIMPI_BUILD_MPI_TESTS` | `OFF` | Register tests that launch a real MPI runtime |
| `UNIMPI_BUILD_BENCHMARKS` | `OFF` | Build benchmark executables and smoke registrations |
| `UNIMPI_ENABLE_STD_MACROS` | `OFF` | Export standard-name macros to consumers of the target |
| `UNIMPI_INSTALL_DOCS` | `ON` | Install the maintained Markdown documentation |
| `BUILD_SHARED_LIBS` | `OFF` | Build `unimpi` shared instead of static |
| `CMAKE_INSTALL_PREFIX` | Platform default | Installation prefix |

Library-only build:

```bash
cmake -S . -B build-min -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNIMPI_BUILD_EXAMPLES=OFF \
  -DUNIMPI_BUILD_TESTS=OFF \
  -DUNIMPI_BUILD_BENCHMARKS=OFF \
  -DUNIMPI_INSTALL_DOCS=OFF
cmake --build build-min --parallel
```

Full developer build with real MPI and smoke benchmarks:

```bash
cmake -S . -B build-dev -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_EXTENSIONS=OFF \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DUNIMPI_BUILD_BENCHMARKS=ON \
  -DMPIEXEC_EXECUTABLE=/path/to/matching/mpirun
cmake --build build-dev --parallel
```

## Platform setup

### Linux: Open MPI or MPICH

Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install --no-install-recommends -y \
  cmake ninja-build build-essential openmpi-bin libopenmpi-dev
```

Use `mpich libmpich-dev` instead of the Open MPI packages for MPICH. If both
are installed, resolve the launcher and library from the intended package and
set `UNIMPI_LIBRARY` explicitly.

```bash
export UNIMPI_LIBRARY="$(readlink -f /path/to/mpi/lib/libmpi.so)"
/path/to/the/same/mpi/bin/mpirun -np 2 ./my_program
```

### macOS: Homebrew Open MPI or MPICH

```bash
brew install cmake ninja open-mpi

MPI_PREFIX="$(brew --prefix open-mpi)"
export UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.dylib"
```

For MPICH, install `mpich` and use `$(brew --prefix mpich)`. Homebrew formulas
can conflict at the unversioned `mpirun`/`mpiexec` links, so prefer the
launcher under the selected prefix.

The POSIX fallback `libmpi.so.40` is not a macOS library name. Always set
`UNIMPI_LIBRARY` for predictable macOS runs.

### Linux: Intel MPI

Install the oneAPI MPI development package, source its environment, and use
the runtime tuple from the same prefix:

```bash
source /opt/intel/oneapi/mpi/latest/env/vars.sh

export UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so
MPIEXEC=/opt/intel/oneapi/mpi/latest/bin/mpiexec

cmake -S . -B build-intel -G Ninja \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DMPIEXEC_EXECUTABLE="$MPIEXEC" \
  "-DMPIEXEC_PREFLAGS=-bootstrap;fork"
```

Some installations place `libmpi.so` under `lib/release`; use the file that is
actually present. Intel MPI is supported by this project on Linux, not macOS or
Windows.

### Windows: MS-MPI

Install the MS-MPI runtime and SDK. The launcher and DLL normally live in
different directories:

```powershell
$launcher = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"

cmake -S . -B build-msmpi -G "Visual Studio 17 2022" -A x64 `
  -DBUILD_SHARED_LIBS=OFF `
  -DUNIMPI_BUILD_TESTS=ON `
  -DUNIMPI_BUILD_MPI_TESTS=ON `
  "-DMPIEXEC_EXECUTABLE=$launcher"
cmake --build build-msmpi --config Release --parallel
```

See [WINDOWS.md](WINDOWS.md) for installation checks and troubleshooting.

## Unit tests

The default test configuration uses generated fake libraries and does not
require a real MPI installation:

```bash
cmake -S . -B build-unit -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=OFF \
  -DUNIMPI_BUILD_BENCHMARKS=OFF
cmake --build build-unit --parallel
ctest --test-dir build-unit -L unit --output-on-failure
```

## Real MPI tests

Set `MPIEXEC_EXECUTABLE` during configuration and `UNIMPI_LIBRARY` while
running. The two must identify the same implementation:

```bash
cmake -S . -B build-mpi -G Ninja \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DMPIEXEC_EXECUTABLE=/absolute/mpi/bin/mpirun \
  -DMPIEXEC_PREFLAGS=--oversubscribe
cmake --build build-mpi --parallel

UNIMPI_LIBRARY=/absolute/mpi/lib/libmpi.so \
  ctest --test-dir build-mpi -L integration \
  --output-on-failure --timeout 180
```

Do not pass `-DCMAKE_C_COMPILER=mpicc` merely to select the runtime. UniMPI
loads MPI dynamically; the launcher/library pair selects the backend.

See [TESTING.md](TESTING.md) for labels, process counts, sanitizers, and direct
test commands.

## Benchmarks

Benchmarks are not part of a default build:

```bash
cmake -S . -B build-bench -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DUNIMPI_BUILD_BENCHMARKS=ON \
  -DMPIEXEC_EXECUTABLE=/absolute/mpi/bin/mpirun
cmake --build build-bench --parallel

UNIMPI_LIBRARY=/absolute/mpi/lib/libmpi.so \
  /absolute/mpi/bin/mpirun -np 2 \
  ./build-bench/benchmarks/bench_latency --smoke --format text
```

See [BENCHMARKS.md](BENCHMARKS.md) before interpreting results.

## Install and consume

Install into a user-controlled prefix:

```bash
cmake --install build --prefix "$PWD/install"
```

Downstream CMake:

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

find_package(unimpi CONFIG REQUIRED)
add_executable(myapp myapp.c)
target_link_libraries(myapp PRIVATE unimpi::unimpi)
```

Configure the consumer:

```bash
cmake -S consumer -B consumer-build \
  -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build consumer-build --parallel
```

## Troubleshooting

### Backend library does not load

- Verify the file exists.
- Inspect dependencies with `ldd`, `otool -L`, or
  `dumpbin /DEPENDENTS`, as appropriate.
- Set `UNIMPI_LIBRARY` to the exact file.
- Confirm its architecture matches the executable.

### Launcher and library disagree

Compare the launcher version with the selected prefix:

```bash
/absolute/mpi/bin/mpirun --version
ls -l /absolute/mpi/lib/libmpi*
```

Mixing installations may fail during initialization or produce ABI corruption.

### CMake does not find MPI for integration tests

Point CMake at the intended prefix or launcher:

```bash
cmake -S . -B build \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DMPI_HOME=/absolute/mpi/prefix \
  -DMPIEXEC_EXECUTABLE=/absolute/mpi/bin/mpirun
```

The library-only build remains available by setting
`UNIMPI_BUILD_MPI_TESTS=OFF`.
