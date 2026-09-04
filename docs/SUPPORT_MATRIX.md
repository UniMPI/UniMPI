# Support and Verification Matrix

This document separates three different claims:

1. an entry exists in the runtime dispatch table;
2. a backend exports a symbol that can populate that entry;
3. a test has exercised the operation with that backend.

These are not equivalent. A function-pointer slot is not, by itself, proof of
MPI conformance or runtime availability.

## Current API inventory

- `unimpi_vtable_t` contains 338 MPI function-pointer fields.
- `unimpi_std_macros.h` contains 311 direct standard-name aliases of the form
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
| Neighbor collectives | No | Yes | Yes | Yes | Yes | MPI-3.0 dist-graph ring round trip: `Neighbor_allgather`, the neighbor `alltoall(v/w)` family and `Comm_idup`; datatype arrays adapted for the MPICH-family 4-byte stride |
| Core datatypes and pack/unpack | No | Yes | Yes | Yes | Yes | Representative derived datatypes |
| Large-count (_x) types | No | Yes | Yes | Yes | Yes | MPI-3.0 `Type_size_x` / `get_extent_x` / `get_true_extent_x`, `Get_elements_x` / `Status_set_elements_x`, `Type_create_hindexed_block` (64-bit `MPI_Count`) |
| Communicators and groups | No | Yes | Yes | Yes | Yes | Representative create/split/compare/group operations |
| Cartesian and graph topology | No | Yes | Yes | Yes | Yes | Focused MPI-2.2 topology cases |
| Intercommunicators | No | Yes | Yes | Yes | Yes | Even and odd process partitions |
| Environment, Info, memory and status | No | Yes | Yes | Yes | Partial | MPI-2.2 calls are required; MPI-3 communicator Info/split calls are required outside MS-MPI and exercised there when exported |
| RMA data path and window attributes | No | Yes | Yes | Yes | Yes | Fence-based Put/Get/Accumulate plus keyvals, attributes and group plumbing |
| MPI I/O extensions | No | Yes | Yes | Yes | Yes | Positioned independent/collective/nonblocking round trips, metadata, sync and atomicity |
| Examples | No | Smoke | Smoke | Smoke | Smoke | See `examples/README.md` for rank requirements |
| Benchmarks | No | Smoke | Smoke | Smoke | Smoke | Execution only; no hard performance threshold |
| MPI-T tools interface | No | Yes | Partial | Partial | Partial | Lifecycle, enum self-check, no-object pvar round trip, survival across `MPI_Finalize`; MPICH-family bind the standard 9/8-arg `get_info` signatures, OpenMPI is bridged from its legacy 13/10-arg signatures, MS-MPI degrades to NULL via `*_available()` |

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
- RMA atomics, shared windows, and epoch models other than fence (dynamic
  windows — `Win_create_dynamic`/`Win_attach`/`Win_detach` — are covered by
  the MPI-3.0 core tests);
- MPI I/O file views, shared/ordered pointers, seek operations, split
  collectives, and broader nonblocking cases;
- custom reduction operations, custom error handlers, and attribute corner
  cases;
- multithreaded concurrent initialization, finalization, and MPI calls;
- fault tolerance, GPU-aware behavior, and vendor-specific extensions;
- the Fortran-2008 status binding (`MPI_Status_f2f08`/`c2f08`/`f082f`/`f082c`):
  F08-binding-only entry points that export no C symbol to bind.

Add or tighten a row only when a focused test demonstrates the behavior. See
[TESTING.md](TESTING.md) for the required test layers.

## Missing-symbol degradation (MS-MPI)

MS-MPI is a Microsoft port of an older MPICH lineage, so it does not export
every MPI-3.0 symbol that UniMPI's dispatch table can hold. The degradation
path is uniform and must be understood when writing MS-MPI-portable code:

1. **Landing on NULL is implicit.** Each backend binding assigns a vtable slot
   directly from `unimpi_platform_dlsym`. When `msmpi.dll` lacks the symbol the
   slot is simply left as `NULL` — there is no per-symbol stub and no global
   failure.
2. **The standard macros do not intercept a NULL slot.** `MPI_Mprobe`,
   `MPI_Iallreduce`, `MPI_Fetch_and_op`, etc. expand to `unimpi.<field>`, so
   calling one that is `NULL` is calling through a null pointer — a crash, not
   an error return. There is deliberately no runtime branch in the macro layer
   (zero-overhead design).
3. **The caller (or the test) degrades by checking the slot first:**

   ```c
   if (unimpi.fetch_and_op != NULL) {        /* or a helper like *_available() */
       /* MPI_Fetch_and_op is safe to call */
   }
   ```

   The MPI-3.0 test suites follow this pattern via small availability helpers
   and degrade to a documented skip when a slot is absent: nonblocking
   collectives (`nbc_available`, `test_mpi30_collective_corner.c`), matched
   probe (`matched_probe_available`, `test_mpi30_matched_probe.c`), and RMA
   atomics/requests (`atomics_available` / `request_rma_available`,
   `test_mpi30_rma_atomic.c`).

Static audit status (per MPI-3.0 canonical cluster from `tools/mpi_version_gate.py`
`M30_CANONICAL`): every cluster is bound through `dlsym` in `src/backends/msmpi.c`,
so any that MS-MPI lacks degrades to a NULL slot covered by the pattern above.
The **exact** per-symbol export set of a given `msmpi.dll` cannot be established
on a Linux host; it is a static audit plus this documented degradation contract,
to be confirmed by running the MS-MPI test suite on Windows
(`test_msmpi_compatibility.bat`). One narrow subset is already documented in the
backend matrix above — nonblocking collectives on MS-MPI export only the
"documented nine-call subset" — which is why the NBC corner tests must gate on
`nbc_available()`.

### MPI-T tools interface lifecycle

MPI-T is exposed through its own vtable — the global `unimpi_mt` — separate
from the main `unimpi` dispatch table. Both are populated by the same backend
initialization for the same library handle, and the tool interface keeps the
backend library open with its own reference count. The practical consequence
is that `MPI_T_*` calls stay usable **before `MPI_Init` and after
`MPI_Finalize`** — the tool interface is intended for out-of-band inspection.
Every `MPI_T_*` wrapper forces `unimpi_ensure_loaded()` before forwarding, so
the interface is available on first use; `MPI_T_init_thread`/`MPI_T_finalize`
acquire/release the tool-interface reference.

Availability is self-gating: the `mpit_available()` helper in
`tests/mpi/test_mpi30_t.c` checks the `t_init_thread`/`t_finalize`/
`t_cvar_get_num`/`t_pvar_get_num` slots and skips the suite when a backend does
not export MPI-T at all (MS-MPI). Backend-fill, not hardcoding, sets the
MPI-T error codes and enumeration constants; a backend that exports no
MPI-T leaves the slots `NULL` and the suite skips rather than failing.

### MPI_Status field access

`MPI_Status` is a 24-byte union whose member layout matches the active
backend: OpenMPI fields at offset 0, legacy (MPICH / Intel-MPI / MS-MPI)
fields at offset 8. The MPI C standard (through MPI-3.1) defines no accessor
for `MPI_SOURCE` / `MPI_TAG` / `MPI_ERROR` (only count, via `MPI_Get_count`),
so read these three through UniMPI's accessor function pointers — which use
the standard MPI-4.0/5.0 names and signatures and are bound by each backend to
its own layout:

```c
int src, tag, err, n;
MPI_Status_get_source(&status, &src);   /* maps to unimpi.status_get_source */
MPI_Status_get_tag(&status, &tag);      /* maps to unimpi.status_get_tag */
MPI_Status_get_error(&status, &err);    /* maps to unimpi.status_get_error */
n = -1;  MPI_Get_count(&status, MPI_INT, &n);   /* standard count accessor */
```

Direct `status.MPI_SOURCE`-style field access is **not layout-portable**
across UniMPI backends and should be avoided in portable code.

These accessors are the standard MPI-4.0 functions `MPI_Status_get_source` /
`MPI_Status_get_tag` / `MPI_Status_get_error` (carried forward into
MPI-5.0); UniMPI also exposes them on its MPI-2.2/3.x surface.
