# Support and Verification Matrix

This document separates three different claims:

1. an entry exists in the runtime dispatch table;
2. a backend exports a symbol that can populate that entry;
3. a test has exercised the operation with that backend.

These are not equivalent. A function-pointer slot is not, by itself, proof of
MPI conformance or runtime availability.

## Current API inventory

- `unimpi_vtable_t` contains 317 MPI function-pointer fields.
- `unimpi_std_macros.h` contains 290 direct standard-name aliases of the form
  `MPI_* -> unimpi.<field>`.
- Initialization, finalization, constants, and a small number of function-like
  convenience macros are defined separately.

The project does not claim the full MPI-3 or MPI-4 API. The header files are the
source of truth for what can be compiled, and the tables below state what is
currently exercised.

## Backend and platform matrix

| Backend | Linux | macOS | Windows | Recommended runtime selection |
|---|---:|---:|---:|---|
| Open MPI | Yes | Yes | No | Exact `libmpi.so*` or `libmpi.dylib` path |
| MPICH | Yes | Yes | No | Exact `libmpi.so*` or `libmpi.dylib` path |
| Intel MPI | Yes | No | No | Exact oneAPI `libmpi.so` path |
| Microsoft MPI | No | No | Yes | `C:\Windows\System32\msmpi.dll` |

The launcher must come from the same installation as the runtime library. For
example, do not combine an Open MPI `mpirun` with an MPICH `libmpi`.

### Backend selection priority

Selection is deterministic:

1. `UNIMPI_LIBRARY` — an exact library path or loader-resolvable library name;
2. `UNIMPI_BACKEND` — `openmpi`, `mpich`, `intelmpi`, or `msmpi`;
3. platform default — `msmpi.dll` on Windows and `libmpi.so.40` on POSIX.

The platform default is a fallback name, not a filesystem scan. In particular,
`libmpi.so.40` is not a normal macOS library name, and more than one `libmpi.so`
may be installed on Linux. Use `UNIMPI_LIBRARY` in CI, production, and whenever
multiple MPI installations are present.

Examples of precise paths:

```bash
# Ubuntu Open MPI; resolve the installed soname rather than copying this path
export UNIMPI_LIBRARY="$(readlink -f /usr/lib/x86_64-linux-gnu/libmpi.so)"

# Homebrew Open MPI
export UNIMPI_LIBRARY="$(brew --prefix open-mpi)/lib/libmpi.dylib"

# Homebrew MPICH
export UNIMPI_LIBRARY="$(brew --prefix mpich)/lib/libmpi.dylib"

# Intel oneAPI MPI
export UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so
```

```powershell
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"
```

## Verification levels

| Level | Meaning |
|---|---|
| Unit | No external MPI installation; fake shared libraries exercise loader, identity, lifecycle, failure, and vtable behavior |
| Real | CTest launches processes through the selected implementation and checks results |
| Smoke | The executable starts, exercises its documented path with reduced work, and exits successfully |
| Uncovered | A slot or macro may exist, but the repository has no focused semantic test for that category |

## Architecture and API verification

`Real` below means the category has focused tests in the backend CI job. It
does not mean every valid input, error class, datatype, topology, or MPI
standard rule is covered.

| Category | Unit/fake | Open MPI | MPICH | Intel MPI | MS-MPI | Current boundary |
|---|---:|---:|---:|---:|---:|---|
| Loader priority and diagnostics | Yes | Smoke | Smoke | Smoke | Smoke | Exact-path and failure behavior |
| Backend identification | Yes | Yes | Yes | Yes | Yes | Fake identity plus real initialization |
| Lifecycle and state queries | Yes | Yes | Yes | Yes | Yes | Normal init/finalize, failure and retry boundaries |
| Thread initialization | Yes | Yes | Yes | Yes | Yes | Negotiation only; concurrent MPI calls are not certified |
| Vtable required-field validation | Yes | Yes | Yes | Yes | Yes | Required profile, not every optional MPI symbol |
| Basic point-to-point | No | Yes | Yes | Yes | Yes | Blocking, nonblocking, probe and sendrecv subset |
| Request completion and persistent setup | No | Yes | Yes | Yes | Yes | Request-handle arrays and per-element status arrays are adapted in place for the native stride; payload completion, zero counts and `Startall` are exercised |
| Blocking collectives | No | Yes | Yes | Yes | Yes | Barrier, broadcast, gather/scatter, all-to-all and reductions subset |
| Nonblocking collectives | No | Yes | Yes | Yes | Partial | Open MPI exercises all 17 calls; MPICH and Intel exercise the full set including `Ialltoallw`, adapted in memory for the native 4-byte datatype-handle stride; MS-MPI requires its documented nine-call subset |
| Core datatypes and pack/unpack | No | Yes | Yes | Yes | Yes | Representative derived datatypes |
| Communicators and groups | No | Yes | Yes | Yes | Yes | Representative create/split/compare/group operations |
| Cartesian and graph topology | No | Yes | Yes | Yes | Yes | Focused MPI-2.2 topology cases |
| Intercommunicators | No | Yes | Yes | Yes | Yes | Even and odd process partitions |
| Environment, Info, memory and status | No | Yes | Yes | Yes | Partial | MPI-2.2 calls are required; MPI-3 communicator Info/split calls are required outside MS-MPI and exercised there when exported |
| RMA data path and window attributes | No | Yes | Yes | Yes | Yes | Fence-based Put/Get/Accumulate plus keyvals, attributes and group plumbing |
| MPI I/O extensions | No | Yes | Yes | Yes | Yes | Positioned independent/collective/nonblocking round trips, metadata, sync and atomicity |
| Examples | No | Smoke | Smoke | Smoke | Smoke | See `examples/README.md` for rank requirements |
| Benchmarks | No | Smoke | Smoke | Smoke | Smoke | Execution only; no hard performance threshold |

## Explicitly not claimed as covered

The dispatch table includes additional MPI entry points that do not yet have
focused, cross-backend semantic coverage. Important examples include:

- dynamic process management and name publishing;
- matched probes, ready/buffered sends, and cancellation semantics;
- persistent collective requests and broader persistent point-to-point
  lifecycle/error cases;
- remaining blocking and nonblocking collective variants and corner cases
  (in-place, zero-count and non-power-of-two behavior is not exhaustively
  certified on every backend);
- advanced datatype constructors and external data representation;
- RMA atomics, dynamic/shared windows, and epoch models other than fence;
- MPI I/O file views, shared/ordered pointers, seek operations, split
  collectives, and broader nonblocking cases;
- custom reduction operations, custom error handlers, and attribute corner
  cases;
- multithreaded concurrent initialization, finalization, and MPI calls;
- fault tolerance, GPU-aware behavior, and vendor-specific extensions.

Add or tighten a row only when a focused test demonstrates the behavior. See
[TESTING.md](TESTING.md) for the required test layers.
