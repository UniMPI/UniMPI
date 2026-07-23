# Benchmarks

UniMPI ships three small MPI benchmarks. They are measurement tools, not
conformance tests and not CI performance gates.

| Executable | Purpose | Recommended ranks |
|---|---|---:|
| `bench_latency` | Point-to-point ping-pong latency and throughput across message sizes | 2 |
| `bench_collective` | Broadcast, all-reduce, gather, and barrier timing | 4 or more |
| `bench_overhead` | Paired direct-symbol and UniMPI-vtable `MPI_Comm_rank` dispatch timing | 1 or more |

The current programs measure end-to-end MPI operations. They do not isolate a
single indirect call from the backend's own work, so results must not be
described as a proven nanosecond-level wrapper overhead.

## Build

Benchmarks are disabled by default:

```bash
cmake -S . -B build-bench -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUNIMPI_BUILD_TESTS=ON \
  -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DUNIMPI_BUILD_BENCHMARKS=ON \
  -DMPIEXEC_EXECUTABLE=/path/to/matching/mpirun
cmake --build build-bench --parallel
```

Select the exact runtime library before running:

```bash
export UNIMPI_LIBRARY=/absolute/path/to/the/same/mpi/lib/libmpi.so
```

On macOS, use `libmpi.dylib`. On Windows, use
`$env:WINDIR\System32\msmpi.dll` and the MS-MPI launcher from
`$env:ProgramFiles\Microsoft MPI\Bin`.

## Common command-line options

All three benchmarks accept:

```text
--smoke
--warmup N
--iterations N
--format text|csv
--help
```

- `--smoke` selects a small workload suitable for CI. If warmup or iteration
  counts are explicitly supplied, those explicit values take precedence.
- `--warmup N` sets unmeasured warmup iterations.
- `--iterations N` sets measured iterations.
- `--format text` is intended for people.
- `--format csv` is intended for scripts and archival comparison.

`bench_overhead` also accepts `--batches N` to repeat the measured batch.

## Smoke runs

```bash
mpirun -np 2 ./build-bench/benchmarks/bench_latency \
  --smoke --format text

mpirun -np 4 ./build-bench/benchmarks/bench_collective \
  --smoke --format text

mpirun -np 1 ./build-bench/benchmarks/bench_overhead \
  --smoke --format text
```

CTest registers only the smoke form:

```bash
ctest --test-dir build-bench -L benchmark --output-on-failure
```

A smoke pass means the benchmark executed and produced valid output. It is not
a performance assertion.

## Measurement runs

Use a Release build and choose iteration counts long enough to dominate startup
and timer noise:

```bash
mpirun -np 2 ./build-bench/benchmarks/bench_latency \
  --warmup 200 --iterations 5000 --format csv \
  > latency-openmpi.csv

mpirun -np 4 ./build-bench/benchmarks/bench_collective \
  --warmup 100 --iterations 1000 --format csv \
  > collective-openmpi.csv

mpirun -np 1 ./build-bench/benchmarks/bench_overhead \
  --warmup 100 --iterations 10000 --batches 10 --format csv \
  > overhead-openmpi.csv
```

Run each backend in a fresh process with a matching launcher/library pair.
Record at least:

- commit SHA and whether the worktree is clean;
- OS, CPU, compiler, build type, and power policy;
- MPI implementation and version;
- launcher command, process placement, and process count;
- `UNIMPI_LIBRARY` path;
- benchmark options and message sizes;
- enough repeated samples to report variation, not only the fastest result.

## Interpretation

- Compare like with like: same hardware, process placement, MPI version,
  message sizes, and iteration counts.
- Separate launcher/startup time from steady-state operation time.
- Use warmups to populate backend caches and establish connections.
- Report median and a spread statistic such as p95 or standard deviation.
- Investigate outliers; do not delete them without documenting why.
- Avoid hard CI thresholds. Shared CI runners, CPU frequency changes, and
  network placement make small regressions noisy.
- A direct native-MPI baseline must be built and run under the same conditions
  before claiming wrapper overhead.

CSV output is the stable automation format. Text output may evolve for
readability.
