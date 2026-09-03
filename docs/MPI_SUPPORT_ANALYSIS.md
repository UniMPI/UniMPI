# MPI API Inventory and Gaps

This document describes the current implementation inventory. It deliberately
does not calculate a percentage against "all MPI functions": function counts
vary by standard revision and language binding, and an exported pointer is not
proof of correct behavior.

The maintained verification contract is
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).

## Inventory

- 338 MPI function-pointer fields in `unimpi_vtable_t`.
- 311 direct standard-name aliases in `unimpi_std_macros.h`.
- Separate control wrappers for initialization, finalization, state queries,
  backend identity, diagnostics, and UniMPI errors.
- Four backend adapters: Open MPI, MPICH, Intel MPI, and MS-MPI.

Backend adapters attempt to resolve a broad MPI-2.2/MPI-3-era surface. A symbol
may be absent in a particular vendor/version, so required-field validation and
focused integration tests determine the supported runtime subset.

The counts above are for the **default target (3.1)**. The exposed surface
narrows with the target version selected at build time
(`UNIMPI_MPI_TARGET_VERSION`/`UNIMPI_MPI_TARGET_SUBVERSION`): the 6 MPI-3.0
clusters (matched probe, nonblocking collectives, comm info, shared windows,
RMA atomics, RMA sync) are physically
removed below a 3.0 target, so a 2.2 build has 297 vtable fields and a matching
alias surface. The MPI-2 base (including `Alltoallw`, `Comm_join`,
`Op_commutative`) is always present. Runtime capability is still
decided by the dlopen'd backend, independent of the compile-time target. See
[VERSION_GATING.md](VERSION_GATING.md).

### MPI-2 base completeness

The always-present 2.2 surface is mechanically audited by
`tools/mpi_version_gate.py base`, which (a) fails if any gated field is
actually a 2.2 canonical function and (b) reports how many of the ~305
canonical 2.2 surface is exposed. **305/305 (100%)** of the MPI-2.2 canonical
entities are covered: 293 are vtable function-pointer fields, and the 12
predefined attribute callbacks (`MPI_*_DUP_FN` / `MPI_*_NULL_COPY_FN` /
`MPI_*_NULL_DELETE_FN`) are exposed as extern global callback-value variables
that each backend resolves to its own value at init (OpenMPI -> real
OMPI_C_MPI_*_FN functions; MPICH-family -> shared MPIR_Dup_fn plus NULL),
exactly like the extern MPI_ERR_* ints. The common subset already added (`MPI_Type_create_hvector`/
`hindexed`/`struct`, `MPI_Get_address`, `MPI_Type_create_f90_*`,
`MPI_Ibsend`/`Irsend`/`Issend`, `MPI_Info_dup`/`get_valuelen`,
`MPI_Request_get_status`, `MPI_Reduce_local`,
`MPI_Comm`/`Win`/`File_get|set_errhandler`) plus a second batch
(`MPI_Error_class`/`MPI_Error_string`, `MPI_Pcontrol`,
`MPI_File_get_type_extent`, `MPI_File_*_shared`, and the
`MPI_File_*_{begin,end}` split collectives, `MPI_Dist_graph_*`,
`MPI_Status_f2c`/`c2f`, `MPI_Grequest_*`, `MPI_Comm`/`Type_*`_attr and
`MPI_Keyval_create`/`free`, `MPI_Register_datarep`) are validated
against real OpenMPI and MPICH in `tests/mpi/test_base_completeness.c`,
`tests/mpi/test_tierA_gaps.c`, `tests/mpi/test_tierD_dist_graph.c`,
`tests/mpi/test_status_fortran.c`, `tests/mpi/test_grequest.c`, and
`tests/mpi/test_attr.c`.

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
- large counts, truncation, wildcard, and `MPI_PROC_NULL` corner cases.

Per-element status arrays from multi-request completion calls are adapted in
place to the native 20-byte record stride, and `MPI_Get_count`/`MPI_Get_elements`
are bound, so portable status access through the standard accessors is covered.
For `MPI_SOURCE`/`MPI_TAG`/`MPI_ERROR`, read them via UniMPI's accessors
(`MPI_Status_get_source`/`MPI_Status_get_tag`/`MPI_Status_get_error` or
`unimpi.status_get_source`/`_tag`/`_error`), which are backend-layout aware;
direct `status.MPI_SOURCE`-style reads are not portable (see docs/API.md and
SUPPORT_MATRIX.md).

### Collectives

- remaining collective variants and full in-place/aliasing rules;
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

### ⚠️ I/O (38/38 - 100%)

| Function | Status | Notes |
|----------|--------|-------|
| MPI_File_open/Close | ✅ | Basic I/O |
| MPI_File_delete | ✅ | File deletion |
| MPI_File_set_size/Get_size | ✅ | File sizing |
| MPI_File_preallocate | ✅ | Preallocation |
| MPI_File_get_group | ✅ | Group query |
| MPI_File_get_amode | ✅ | Access mode query |
| MPI_File_get_info/Set_info | ✅ | Info management |
| MPI_File_seek/Get_position/Get_byte_offset | ✅ | Positioning |
| MPI_File_read/Write | ✅ | Individual I/O |
| MPI_File_read_all/Write_all | ✅ | Collective I/O |
| MPI_File_read_at/Write_at | ✅ | Explicit offset |
| MPI_File_read_at_all/Write_at_all | ✅ | Collective explicit offset |
| MPI_File_read_shared/Write_shared | ✅ | Shared pointer |
| MPI_File_read_ordered/Write_ordered | ✅ | Ordered collective |
| MPI_File_iread/Iwrite | ✅ | Nonblocking |
| MPI_File_iread_at/Iwrite_at | ✅ | Nonblocking explicit offset |
| MPI_File_set_view/Get_view | ✅ | View management |
| MPI_File_set_atomicity/Get_atomicity | ✅ | MPI 2.2 - Atomicity |
| MPI_File_sync | ✅ | MPI 2.2 - Synchronization |
| MPI_File_create_errhandler | ✅ | Error handler creation |
| MPI_File_call_errhandler | ✅ | MPI 2.2 - Call error handler |
| MPI_File_set_errhandler/Get_errhandler | ✅ | MPI 2.2 - Error handler management |

### ✅ Dynamic Processes (8/8 - 100%)

| Function | Status | Notes |
|----------|--------|-------|
| MPI_Comm_spawn | ✅ | Spawn processes |
| MPI_Comm_spawn_multiple | ✅ | Multiple spawn |
| MPI_Comm_get_parent | ✅ | Parent access |
| MPI_Comm_join | ✅ | Join |
| MPI_Comm_connect/Accept | ✅ | Client-server |
| MPI_Open_port | ✅ | Port management |
| MPI_Close_port | ✅ | Port management |
| MPI_Publish/Lookup_name | ✅ | Name service |

**Note:** All functions implemented. See `tests/mpi/test_dynamic.c` for test coverage.

### ❌ Sessions (MPI-4) (0/6 - 0%)

| Function | Status | Notes |
|----------|--------|-------|
| MPI_Session_init | ❌ | MPI-4 |
| MPI_Session_finalize | ❌ | MPI-4 |
| MPI_Session_get_info | ❌ | MPI-4 |
| MPI_Session_get_pset | ❌ | MPI-4 |
| MPI_Group_from_session | ❌ | MPI-4 |
| MPI_Comm_create_from_group | ❌ | MPI-4 |

### ❌ Partitioned Communication (MPI-4) (0/4 - 0%)

| Function | Status | Notes |
|----------|--------|-------|
| MPI_Psend_init | ❌ | MPI-4 |
| MPI_Precv_init | ❌ | MPI-4 |
| MPI_Pready | ❌ | MPI-4 |
| MPI_Parrived | ❌ | MPI-4 |

### ❌ Tools Interface (MPI-4) (0/5 - 0%)

| Function | Status | Notes |
|----------|--------|-------|
| MPI_T_init_thread | ❌ | MPI-4 |
| MPI_T_finalize | ❌ | MPI-4 |
| MPI_T_category_get_info | ❌ | MPI-4 |
| MPI_T_pvar_read/write | ❌ | MPI-4 |
| MPI_T_cvar_get_info | ❌ | MPI-4 |

## Summary

### Strengths
- ✅ Complete environment management
- ✅ Comprehensive P2P and collective communication
- ✅ Full communicator and group operations
- ✅ Complete process topology support (Cartesian and Graph)
- ✅ Full MPI-2.2 RMA window attributes (create_keyval, set_attr, get_attr, delete_attr, get_group)
- ✅ Good RMA support (basic + MPI-2.2 attributes + MPI-3 atomics)
- ✅ Complete MPI-2.2 I/O support (38/38 functions, including atomicity, sync, errhandler)
- ✅ Complete Dynamic Processes support (8/8 functions, including spawn, connect, name service)
- ✅ Message probing (including MPI-3 matched probes)

### Gaps
- ❌ MPI-4 new features (sessions, partitioned, tools)
- ⚠️ Non-blocking collectives (MPI-3) partial
- ⚠️ Shared memory windows (MPI-3)
- ⚠️ I/O incomplete

### Recommendations

1. **High Priority**
   - Complete non-blocking collectives (Iallgather, Iallreduce, etc.)
   - Add shared memory window support (MPI_Win_allocate_shared)

2. **Medium Priority**
   - Complete I/O operations
   - Add RMA request-based operations (Rput, Rget)

3. **Low Priority / Complex**
   - Dynamic processes (requires significant runtime integration)
   - MPI-4 sessions and partitioned communication

## References

- [MPI References](mpi-references.md) - Official documentation links
- [MPI API Summary](mpi-api-summary.md) - Quick reference

Historical plans under `docs/superpowers/plans/` may mention 400+ functions or
100% MPI-3 coverage. Those are planning artifacts, not current capability
claims.
