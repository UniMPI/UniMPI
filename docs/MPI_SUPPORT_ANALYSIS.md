# MPI API Inventory and Gaps

This document describes the current implementation inventory. It deliberately
does not calculate a percentage against "all MPI functions": function counts
vary by standard revision and language binding, and an exported pointer is not
proof of correct behavior.

The maintained verification contract is
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).

## Inventory

- 276 MPI function-pointer fields in `unimpi_vtable_t`.
- 247 direct standard-name aliases in `unimpi_std_macros.h`.
- Separate control wrappers for initialization, finalization, state queries,
  backend identity, diagnostics, and UniMPI errors.
- Four backend adapters: Open MPI, MPICH, Intel MPI, and MS-MPI.

Backend adapters attempt to resolve a broad MPI-2.2/MPI-3-era surface. A symbol
may be absent in a particular vendor/version, so required-field validation and
focused integration tests determine the supported runtime subset.

## Focused real-backend coverage

The current test suite has focused cases for:

- initialization, finalization, state, thread-init negotiation, and backend
  identity;
- basic blocking/nonblocking point-to-point operations;
- request-handle wait/test arrays, zero-count request arrays, sendrecv, and a
  representative persistent `Startall` path;
- basic, variable-count, reduction, and representative MPI-3 nonblocking
  collectives;
- communicators, groups, intercommunicators, and selected topologies;
- representative derived datatypes and pack/unpack;
- MPI-2.2 resized and envelope datatype operations;
- environment/thread queries, selected Info operations (including
  `MPI_Info_create` / `MPI_Info_free` width adaptation on integer-handle
  backends), object names, memory allocation, custom operations, and status
  helpers;
- fence-based RMA `Put`, `Get`, and `Accumulate` plus window lifecycle,
  attributes, and group plumbing;
- positioned MPI I/O round trips in independent, collective, and nonblocking
  forms plus atomicity, sync, metadata, and error-handler plumbing.

These cases run across the platform/backend matrix described in
[TESTING.md](TESTING.md).

## Important uncovered or partial categories

### Integer-handle ABI boundaries

UniMPI facade handles are `intptr_t`. On MPICH, Intel MPI, and MS-MPI, native
opaque handles are C `int`. Integer backends therefore need native-width
temporaries (and packed native arrays) whenever a call writes a handle through
a pointer or passes a handle array. Open MPI keeps direct pointer-handle binds
and is out of scope for these adapters.

**Width-adapted (issue #2 primary debt + companions):**

- `MPI_Comm_dup` / `MPI_Comm_free`
- `MPI_Info_create` / `MPI_Info_free`
- `MPI_Type_get_contents` (OUT datatype array stride)
- `MPI_Comm_spawn_multiple` (root Info input array + intercomm OUT; non-root
  may pass a NULL info array)

**Width-adapted (Class C — frozen matrix-exercised create/OUT/INOUT only):**

- Communicators: `Comm_dup_with_info`, `Comm_split`, `Comm_split_type`,
  `Comm_create`, `Comm_group`, `Intercomm_create`, `Intercomm_merge`
- Datatypes: `Type_contiguous`, `Type_vector`, `Type_indexed`, `Type_dup`,
  `Type_create_resized`, `Type_commit`, `Type_free`
- Groups: `Group_incl`, `Group_excl`, `Group_free`
- Ops: `Op_create`, `Op_free`
- RMA: `Win_create`, `Win_free`

These installs live in `unimpi_bind_integer_opaque_apis`
(`src/backends/opaque_handle_wrappers.c`). That binder is the sole installer of
every field it owns; Open MPI does not call it. This is **not** a claim that
every opaque OUT on every integer backend is hardened.

**Class D quarantine (raw bind may be non-NULL; not ABI-certified width):**

- Dynamic process scalars beyond spawn_multiple: `Comm_spawn`, `Comm_accept`,
  `Comm_connect`, `Comm_join`, `Comm_get_parent`, `Comm_disconnect`
- Optional Info: `Comm_get_info`
- Remaining `type_*` / `group_*` / `win_*` constructors not in the Class C
  table (for example hvector/hindexed/subarray/darray, group set operations,
  `Win_allocate` / `Win_allocate_shared` / `Win_create_dynamic`)
- Errhandler create/OUT paths
- File handles are pointer-width on the supported integer backends and are not
  treated as int-handle conversion debt

Quarantined scalar OUT slots may remain non-NULL via raw `dlsym`. Do not claim
full-width facade safety for them. Handle-array OUT/IN that cannot ship raw
safely must be wrapped or forced NULL; within issue #2 the array debts are
`Type_get_contents` and spawn_multiple info (both adapted above).

**Class E deferred NULL:**

- Integer-handle `MPI_Ialltoallw` remains intentionally NULL until
  request-bound datatype-array lifetime exists. Open MPI uses its native
  pointer-handle path. Existing tests assert the integer NULL contract.

**NULL-slot contract:**

1. Missing optional symbol → binder sets `unimpi.<field> = NULL`.
2. Intentionally deferred unsafe path → binder forces NULL even if the native
   symbol exists (integer `ialltoallw` today).
3. Raw-but-quarantined → slot may be non-NULL; documentation forbids treating
   it as ABI-hardened; the support matrix does not claim width-safe coverage.
4. Wrapped → slot is non-NULL only if the native symbol is present; the call is
   handle-width safe for that path.

A non-NULL vtable entry is therefore **not** equivalent to “safe to call for
every opaque ABI property.” Callers and tests must null-check optional or
deferred slots before use:

```c
if (unimpi.ialltoallw != NULL) {
    unimpi.ialltoallw(...);
} else {
    /* expected on integer-handle backends until request-bound type arrays */
}
```

Standard-name macros do not hide NULL: calling through a NULL function pointer
is a bug. Fake binder regressions live in `tests/internal/test_opaque_handle_width.c`
(and request-side `test_request_handle_width.c` for `ialltoallw`).

### Point-to-point and requests

- broader persistent request lifecycle and error cases;
- cancellation and cancelled-status semantics;
- matched probe/receive semantics;
- broader mixed completion errors and larger request-array stress beyond the
  covered per-element status, ignored-status, and error-class paths;
- portable direct access to source/tag/error fields across native status
  layouts;
- large counts, truncation, wildcard, and `MPI_PROC_NULL` corner cases.

### Collectives

- remaining collective variants;
- broader blocking `Alltoallw` in-place, zero-count, and asymmetric
  intercommunicator cases beyond the typed array adapter;
- integer-handle `Ialltoallw`, pending request-bound lifetime management for
  converted datatype arrays; Open MPI uses its native pointer-handle path;
- noncommutative reductions and user-defined operations;
- full in-place, aliasing, and zero-count rules across other collectives;
- broad non-power-of-two and large-process testing.

### Datatypes

- every hindexed/subarray/darray constructor;
- exact lower-bound/true-extent corner cases;
- external32 packing;
- nested datatype stress and invalid-constructor arguments.

### Communicators and topology

- dynamic process management, ports, and name publishing;
- custom communicator error-handler matrix;
- exhaustive Cartesian/graph mapping and invalid topology arguments;
- distributed graph topology.

### RMA

- atomics and request-based RMA data correctness;
- lock/unlock, flush, PSCW, and mixed epoch rules;
- shared and dynamic windows;
- memory-model and displacement corner cases.

### MPI I/O

- shared/ordered file-pointer operations and split collectives;
- views, seek/position, preallocation, and broader deletion/error cases;
- custom data representations and broad filesystem behavior.

### Threading and resilience

- concurrent UniMPI lifecycle mutation;
- multi-threaded MPI call stress under `MPI_THREAD_MULTIPLE`;
- process failure, recovery, fault tolerance, GPU awareness, and vendor
  extensions.

## How to update this analysis

When adding an API:

1. add the typed vtable field and standard alias where appropriate;
2. resolve it in every applicable backend adapter;
3. decide whether it is required or optional for the supported profile;
4. add fake failure/cleanup coverage if loading or state changes;
5. add focused real-backend semantics with the necessary process counts;
6. update `SUPPORT_MATRIX.md` only after those tests pass.

### Appendix: coverage posture (not a completeness claim)

The sections above and [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) are authoritative.
The following notes retain only currently verified inventory facts and known
partial areas; they intentionally avoid "100% complete" language.

#### MPI I/O

- Backends resolve a broad MPI I/O surface (open/close, positioned
  read/write, collective and nonblocking forms, atomicity, sync, metadata,
  and error-handler plumbing).
- Focused real-backend coverage exercises positioned independent, collective,
  and nonblocking round trips plus selected metadata/error-handler paths.
- Still partial or uncovered: shared/ordered file-pointer operations, split
  collectives, views, seek/position breadth, preallocation, deletion/error
  matrices, custom data representations, and broad filesystem behavior.

#### Dynamic processes

- Backends attempt to load spawn, connect/accept, port, name-service, and
  parent-query symbols where the vendor exports them.
- On integer-handle backends, `MPI_Comm_spawn_multiple` is width-adapted when
  the symbol is present; single `MPI_Comm_spawn` and other dynamic OUT paths
  remain Class D quarantine (raw, not ABI-certified width).
- Focused tests exist (`tests/mpi/test_dynamic.c`), but runtime availability
  and correctness remain backend- and environment-dependent (MS-MPI dynamic
  process support is limited and environment-sensitive; missing symbols leave
  the corresponding vtable slots NULL rather than inventing permanent stubs).
- Dynamic process management remains listed under uncovered/partial categories
  above and must not be described as fully covered.

#### MPI-4 surfaces

- Sessions, partitioned communication, and the tools interface are not part of
  the verified UniMPI support surface.

## Summary

### Strengths (verified inventory, not completeness)

- Broad environment management, P2P, collectives, communicators/groups, and
  selected topology coverage through focused tests.
- Representative RMA fence paths and window lifecycle/attribute plumbing.
- Positioned MPI I/O round trips with selected metadata and error-handler paths.
- Backend adapters for Open MPI, MPICH, Intel MPI, and MS-MPI with runtime
  loading and identity detection.

### Gaps and partial areas

- Non-request opaque-handle ABI hardening on integer backends is **partial**:
  primary debt paths and the frozen Class C matrix set are width-adapted;
  Class D quarantine remains raw and uncertified; this is not full MPI-3
  opaque-handle completeness.
- Dynamic process management, ports, and name publishing remain partial and
  environment-dependent.
- MPI I/O beyond the focused positioned/collective/nonblocking paths remains
  partial.
- Nonblocking collectives and integer-handle `Ialltoallw` (intentionally NULL)
  remain partial on some backends (see the support matrix).
- Shared/dynamic windows, broader RMA epochs, and request-based RMA remain
  partial or uncovered.
- MPI-4 sessions, partitioned communication, and tools interface are absent.

### Recommendations

1. Extend real-backend semantics only with focused tests that update
   `SUPPORT_MATRIX.md` after they pass.
2. Prefer matrix-driven claims over historical percentage tables.
3. Keep process-failure and vendor extension classes out of the portable
   support claims unless a backend publishes stable public constants.

## References

- [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) - maintained verification contract
- [MPI References](mpi-references.md) - Official documentation links
- [MPI API Summary](mpi-api-summary.md) - Quick reference

Historical plans under `docs/superpowers/plans/` may mention 400+ functions or
100% MPI-3 coverage. Those are planning artifacts, not current capability
claims.
