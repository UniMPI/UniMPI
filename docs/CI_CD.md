# Continuous Integration

The primary maintained CI definition is the
[GitHub Actions workflow](https://github.com/UniMPI/UniMPI/blob/dev/.github/workflows/ci.yml).
The [GitLab configuration](https://github.com/UniMPI/UniMPI/blob/dev/.gitlab-ci.yml)
mirrors Linux Open MPI/MPICH and benchmark flows, but it is not the
cross-platform release matrix described below.

## Matrix

| Job configuration | Runner | Runtime | Main purpose |
|---|---|---|---|
| Linux Open MPI Debug | Ubuntu 24.04 | Distribution Open MPI | Debug build and real MPI tests |
| Linux MPICH Release | Ubuntu 22.04 | Distribution MPICH | Release and MPICH ABI tests |
| Linux Intel MPI Release | Ubuntu 24.04 | oneAPI MPI | Intel backend tests |
| macOS Open MPI Debug | macOS 14 | Homebrew Open MPI | AppleClang/Open MPI tests |
| macOS MPICH Release | macOS 14 | Homebrew MPICH | AppleClang/MPICH tests |
| Windows MS-MPI Release | Windows Server 2022 | Official MS-MPI runtime | MSVC x64 and Windows loader tests |
| Strict C99 sanitizers | Ubuntu 24.04 | Generated fake backends | Clang, ASan, UBSan, no C extensions |

`fail-fast` is disabled for backend matrices so one implementation does not
hide results from another.

The Linux MPICH job intentionally remains on the supported Ubuntu 22.04 runner.
Ubuntu 24.04's distribution MPICH/Hydra PMI-PMIx packaging can launch the
requested processes as independent size-one MPI worlds, which would make
multi-rank coverage invalid. Open MPI and Intel MPI continue to use Ubuntu
24.04. See [Ubuntu bug 2072338](https://bugs.launchpad.net/bugs/2072338) for
the distribution packaging issue.

## Runtime tuple

Each real-MPI job resolves:

- the launcher executable;
- the native runtime library;
- the runtime library directory where needed;
- launcher preflags such as Open MPI oversubscription or Intel bootstrap.

It then configures `MPIEXEC_EXECUTABLE`, exports the exact
`UNIMPI_LIBRARY`, and runs CTest. A successful configuration with a mismatched
launcher/library pair is not accepted.

Important platform facts:

- Linux uses the resolved `libmpi.so` and propagates its directory through
  `LD_LIBRARY_PATH` when required.
- macOS uses the exact Homebrew `libmpi.dylib`; the POSIX fallback
  `libmpi.so.40` is not used.
- Intel MPI sources the oneAPI environment and uses the runtime from the same
  prefix.
- Windows launches with
  `C:\Program Files\Microsoft MPI\Bin\mpiexec.exe` and loads
  `C:\Windows\System32\msmpi.dll`.

## Test stages

Real backend jobs enable examples and benchmarks, then run the complete CTest
registry so unit/fake checks and all real-MPI, example, and benchmark smoke
invocations execute:

```bash
ctest --test-dir build --output-on-failure --timeout 180
```

The `integration`, `example`, and `benchmark` labels remain available for
focused local reruns. Benchmark smoke tests verify execution only; CI does not
compare timing against a hard threshold.

The strict job runs generated fake libraries under ASan/UBSan:

```bash
ctest --test-dir build-sanitized -L unit \
  --output-on-failure --timeout 120
```

Fake backend DSOs with overlapping implementation globals must be loaded and
unloaded sequentially. Sanitizer checks should not be disabled to hide an ODR
violation.

## Adding a test

1. Register it in `tests/CMakeLists.txt`.
2. Assign `unit`/`fake` or `integration`/`mpi` labels.
3. Choose a process count that executes its assertions.
4. Add an odd process count if communicator partitioning is involved.
5. Set an explicit timeout and isolate temporary files.
6. Ensure the test runs in every applicable backend job.
7. Update [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) only after focused behavior is
   exercised.

See [TESTING.md](TESTING.md) for the process-count and sanitizer policy.

## Dependency and action hygiene

- Pin third-party actions to immutable commit SHAs.
- Use official package repositories and verify downloaded installers.
- Keep workflow permissions at `contents: read` unless a job demonstrably needs
  more.
- Avoid weakening warnings, sanitizers, or backend coverage to make one runner
  pass.
- Preserve Debug and Release coverage because assertion and optimization
  behavior differ.

## What CI does not prove

A green matrix does not establish complete MPI conformance, fault tolerance,
GPU awareness, production-scale performance, or every entry in the 276-field
vtable. Explicit gaps are listed in [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).
