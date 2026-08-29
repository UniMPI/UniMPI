# Standard-Name API Quick Reference

This is a UniMPI usage reference, not a complete MPI standard reference. The
current standard-name header provides 296 direct `MPI_*` aliases plus separate
control wrappers, function-like macros, and constants. Availability at compile
time does not prove that every backend exports or semantically verifies an
operation.

For the tested boundary, see [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md). For the MPI
standard, use [mpi-references.md](mpi-references.md).

## Enable standard names

Per translation unit:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"
```

Or enable the target-wide definition when configuring UniMPI:

```bash
cmake -S . -B build -DUNIMPI_ENABLE_STD_MACROS=ON
```

The macros dispatch through the same runtime vtable as direct `unimpi.*` calls.

## Lifecycle

```c
int MPI_Init(int *argc, char ***argv);
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided);
int MPI_Finalize(void);
int MPI_Initialized(int *flag);
int MPI_Finalized(int *flag);
```

Use `&argc` with `&argv`, or pass both as `NULL`. Check every return value.

## Basic communicator queries

```c
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int MPI_Comm_free(MPI_Comm *comm);
```

Predefined communicators are populated by the selected backend:

```c
MPI_COMM_WORLD
MPI_COMM_SELF
```

## Point-to-point

```c
int MPI_Send(const void *buf, int count, MPI_Datatype datatype,
             int dest, int tag, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype,
             int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype,
              int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype,
              int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Wait(MPI_Request *request, MPI_Status *status);
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
int MPI_Waitall(int count, MPI_Request *requests, MPI_Status *statuses);
int MPI_Sendrecv(...);
```

Focused tests cover representative blocking/nonblocking paths, request-array
completion, zero-count request arrays, and a `Send_init`/`Recv_init` +
`Startall` path. They do not cover matched probes, cancellation semantics,
every send mode, wildcard behavior, or the full persistent-request lifecycle.

## Collectives

Common aliases include:

```c
MPI_Barrier
MPI_Bcast
MPI_Reduce
MPI_Allreduce
MPI_Gather
MPI_Gatherv
MPI_Allgather
MPI_Scatter
MPI_Alltoall
MPI_Reduce_scatter
MPI_Scan
MPI_Ibarrier
```

Focused tests exercise the blocking calls above plus the `Ibarrier`, fixed and
variable-count gather/scatter/all-to-all families, and nonblocking reduction
families. This is still a representative subset rather than exhaustive
collective conformance or corner-case coverage.

## Datatypes

Common predefined datatypes:

```c
MPI_CHAR
MPI_BYTE
MPI_SHORT
MPI_INT
MPI_LONG
MPI_LONG_LONG
MPI_FLOAT
MPI_DOUBLE
MPI_LONG_DOUBLE
```

Representative constructors and queries:

```c
MPI_Type_contiguous
MPI_Type_vector
MPI_Type_indexed
MPI_Type_dup
MPI_Type_create_resized
MPI_Type_commit
MPI_Type_free
MPI_Type_size
MPI_Type_get_extent
MPI_Type_get_envelope
MPI_Type_get_contents
MPI_Pack
MPI_Unpack
```

Do not copy numeric datatype values from a vendor header. UniMPI initializes
the exported predefined objects for the selected backend.

## Communicators, groups, and topology

Focused tests exercise representative calls from:

```c
MPI_Comm_compare
MPI_Comm_group
MPI_Group_size
MPI_Group_rank
MPI_Group_incl
MPI_Group_excl
MPI_Group_compare
MPI_Intercomm_create
MPI_Intercomm_merge
MPI_Dims_create
MPI_Cart_create
MPI_Cart_get
MPI_Cart_shift
MPI_Graph_create
MPI_Graphdims_get
MPI_Topo_test
```

See the support matrix for untested topology and dynamic-process operations.

## RMA and MPI I/O

The interface includes a broad set of window and file fields. Current focused
tests cover fence-based `Put`, `Get`, and `Accumulate`; window
lifecycle/attributes; and positioned independent, collective, and nonblocking
file round trips plus selected metadata. They do not cover the full RMA epoch,
atomic-operation, file-view, shared-pointer, or split-collective matrix.

Representative aliases:

```c
MPI_Win_create
MPI_Win_allocate
MPI_Win_fence
MPI_Put
MPI_Get
MPI_Accumulate
MPI_Win_free
MPI_Win_create_keyval
MPI_Win_set_attr
MPI_Win_get_attr

MPI_File_open
MPI_File_close
MPI_File_write_at
MPI_File_read_at
MPI_File_write_at_all
MPI_File_read_at_all
MPI_File_iwrite_at
MPI_File_iread_at
MPI_File_set_atomicity
MPI_File_get_atomicity
MPI_File_sync
```

## Thread levels

```c
MPI_THREAD_SINGLE
MPI_THREAD_FUNNELED
MPI_THREAD_SERIALIZED
MPI_THREAD_MULTIPLE
```

`MPI_Init_thread` reports backend MPI thread support. It does not certify
thread-safe concurrent UniMPI initialization or finalization.

## Runtime selection reminder

Standard-name macros do not select a backend. Runtime priority remains:

```text
UNIMPI_LIBRARY > UNIMPI_BACKEND > platform fallback
```

Use [BACKENDS.md](BACKENDS.md) to choose a matching launcher/library pair.
