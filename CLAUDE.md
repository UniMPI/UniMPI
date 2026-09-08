# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**unimpi** is a Universal MPI wrapper library that provides runtime backend loading for OpenMPI, MPICH, Intel-MPI, and MS-MPI. It allows writing MPI code once and running it with any supported MPI implementation without recompiling.

An application links to UniMPI once and picks the MPI implementation at runtime via `dlopen`/`dlsym` (or the Windows loader). UniMPI is a C99 library and does not need MPI headers at build time.

**Key characteristics:**
- Runtime backend detection and loading (one load at init), zero steady-state symbol lookup
- Dispatch is a macro layer over one global vtable: direct `unimpi.<field>` calls or standard `MPI_*` names under `UNIMPI_USE_STD_NAMES`
- No MPI header dependency, no ABI_binding to a specific vendor; handle values are `intptr_t`
- Dual API style: function-pointer (`unimpi.send`) and standard MPI macros (`MPI_Send`)
- **Version-gated surface**: MPI-3.0 / MPI-3.1 clusters appear only when built for that target (default is MPI-2.2)

## Architecture

### Core Components

```
include/unimpi.h            # Public API header (control wrappers + MPI_T std-name layer)
include/unimpi_vtable.h     # Main vtable struct, ~370 function-pointer fields, version-gated
include/unimpi_mt.h         # MPI-T tools vtable (unimpi_mt, separate from unimpi)
include/unimpi_std_macros.h # Standard MPI_* naming macros (direct vtable aliases)
include/unimpi_platform.h   # Platform abstraction (dlopen, dlclose, dlsym)
include/unimpi_loader.h     # Backend detection and loading
include/unimpi_version.h.in # CMake-generated; defines UNIMPI_MPI_TARGET_VERSION/SUBVERSION and UNIMPI_MPI_AT_LEAST

src/core.c                  # Initialization, finalization, lifecycle, UNIMPI_ERR_* handling
src/mpit.c                  # MPI-T wrapper layer (unimpi_mt_* forwarding)
src/mt_globals.c            # MPI-T globals
src/vtable.c                # Vtable init, per-backend dispatch
src/mp_constants.c          # Backend-specific communicator/Op constant values
src/loader.c                # Backend auto-detection, library loading
src/platform_posix.c        # POSIX implementation (dlfcn.h)
src/platform_windows.c      # Windows implementation (LoadLibrary)

src/backends/
  openmpi.c / mpich.c / intelmpi.c / msmpi.c + *_wrappers.c   # Per-backend init, dlsym binding, bridges
```

### Two vtables

- **Main dispatch table** — `unimpi` (global `unimpi_vtable_t`). Holds every MPI function pointer, populated once at init by the active backend. Fields are version-gated clusters: MPI-3.0 entities (`matched_probe`, `nonblocking_collectives`, `neighbor_collectives`, `comm_3x`, `win_alloc_shared`, `rma_atomics`, `rma_sync_3x`, `large_count`, `win_dynamic`, `mpi_t_tools`) are compiled only for target >= 3.0; the MPI-3.1 additions (`aint_add_diff`, `nonblocking_io_all`, `comm_idup`, `mpi_t_get_index`) only for target >= 3.1.
- **MPI-T tools vtable** — `unimpi_mt` (global `unimpi_mt_t`). Independent of `unimpi`, carries the `t_*` slots for `MPI_T_*`. It **stays usable after `MPI_Finalize`** via a reference-counted backend load (see `docs/API.md` MPI-T section). `unimpi_ensure_loaded()` is the force-load hook both vtables use.

### Initialization Flow

1. **Environment Check** (`src/loader.c:unimpi_loader_detect_backend`): `UNIMPI_BACKEND`, then `UNIMPI_LIBRARY`, then platform default.
2. **Library Loading** (`src/loader.c:unimpi_loader_load`): rejects standard-MPI-ABI libraries; platform `dlopen`/`LoadLibrary`.
3. **Backend Identification** (`src/loader.c:unimpi_loader_identify_backend`): OpenMPI via `ompi_mpi_comm_world`, Intel-MPI via `__I_MPI___cpu_core_type`, MPICH via `MPIR_Err_create_code`/`MPIR_Dup_fn`, MS-MPI via `MSMPI_Get_version` (Windows only).
4. **Vtable Population** (`src/vtable.c:unimpi_vtable_init`): validates core symbols (`MPI_Init`, `MPI_Comm_size`, `MPI_Comm_rank`), dispatches to the backend init, which assigns each slot via `unimpi_platform_dlsym`.

**Backend detection priority**: `UNIMPI_BACKEND` → `UNIMPI_LIBRARY` → auto (OpenMPI → Intel-MPI → MPICH → MS-MPI on Windows).

### Backend-specific ABI values

Handle and communicator *values* differ by backend and are resolved at runtime, never hardcoded in application code:

- OpenMPI: pointers (`ompi_mpi_comm_world`)
- MPICH/Intel-MPI: small ints (`MPI_COMM_WORLD = 91`)
- MS-MPI: magic values (`0x44000000` WORLD, `0x44000001` SELF)

`MPI_Status` is a 24-byte union whose member layout matches the active backend (OpenMPI fields at offset 0; legacy MPICH/Intel-MPI/MS-MPI at offset 8). **Never read `status.MPI_SOURCE` / `.MPI_TAG` / `.MPI_ERROR` directly** — use UniMPI's layout-aware accessors `MPI_Status_get_source/_tag/_error` (`unimpi.status_get_source` &c). These are bound per backend; see `docs/API.md`.

### Missing-symbol degradation

Backend binding assigns a slot directly from `dlsym`; a symbol the backend lacks simply leaves the slot `NULL` (no stub, no global failure). The macro layer does **not** intercept a NULL slot — calling one is a null-pointer crash by design (zero-overhead). Degrading backends (notably MS-MPI) must be gated by the **caller**: check the slot (or a `*_available()` helper) before calling; test suites follow the `mpit_available()` / `nbc_available()` pattern. See `docs/SUPPORT_MATRIX.md` "Missing-symbol degradation (MS-MPI)".

## The version-gating system (critical to understand before editing)

MPI features are physically compiled in/out by target version. **Always keep the three surfaces in lockstep**:

1. `include/unimpi_vtable.h` — the vtable fields (the `VERSION_GATING` principle: exposing a field or macro is not a claim of runtime behavior; the table is the compile-time surface).
2. `include/unimpi_std_macros.h` — the standard `MPI_*` aliases (same clusters, same gates).
3. The four `src/backends/*.c` — the dlsym bindings (same gates).

- Each gate block is one cluster guarded with `#if UNIMPI_MPI_AT_LEAST(maj,min)` ... `#endif`, anchored by an in-block header comment `/* MPI-maj.min <cluster> */`.
- `UNIMPI_MPI_TARGET_VERSION` / `UNIMPI_MPI_TARGET_SUBVERSION` (CMake cache vars) and the `UNIMPI_MPI_AT_LEAST` macro come from the **CMake-generated** `include/unimpi_version.h` (source `.in`).
- `tools/versioned_clusters.csv` is the audit registry (file, cluster, version). `tools/mpi_version_gate.py` is the validator:
  - `python3 tools/mpi_version_gate.py check` — data consistency;
  - `python3 tools/mpi_version_gate.py check --require-guards` — every cluster has a real `#if UNIMPI_MPI_AT_LEAST` guard (this is what catches a "gate drift" — a cluster whose edition changed but the guard didn't);
  - `python3 tools/mpi_version_gate.py base` — the always-present MPI-2.2 baseline (305/305 entities).
  - `tools/count_surface.py` — reports current vtable field / alias counts (370 fields / 367 aliases at the 3.1 target).
- **Always run `check --require-guards` after editing any gated file**, and re-verify a fresh target build (see Build below). A "gate pass" counts 14 clusters / 105 entities.

> **Editing rule**: when you touch a gated region, keep the #if/#endif pairing and the in-block cluster header comment. The pairing must stay mechanical — if you introduce a stray `#if`, `--require-guards` will fail.

## Build

### Quick Build (Linux/macOS)

```bash
cmake -B build .
cmake --build build
```

The **default target is MPI-2.2** (`UNIMPI_MPI_TARGET_VERSION=2`). To build the MPI-3.0 or MPI-3.1 surface:

```bash
cmake -B build30 . -DUNIMPI_MPI_TARGET_VERSION=3 -DUNIMPI_MPI_TARGET_SUBVERSION=0
cmake --build build30
```

`UNIMPI_VTABLE_COUNT` (in `unimpi_vtable.h`) is the runtime `sizeof(vtable)/8`; it changes per target **and** per build. Do not hardcode the count in tests.

### Build Options

```bash
cmake -B build . -DUNIMPI_BUILD_EXAMPLES=OFF -DUNIMPI_BUILD_TESTS=OFF   # minimal
cmake -B build . -DCMAKE_BUILD_TYPE=Debug
cmake -B build . -DUNIMPI_ENABLE_STD_MACROS=ON    # enable standard MPI_* names by default
cmake -B build . -DUNIMPI_BUILD_MPI_TESTS=ON      # real MPI tests (needs mpirun/mpiexec)
```

### Windows

- MS-MPI (MPICH-derived) is the only supported Windows backend; `msmpi.dll` lives in `C:\Windows\System32`.
- MinGW: `cmake -B build -G "MinGW Makefiles"; cmake --build build`
- Visual Studio: `cmake -B build -G "Visual Studio 17 2022" -A x64; cmake --build build --config Release`
- No `dlopen` — uses `LoadLibrary`/`GetProcAddress`.

## Test Commands

### All fake/unit tests (no MPI runtime needed)

```bash
cmake -S . -B build-unit -DUNIMPI_BUILD_TESTS=ON -DUNIMPI_BUILD_MPI_TESTS=OFF
cmake --build build-unit --parallel
ctest --test-dir build-unit -L unit --output-on-failure
```

### Real MPI tests (requires mpirun and `UNIMPI_LIBRARY` pointing at a matching library)

```bash
cmake -S . -B build-mpi -DUNIMPI_BUILD_TESTS=ON -DUNIMPI_BUILD_MPI_TESTS=ON \
  -DMPIEXEC_EXECUTABLE=/path/to/mpirun
cmake --build build-mpi --parallel
UNIMPI_LIBRARY=/absolute/path/to/libmpi.so \
  ctest --test-dir build-mpi -L integration --output-on-failure --timeout 180
```

### Single test

```bash
ctest --test-dir build -R test_loader          # by name
./build/tests/test_vtable_layout               # direct binary run
mpirun -np 2 ./build/tests/test_p2p            # MPI test with N ranks
```

### Backend selection

```bash
UNIMPI_BACKEND=openmpi mpirun -np 4 ./build/examples/minimal
UNIMPI_LIBRARY=/usr/lib/x86_64-linux-gnu/libmpi.so mpirun -np 4 ./build/examples/minimal
```

The launcher must come from the same MPI installation as the library.

## Docs as contract

The `docs/` set is the maintained contract — never write a claim there that a test does not demonstrate, and update it when behavior changes:

- `docs/SUPPORT_MATRIX.md` — separates "slot exists" / "backend exports symbol" / "test passed" and states the verification boundary per category. **Add or tighten a matrix row only after a focused test demonstrates the behavior.**
- `docs/API.md` — public control API + recommended usage; documents the MPI-T vtable and the layout-aware status accessors.
- `docs/VERSION_GATING.md` — the target-slice mechanism.
- `docs/TESTING.md` — test labels, process-count requirements, backend matrix.

## Coding Standards

- **Indentation**: 4 spaces (no tabs); **Braces**: K&R; **Line length**: max 100.
- **Naming**: functions `snake_case`; macros `UPPER_CASE`; types `snake_case_t`; global vtable `unimpi`.
- **Applications use native MPI style** (`MPI_*`) — `UNIMPI_*` prefixes are for internal implementation only; the public API presents as standard MPI.
- **Commit messages**: English conventional commits (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `perf:`, `chore:`). This is a hard repo rule for all contributors.
- **Git**: avoid push to `internal` unless asked; work is staged/committed locally.

## Adding an MPI function (checklist)

1. Add the vtable field in `include/unimpi_vtable.h` under the correct version-gated cluster, guarded by `UNIMPI_MPI_AT_LEAST(maj,min)`, with the in-block `/* MPI-maj.min <cluster> */` anchor comment.
2. Add the standard alias in `include/unimpi_std_macros.h` in the same gate.
3. Register the cluster (or an existing one, if adding to it) in `tools/versioned_clusters.csv` under the correct file→cluster→version.
4. Bind it in every backend that exports it (`src/backends/<backend>.c` + `*_wrappers.c` if a bridge is needed); leave adjacent slots `NULL` where a backend lacks it.
5. Run the gate validator (`check`, `check --require-guards`) and `count_surface.py`.
6. Add fake/unit coverage and focused real-backend tests; update `docs/SUPPORT_MATRIX.md` only after they pass.
