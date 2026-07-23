# MPI API Inventory and Gaps

This document describes the current implementation inventory. It deliberately
does not calculate a percentage against "all MPI functions": function counts
vary by standard revision and language binding, and an exported pointer is not
proof of correct behavior.

The maintained verification contract is
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).

## Inventory

- 275 MPI function-pointer fields in `unimpi_vtable_t`.
- 246 direct standard-name aliases in `unimpi_std_macros.h`.
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
- MPI-2.2 resized/envelope/contents datatype operations;
- environment/thread queries, Info CRUD, object names, memory allocation,
  custom operations, and status helpers;
- fence-based RMA `Put`, `Get`, and `Accumulate` plus window lifecycle,
  attributes, and group plumbing;
- positioned MPI I/O round trips in independent, collective, and nonblocking
  forms plus atomicity, sync, metadata, and error-handler plumbing.

These cases run across the platform/backend matrix described in
[TESTING.md](TESTING.md).

## Important uncovered or partial categories

### Point-to-point and requests

- broader persistent request lifecycle and error cases;
- cancellation and cancelled-status semantics;
- matched probe/receive semantics;
- mixed completion errors and larger request-array stress;
- per-element status results from multi-request completion arrays, pending a
  native/facade status-array stride adapter;
- portable direct access to source/tag/error fields across native status
  layouts;
- large counts, truncation, wildcard, and `MPI_PROC_NULL` corner cases.

### Collectives

- remaining collective variants and full in-place/aliasing rules;
- `Alltoallw` datatype arrays on integer-handle backends, pending a typed
  array adapter;
- noncommutative reductions and user-defined operations;
- in-place and zero-count corner cases;
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

Historical plans under `docs/superpowers/plans/` may mention 400+ functions or
100% MPI-3 coverage. Those are planning artifacts, not current capability
claims.
