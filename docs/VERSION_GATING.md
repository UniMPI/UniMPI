# MPI Version-Gated Static Builds

UniMPI lets you choose an MPI **target version** at build time. Everything above
that target is *physically removed* from the compiled surface: vtable fields,
standard-name macros, and backend symbol bindings. The result is a smaller,
MPI-baseline-exact library whose layout and symbols only reference
`≤ target` MPI operations. This document explains the concept, the
configuration, the ABI consistency requirement, the gated clusters, and how to
audit the resulting artifact.

## Concept: compile-time baseline vs runtime capability

Two **independent** questions drive how UniMPI behaves:

| Question | Decided by | Effect |
|---|---|---|
| Which MPI surface does UniMPI **expose** (fields, macros, bindings)? | The **target version** chosen at build time | Compile-time: everything above the target is removed by `#if` |
| Which MPI operations actually **work at runtime**? | Which backend library is `dlopen`'d at runtime | Run-time: the backend (Open MPI, MPICH, Intel MPI, MS-MPI) decides real capability |

The target version **only narrows the exposed surface**. It does not (and cannot)
add runtime capability — that is entirely determined by the backend that gets
loaded. Raising the target simply re-exposes whatever `≤ target` surface was
already implemented; it never manufactures new behavior in a backend that lacks
it.

This design means a single UniMPI library source tree can be built once per
target:

- `target=3.1` (default): the full implemented MPI-3 surface, as before.
- `target=2.2`: an MPI-2.2-baseline build whose struct layout and symbols
  contain no MPI-3.0 field or binding at all (see
  [Auditing the resulting artifact](#auditing-the-resulting-artifact)).

## The `UNIMPI_MPI_AT_LEAST` macro

`include/unimpi_version.h` defines the target-version constants and the gate
macro:

```c
#ifndef UNIMPI_MPI_TARGET_VERSION
#define UNIMPI_MPI_TARGET_VERSION 3
#define UNIMPI_MPI_TARGET_SUBVERSION 1
#endif

#define UNIMPI_MPI_AT_LEAST(MJ, MN) \
    ((UNIMPI_MPI_TARGET_VERSION) > (MJ) || \
     ((UNIMPI_MPI_TARGET_VERSION) == (MJ) && \
      (UNIMPI_MPI_TARGET_SUBVERSION) >= (MN)))
```

`UNIMPI_MPI_AT_LEAST(maj,min)` is true when the chosen **target** version is
`>= maj.min`. Every gated element is wrapped in `#if UNIMPI_MPI_AT_LEAST(3,0)`.
At the default target (3.1) every gate is open; at target 2.2 every `AT_LEAST(3,0)`
gate is closed and its contents vanish from the compiled artifact.

### Do not confuse the target with the facade-capability constants

`include/unimpi.h` also declares `UNIMPI_MPI_VERSION` / `UNIMPI_MPI_SUBVERSION`
(currently `3`/`0`). These are **unrelated** to the target version:

- `UNIMPI_MPI_TARGET_VERSION` / `UNIMPI_MPI_TARGET_SUBVERSION` — the **compile-time
  baseline** you select; they drive `UNIMPI_MPI_AT_LEAST` and physically strip
  the surface.
- `UNIMPI_MPI_VERSION` / `UNIMPI_MPI_SUBVERSION` — a **facade capability** report
  (the max MPI version the library was written against, `3.0`), exposed for
  compatibility. Changing the target does **not** change these.

Lowering the target strips surface but keeps the facade constants at their
declared value, so consumers must not read the facade constants to derive the
target.

## Configuration

### The supported way: CMake

The top-level `CMakeLists.txt` exposes two cache variables, propagated `PUBLIC`
to every consumer:

```bash
cmake -B build22 . \
  -DUNIMPI_MPI_TARGET_VERSION=2 \
  -DUNIMPI_MPI_TARGET_SUBVERSION=2
```

Any target that links the `unimpi` target automatically inherits the same
compile definitions through `INTERFACE_COMPILE_DEFINITIONS`, so the library and
its consumers are always built with the same macros.

### Compiling directly (without CMake)

If you compile the sources and your application by hand, pass the macros on the
command line so the header sees them before `unimpi_version.h` defines the
defaults:

```bash
cc -DUNIMPI_MPI_TARGET_VERSION=2 -DUNIMPI_MPI_TARGET_SUBVERSION=2 -Iinclude ...
```

Both definitions must be given together; they are a matched pair.

## Single-source consistency requirement (CRITICAL)

The vtable struct layout — the **set and offsets of its fields** — depends on
the target version. A 2.2 build has a smaller struct (284 fields) than a 3.1
build (325 fields). If you compile the library at one target and a consumer at
another, the consumer will read the vtable at the wrong offsets and crash or
misbehave.

> **You must build the library and every consumer with the identical
> `UNIMPI_MPI_TARGET_VERSION` / `UNIMPI_MPI_TARGET_SUBVERSION` macros.**
>
> Do NOT link a target-2.2-built `libunimpi` against a target-3.1-built
> consumer (or vice-versa).

In CMake this is guaranteed automatically: `unimpi::unimpi` propagates the
definitions over `INTERFACE`, so any target that links it is compiled with the
same macros. When compiling by hand, you are responsible for keeping them in
sync. This is the fundamental ABI rationale behind the whole feature — the
gating is not cosmetic, it changes the struct.

> **Default (3.1) field offsets are not guaranteed stable across this feature.**
> To keep each gated cluster a single contiguous block, a few MPI-3.0 fields
> (mainly in `comm_3x`) were regrouped relative to the pre-gating layout, so
> the 3.1 struct is not bit-identical to the older gating-free header — the
> total field count and `sizeof` are unchanged, but some offsets moved. Always
> fully recompile the library and every consumer against the same header; do
> not hard-code or persist a field offset from before this feature. All
> bindings are by field name (`unimpi.<field>`), so this is transparent to
> application code under a fresh, consistent build.

## The gated clusters

Gating is organized in **6 clusters**, all introduced by MPI-3.0, applied
identically across `include/unimpi_vtable.h`, `include/unimpi_std_macros.h`, and
each backend adapter (`src/backends/openmpi.c`, `mpich.c`, `intelmpi.c`,
`msmpi.c`). The authoritative list is `tools/versioned_clusters.csv`; the check
tool derives it from `tools/api_versions.csv`.

| Cluster | Representative MPI-3.0 API (removed below target) |
|---|---|
| `matched_probe` | `MPI_Mprobe`, `MPI_Improbe`, `MPI_Mrecv`, `MPI_Imrecv` |
| `nonblocking_collectives` | `MPI_Ibarrier`, `MPI_Ibcast`, `MPI_Igather(v)`, `MPI_Iscatter(v)`, `MPI_Iallgather(v)`/`MPI_Ialltoall(v/w)`, `MPI_Ireduce`, `MPI_Iallreduce`, `MPI_Iscan`, `MPI_Iexscan`, `MPI_Ireduce_scatter*` |
| `comm_3x` | `MPI_Comm_dup_with_info`, `MPI_Comm_split_type`, `MPI_Comm_create_group`, `MPI_Comm_get_info`, `MPI_Comm_set_info` |
| `win_alloc_shared` | `MPI_Win_allocate_shared`, `MPI_Win_create_dynamic` |
| `rma_atomics` | `MPI_Get_accumulate`, `MPI_Fetch_and_op`, `MPI_Compare_and_swap`, `MPI_Rput`, `MPI_Rget`, `MPI_Raccumulate`, `MPI_Rget_accumulate` |
| `rma_sync_3x` | `MPI_Win_lock_all`, `MPI_Win_unlock_all`, `MPI_Win_flush`, `MPI_Win_flush_all`, `MPI_Win_flush_local`, `MPI_Win_sync` |

Everything else — the MPI-2 base (point-to-point, collectives, datatypes,
fence-based RMA, communicators/topology, dynamic-process management, MPI-2.2 I/O,
Info, etc.) — is **always present**, regardless of target.

At a 2.2 target every row above is built out of the binary; at 3.1 all of them
are present. There is no partial-gating support requested outside these 9
clusters in Phase 1.

## Known 2.2 whole-tree limitation

Because the *examples* and *MPI integration tests* are written against the
default (3.1) surface, **not every source file compiles at a 2.2 target**. A
2.2 full-tree build with `UNIMPI_BUILD_EXAMPLES=ON` and/or
`UNIMPI_BUILD_MPI_TESTS=ON` fails to compile these files (they call
MPI-3.0-only API, which is intentionally absent):

- `examples/nonblocking.c` — calls `MPI_Ibarrier` (a nonblocking collective in
  the `nonblocking_collectives` cluster), the only MPI-3 call in the file; its
  other calls (`MPI_Isend`/`MPI_Irecv`, `MPI_Test`/`MPI_Wait`, `MPI_Sendrecv`)
  are all MPI-2.
- `tests/mpi/test_collective_nonblocking.c` — uses the full MPI-3 nonblocking
  collective set (`MPI_Ibcast`, `MPI_Igather`, `MPI_Iallreduce`, …). Fails with
  `implicit declaration of function 'MPI_*'` and `'unimpi_vtable_t' has no member
  named 'i*'`.
- `tests/mpi/test_environment_info.c` — uses `MPI_Comm_dup_with_info`,
  `MPI_Comm_get_info`, `MPI_Comm_set_info` (the `comm_3x` cluster).

A **2.2 build is validated on the library and the unit-test executables**, not
on these example/integration files. For a clean full-tree 2.2 build:

```bash
cmake -B build22 . \
  -DUNIMPI_MPI_TARGET_VERSION=2 -DUNIMPI_MPI_TARGET_SUBVERSION=2 \
  -DUNIMPI_BUILD_EXAMPLES=OFF \
  -DUNIMPI_BUILD_MPI_TESTS=OFF
```

To keep building the real MPI tests, skip the two incompatible test objects
(see [Real-backend 2.2 integration](#real-backend-22-integration)).

## Auditing the resulting artifact

These checks confirm the gate actually removed the 3.0 surface (default 3.1
build vs 2.2 build):

### String scan of the static library

```bash
strings libunimpi.a | grep -cE 'MPI_Ibcast|MPI_Comm_create_group|MPI_Win_sync'
```

- Default (3.1): `12`
- Target 2.2: `0`

### Struct layout (vtable smoke test)

`tests/internal/test_vtable_layout.c` prints the size and field count. Built
and run at the target under test:

```bash
./build/tests/test_vtable_layout    # or build22
```

- Default (3.1): `VTABLE_SIZE=2600`, `VTABLE_COUNT=325`
- Target 2.2: `VTABLE_SIZE=2272`, `VTABLE_COUNT=284`

### Gate checker

`tools/mpi_version_gate.py` verifies every versioned entity is consistent and
guarded:

```bash
python3 tools/mpi_version_gate.py check --clusters tools/versioned_clusters.csv
python3 tools/mpi_version_gate.py check --clusters tools/versioned_clusters.csv --require-guards
```

Both should report `gate check passed (6 clusters, 41 entities)`. This runs in
the final whole-branch verification below.

`base` verifies the MPI-2.2 canonical function list (from
`docs/MPI_VERSION_EVOLUTION.md`) never overlaps the gated surface, and reports
always-present 2.2 coverage — it is the mechanical anti-regression guard that
would have caught the alltoallw/comm_join/op_commutative misclassification:

```bash
python3 tools/mpi_version_gate.py base
```

It hard-fails if any gated field is a 2.2 canonical function (a 2.2 baseline
would silently lose it), and prints how many of the ~305 2.2 functions are
exposed in the always-present vtable. It executes in the final whole-branch
verification below.

## Real-backend 2.2 integration

When a real MPI runtime is installed, build the library + a base MPI test at 2.2
and run it under a real backend to confirm the gate does not break MPI-2
operations:

```bash
cmake -B build22 . \
  -DUNIMPI_MPI_TARGET_VERSION=2 -DUNIMPI_MPI_TARGET_SUBVERSION=2 \
  -DUNIMPI_BUILD_MPI_TESTS=ON -DUNIMPI_BUILD_EXAMPLES=OFF
cmake --build build22 --target test_p2p   # MPI-2-only test
mpiexec -np 2 env UNIMPI_BACKEND=openmpi ./build22/tests/test_p2p
```

`test_p2p` uses only MPI-2 point-to-point (`Send`, `Isend`/`Irecv`, `Sendrecv_replace`,
`Probe`), so it compiles and runs at a 2.2 target, exercising the stripped
build against a live backend. The two MPI-3-only test objects
(`test_collective_nonblocking`, `test_environment_info`) are not part of a 2.2
build; expect them to fail to compile if requested.

> If no MPI runtime is present, skip this step and record `SKIP (no MPI
> runtime)`; the library + unit build still validates the gate.

## Monotonicity

The target is a **monotone baseline**: lowering it removes surface, and raising
it back only re-exposes the same `≤ target` surface. It never adds runtime
capability. Concretely:

- `target 3.1 → 2.2`: MPI-3.0 clusters disappear; the MPI-2 base is unchanged.
- `target 2.2 → 3.1`: the 6 clusters come back; the MPI-2 base is still the
  same.

There is no "one-way downgrade": rebuilding at a higher target restores the
full implemented surface. The runtime behavior in each build still depends only
on the dlopen'd backend.

## Phase 2 outlook

The same mechanism generalizes to MPI-4 / MPI-5. Future clusters (e.g. MPI-4
*sessions*, *partitioned communication*, *tool control interface*, and any
later additions surfaced in `MPI_VERSION_EVOLUTION.md`) would be guarded with
`UNIMPI_MPI_AT_LEAST(4,0)` / `UNIMPI_MPI_AT_LEAST(5,0)` and registered in
`tools/versioned_clusters.csv` plus `tools/api_versions.csv`. The target-version
machinery (`unimpi_version.h`, the `AT_LEAST` gate, CMake PUBLIC propagation,
the checker) already supports this; no MPI-4/5 surface is implemented in this
phase.

## Final whole-branch verification

```bash
python3 tools/mpi_version_gate.py check --clusters tools/versioned_clusters.csv
python3 tools/mpi_version_gate.py check --clusters tools/versioned_clusters.csv --require-guards
python3 tools/mpi_version_gate.py base
cmake -B build .
cmake --build build
ctest --output-on-failure
```

Both `check` invocations pass, `base` reports the gated surface disjoint from
the 2.2 canonical list, and the default build + test suite is green.
