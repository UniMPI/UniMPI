# Testing

UniMPI uses two complementary test layers:

- unit tests load fake shared libraries and do not require an MPI installation;
- integration tests launch a real MPI runtime and verify communication and
  backend-specific ABI behavior.

Passing unit tests is not a substitute for running the real backend matrix.

## Configure

### Unit and fake-backend tests

```bash
cmake -S . -B build-unit -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_EXTENSIONS=OFF \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=OFF \
  -DUNIMPI_BUILD_BENCHMARKS=OFF
cmake --build build-unit --parallel
ctest --test-dir build-unit -L unit --output-on-failure
```

CTest supplies the fake library path to tests that initialize UniMPI. Do not
depend on a developer machine's default `libmpi`.

### Real MPI integration tests

Resolve the launcher and library from the same installation:

```bash
MPI_PREFIX="$(brew --prefix open-mpi)"

cmake -S . -B build-openmpi -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_EXTENSIONS=OFF \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DMPIEXEC_EXECUTABLE="$MPI_PREFIX/bin/mpirun" \
  -DMPIEXEC_PREFLAGS=--oversubscribe
cmake --build build-openmpi --parallel

UNIMPI_LIBRARY="$MPI_PREFIX/lib/libmpi.dylib" \
  ctest --test-dir build-openmpi -L integration \
  --output-on-failure --timeout 180
```

On Linux, resolve symlinks where practical:

```bash
export UNIMPI_LIBRARY="$(readlink -f /path/to/mpi/lib/libmpi.so)"
ctest --test-dir build -L integration --output-on-failure --timeout 180
```

On Windows:

```powershell
$launcher = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DUNIMPI_BUILD_TESTS=ON `
  -DUNIMPI_BUILD_MPI_TESTS=ON `
  "-DMPIEXEC_EXECUTABLE=$launcher"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release -L integration `
  --output-on-failure --timeout 180
```

## CTest labels

Use labels to select the intended layer:

| Label | Contents | External MPI required |
|---|---|---:|
| `unit` | Error, loader, platform, lifecycle, vtable, identity, and backend-parity tests | No |
| `fake` | Unit tests that load generated fake MPI shared libraries | No |
| `integration` | Tests that initialize a real MPI runtime | Yes |
| `mpi` | Real-MPI execution layer, including singleton lifecycle cases | Yes |
| `example` | Example smoke tests | Yes |
| `benchmark` | Reduced-work benchmark smoke tests | Yes |

Useful commands:

```bash
ctest --test-dir build -N
ctest --test-dir build -L unit --output-on-failure
ctest --test-dir build -L 'integration|example' --output-on-failure
ctest --test-dir build -LE benchmark --output-on-failure
ctest --test-dir build -R 'loader|lifecycle' --output-on-failure
```

Benchmark labels run only smoke workloads under CTest. Full measurement runs
are manual; see [BENCHMARKS.md](BENCHMARKS.md).

With tests, real MPI, examples, and benchmarks all enabled, the current CTest
registry contains 49 invocations: 20 unit tests and 29 integration
invocations. The integration total includes 7 example smokes and 3 benchmark
smokes. Recount after changing registrations:

```bash
ctest --test-dir build -N
ctest --test-dir build -N -L unit
ctest --test-dir build -N -L integration
```

## Process-count matrix

MPI behavior changes with communicator size. The registered suite covers the
counts marked `Current`; the remaining rows are explicit extension guidance.
New integration tests should use the smallest count that exercises the
behavior and add an important partition shape when it can expose another path.

| Processes | Status | Primary scenarios |
|---:|---|---|
| 1 | Current | Lifecycle macros and single-rank example/dispatch smoke |
| 2 | Current | Point-to-point, request arrays, datatype, RMA, MPI I/O, environment queries |
| 3 | Gap/candidate | Smallest odd communicator split when the operation permits it |
| 4 | Current | Collectives, communicators, groups, Cartesian/graph topology, intercommunicators |
| 5 | Current | Odd asymmetric intercommunicator partition |
| 8 | Optional | Stress/scaling checks; not required for every change |

A test that skips for insufficient ranks must also have at least one registered
invocation with enough ranks to execute its assertions.

## Backend matrix

The GitHub Actions matrix exercises:

| Platform | Backend | Typical build |
|---|---|---|
| Ubuntu | Open MPI | Debug, GCC |
| Ubuntu | MPICH | Release, GCC |
| Ubuntu | Intel MPI | Release, oneAPI environment |
| macOS | Open MPI | Debug, AppleClang |
| macOS | MPICH | Release, AppleClang |
| Windows | MS-MPI | Release, MSVC x64 |
| Ubuntu | Fake backend | Debug, Clang, ASan/UBSan |

Do not remove a backend to make a test portable. Isolate implementation
differences in fixtures, backend adapters, or test expectations.

## Version-gated builds

The exposed MPI surface narrows with the target version chosen at build time:
`UNIMPI_MPI_TARGET_VERSION`/`UNIMPI_MPI_TARGET_SUBVERSION` (default 3/1)
physically remove everything above the target from vtable fields, standard
macros, and backend bindings. The library and any consumer must be compiled
with the same macros; CMake's `PUBLIC` propagation makes this automatic. See
[VERSION_GATING.md](VERSION_GATING.md) for the full mechanism, the gated
clusters, and the ABI consistency requirement.

Add the artifact-audit check to the validation matrix when a target-version
build is under test:

```bash
# 3.1 (default) build
strings libunimpi.a | grep -cE 'MPI_Ibcast|MPI_Comm_join|MPI_Iallreduce'   # -> 12
./build/tests/test_vtable_layout                                           # 2584 / 323

# 2.2 build (-DUNIMPI_MPI_TARGET_VERSION=2 -DUNIMPI_MPI_TARGET_SUBVERSION=2)
strings libunimpi.a | grep -cE 'MPI_Ibcast|MPI_Comm_create_group|MPI_Win_sync'   # -> 0
strings build22/libunimpi.a | grep -cE 'MPI_Alltoallw|MPI_Comm_join|MPI_Op_commutative'  # -> 12 (always present)
./build22/tests/test_vtable_layout                                         # 2256 / 282
```

At target 2.2 the whole-tree build is validated on the library and unit tests;
`examples/nonblocking.c`, `tests/mpi/test_collective_nonblocking.c`, and
`tests/mpi/test_environment_info.c` use MPI-3.0-only API and do not compile at a
2.2 target (`-DUNIMPI_BUILD_EXAMPLES=OFF`, and exclude those MPI tests).

[MS-MPI is MPI 2.2 compliant and implements a subset of MPI
3.1](https://github.com/microsoft/Microsoft-MPI#version-of-mpi-standard).
Tests therefore require the nonblocking calls in Microsoft's
[collective-function reference](https://learn.microsoft.com/en-us/message-passing-interface/mpi-collective-functions),
execute additional MPI-3 symbols when the installed DLL exports them, and
report an explicit skip otherwise. Outside the documented
integer-handle/datatype-array adapter gap, missing MPI-3 symbols fail the Open
MPI, MPICH, and Intel MPI jobs rather than being silently skipped.

## Sanitizers

The strict job builds C99 without compiler extensions and executes unit/fake
tests with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S . -B build-sanitized -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_C_EXTENSIONS=OFF \
  "-DCMAKE_C_FLAGS=-Werror -fsanitize=address,undefined -fno-omit-frame-pointer" \
  "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined" \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=OFF \
  -DUNIMPI_BUILD_BENCHMARKS=OFF
cmake --build build-sanitized --parallel
ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1 \
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-sanitized -L unit --output-on-failure
```

AppleClang does not support the Linux leak-sanitizer setting used in CI. For a
local macOS reproduction, use `ASAN_OPTIONS=detect_leaks=0`; do not weaken the
Linux CI setting.

Fake backend identity libraries are loaded and unloaded sequentially. Keeping
multiple MPICH-derived fixtures resident can produce genuine ASan ODR reports
because those libraries intentionally expose the same implementation globals.

## Test-writing rules

- Use `TEST_CHECK_SUCCESS` or an equivalent checked call; do not silently
  discard an MPI return value.
- Check the result, not only that a function pointer is non-null.
- Give temporary files unique per-test/per-rank names and remove them.
- Test success, invalid input, backend failure, and state cleanup where the API
  defines those outcomes.
- Keep assertions enabled in Release test targets.
- Set a finite timeout for MPI tests so a failed rank cannot hang CI forever.
- Treat a benchmark result as data, not a pass/fail performance threshold.

The current verification boundary is documented in
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).
