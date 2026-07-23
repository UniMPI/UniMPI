# Contributing

Contributions should preserve UniMPI's runtime-loading architecture and all
supported backend/platform combinations.

## Set up

```bash
git clone https://github.com/YOUR_USERNAME/UniMPI.git
cd UniMPI
git remote add upstream https://github.com/UniMPI/UniMPI.git

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_EXTENSIONS=OFF \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=OFF
cmake --build build --parallel
ctest --test-dir build -L unit --output-on-failure
```

Use a real MPI build when changing communication, predefined values, status
layout, or backend adapters. See [TESTING.md](TESTING.md).

## Code style

- C99, four-space indentation, K&R braces.
- Keep compiler warnings clean; supported GCC/Clang builds use `-Werror`.
- Use fixed-width or pointer-sized types where ABI width matters.
- Check allocations, arguments, loader results, and MPI return values.
- Do not hard-code one vendor's communicator, datatype, operation, or status
  representation into common code.
- Keep platform-specific behavior in the platform or backend adapter.

## Tests

Every behavior change needs the narrowest useful layers:

- fake/unit tests under `tests/internal/` for loader, lifecycle, failure,
  cleanup, and vtable behavior;
- real tests under `tests/mpi/` for communication and backend ABI semantics;
- generated fixture symbols under `tests/fixtures/`;
- example smoke coverage when changing a documented workflow;
- benchmark smoke coverage when changing benchmark execution.

Register tests in `tests/CMakeLists.txt` with:

- `unit`/`fake` or `integration`/`mpi` labels;
- the minimum process count that executes all assertions;
- odd process counts for partitioning logic;
- finite timeouts;
- isolated temporary files and cleanup.

Run:

```bash
cmake --build build --parallel
ctest --test-dir build -L unit --output-on-failure
```

For a real backend:

```bash
UNIMPI_LIBRARY=/absolute/mpi/lib/libmpi.so \
  ctest --test-dir build-mpi -L integration \
  --output-on-failure --timeout 180
```

Do not remove backend coverage, suppress sanitizers, or weaken assertions to
make a test pass. Model genuine implementation differences explicitly.

## Backend changes

When adding or changing a vtable operation:

1. update the typed field in `include/unimpi_vtable.h`;
2. update the standard alias when intended;
3. resolve it in every applicable backend adapter;
4. decide whether it is required or optional;
5. extend fake required/missing-symbol coverage;
6. add focused real-backend semantics;
7. update [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) only after verification.

When adding a backend:

1. add its backend type and loader identification;
2. implement platform support rules;
3. initialize predefined handles/constants and the required vtable profile;
4. add fake identity fixtures;
5. add a real CI job on every supported platform;
6. document an exact launcher/library tuple.

## Documentation

Update the maintained documents for user-visible changes:

- `README.md` for entry-point workflows;
- `docs/BUILDING.md` for options and dependencies;
- `docs/BACKENDS.md` for runtime selection;
- `docs/TESTING.md` and `docs/SUPPORT_MATRIX.md` for verification;
- `docs/BENCHMARKS.md` for benchmark CLI or methodology;
- `examples/README.md` for example commands.

Do not use historical plans under `docs/superpowers/plans/` as current support
claims.

## Pull requests

Describe:

- what changed and why;
- affected backend/platform combinations;
- exact configure/build/test commands;
- test results, including skipped or unavailable environments;
- API or documentation compatibility considerations.

Use focused commits and conventional prefixes such as `feat:`, `fix:`,
`test:`, `docs:`, `perf:`, and `build:`.

Before submitting:

```bash
git diff --check
cmake --build build --parallel
ctest --test-dir build -L unit --output-on-failure
```

Report real-backend validation separately rather than implying it from a
fake-only test run.
