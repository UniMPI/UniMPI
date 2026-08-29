# API Reference

Reference for UniMPI's public control API and commonly used runtime-dispatch
fields. The vtable currently contains 325 MPI function-pointer fields and the
standard-name header contains 298 direct aliases. This document is not a claim
of complete MPI-standard coverage; consult
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) before depending on a category.

---

## Standard MPI style

For MPI-shaped application code, enable standard names before including the
header:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

int main(int argc, char **argv) {
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        return 1;
    }
    
    int rank;
    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS) {
        MPI_Finalize();
        return 1;
    }
    
    printf("Hello from rank %d\n", rank);
    
    return MPI_Finalize() == MPI_SUCCESS ? 0 : 1;
}
```

This provides:
- familiar MPI function names for the aliases UniMPI exposes;
- the same runtime dispatch used by direct `unimpi.*` calls;
- source-level convenience when porting code within the supported subset.

The API reference below documents the underlying function pointer interface.
Standard-name compatibility is not complete enough to treat arbitrary MPI
programs as drop-in compatible.

---

## Table of Contents

- [Initialization and Finalization](#initialization-and-finalization)
- [Error Handling](#error-handling)
- [Environment Management](#environment-management)
- [Point-to-Point Communication](#point-to-point-communication)
- [Collective Communication](#collective-communication)
- [Data Types](#data-types)
- [Communicators and Groups](#communicators-and-groups)
- [RMA (One-Sided Communication)](#rma-one-sided-communication)
- [Parallel I/O](#parallel-io)
- [Process Management](#process-management)
- [Error Codes](#error-codes)

---

## Initialization and Finalization

### unimpi_init

```c
int unimpi_init(int *argc, char ***argv);
```

Initialize the unimpi library with auto-detected backend.

**Parameters:**
- `argc` - Pointer to argument count
- `argv` - Pointer to argument array

**Returns:** `UNIMPI_OK` on success, error code otherwise.

**Example:**
```c
unimpi_init(&argc, &argv);
```

---

### unimpi_init_thread

```c
int unimpi_init_thread(int *argc, char ***argv,
                       int required, int *provided);
```

Initialize UniMPI and negotiate one of `UNIMPI_THREAD_SINGLE`,
`UNIMPI_THREAD_FUNNELED`, `UNIMPI_THREAD_SERIALIZED`, or
`UNIMPI_THREAD_MULTIPLE`. The backend may provide a lower level; applications
must obey the returned level.

---

### unimpi_init_with

```c
int unimpi_init_with(const char *backend_name);
```

Initialize with a backend name or loader path. This entry point passes
`NULL, NULL` to the backend `MPI_Init`; use `unimpi_init` when the backend
should receive application arguments.

**Parameters:**
- `backend_name` - `"openmpi"`, `"mpich"`, `"intelmpi"`, `"msmpi"`, or a
  loader path/name

**Example:**
```c
unimpi_init_with("/opt/intel/oneapi/mpi/latest/lib/libmpi.so");
```

An exact path is safer than a backend name, especially on macOS or a host with
multiple MPI installations.

---

### unimpi_finalize

```c
int unimpi_finalize(void);
```

Finalize the library.

**Returns:** `UNIMPI_OK` on success.

After successful finalization, UniMPI follows MPI lifecycle rules and cannot
be initialized again in the same process.

---

## Error Handling

### unimpi_error_string

```c
const char *unimpi_error_string(int error_code);
```

Get human-readable error message.

**Example:**
```c
int ret = unimpi_init(&argc, &argv);
if (ret != UNIMPI_OK) {
    fprintf(stderr, "Error: %s\n", unimpi_error_string(ret));
}
```

---

### unimpi_get_backend_name

```c
const char *unimpi_get_backend_name(void);
```

Get name of currently loaded backend.

**Returns:** String like "openmpi", "mpich", etc.

---

### Runtime state and identity

```c
int unimpi_is_initialized(void);
int unimpi_mpi_initialized(int *flag);
int unimpi_mpi_finalized(int *flag);
const char *unimpi_get_library_path(void);
```

`unimpi_get_backend_name` and `unimpi_get_library_path` return pointers owned
by UniMPI. Treat them as read-only and do not retain them after finalization.

---

## Environment Management

### unimpi.comm_rank

```c
int (*comm_rank)(MPI_Comm comm, int *rank);
```

Get rank in communicator.

**Example:**
```c
int rank;
unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
```

---

### unimpi.comm_size

```c
int (*comm_size)(MPI_Comm comm, int *size);
```

Get size of communicator.

**Example:**
```c
int size;
unimpi.comm_size(UNIMPI_COMM_WORLD, &size);
```

---

### unimpi.wtime

```c
double (*wtime)(void);
```

Get wall clock time.

**Example:**
```c
double start = unimpi.wtime();
// ... work ...
double end = unimpi.wtime();
printf("Time: %f sec\n", end - start);
```

---

## Point-to-Point Communication

### unimpi.send

```c
int (*send)(const void *buf, int count, MPI_Datatype datatype,
            int dest, int tag, MPI_Comm comm);
```

Standard blocking send.

**Example:**
```c
int data = 42;
unimpi.send(&data, 1, UNIMPI_INT, 1, 0, UNIMPI_COMM_WORLD);
```

---

### unimpi.recv

```c
int (*recv)(void *buf, int count, MPI_Datatype datatype,
            int source, int tag, MPI_Comm comm, UNIMPI_Status *status);
```

Standard blocking receive.

**Example:**
```c
int data;
UNIMPI_Status status;
unimpi.recv(&data, 1, UNIMPI_INT, 0, 0, UNIMPI_COMM_WORLD, &status);
```

---

### unimpi.isend / unimpi.irecv

```c
int (*isend)(const void *buf, int count, MPI_Datatype datatype,
             int dest, int tag, MPI_Comm comm, MPI_Request *request);
int (*irecv)(void *buf, int count, MPI_Datatype datatype,
             int source, int tag, MPI_Comm comm, MPI_Request *request);
```

Non-blocking send/receive.

**Example:**
```c
MPI_Request req;
MPI_Status status;
int data = 42;
unimpi.isend(&data, 1, UNIMPI_INT, 1, 0, UNIMPI_COMM_WORLD, &req);
// ... do other work ...
unimpi.wait(&req, &status);
```

---

### unimpi.wait

```c
int (*wait)(MPI_Request *request, UNIMPI_Status *status);
```

Wait for non-blocking operation to complete.

---

## Collective Communication

### unimpi.bcast

```c
int (*bcast)(void *buffer, int count, MPI_Datatype datatype,
             int root, MPI_Comm comm);
```

Broadcast from root to all.

**Example:**
```c
int data = 100;
unimpi.bcast(&data, 1, UNIMPI_INT, 0, UNIMPI_COMM_WORLD);
```

---

### unimpi.reduce

```c
int (*reduce)(const void *sendbuf, void *recvbuf, int count,
              MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
```

Reduce operation.

**Example:**
```c
int send = rank;
int recv;
unimpi.reduce(&send, &recv, 1, UNIMPI_INT, UNIMPI_SUM, 0,
              UNIMPI_COMM_WORLD);
```

---

### unimpi.allreduce

```c
int (*allreduce)(const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
```

All-reduce (result on all processes).

---

### unimpi.barrier

```c
int (*barrier)(MPI_Comm comm);
```

Barrier synchronization.

**Example:**
```c
unimpi.barrier(UNIMPI_COMM_WORLD);
```

---

## Data Types

### Predefined Types

| Direct style | Standard-name style | Description |
|---|---|---|
| `UNIMPI_CHAR` | `MPI_CHAR` | Character |
| `UNIMPI_INT` | `MPI_INT` | Integer |
| `UNIMPI_FLOAT` | `MPI_FLOAT` | Float |
| `UNIMPI_DOUBLE` | `MPI_DOUBLE` | Double |
| `UNIMPI_LONG` | `MPI_LONG` | Long integer |
| `UNIMPI_BYTE` | `MPI_BYTE` | Byte |

---

### unimpi.type_contiguous

```c
int (*type_contiguous)(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
```

Create contiguous type.

---

### unimpi.type_commit

```c
int (*type_commit)(MPI_Datatype *datatype);
```

Commit a datatype.

---

### unimpi.type_free

```c
int (*type_free)(MPI_Datatype *datatype);
```

Free a datatype.

---

## Communicators and Groups

### unimpi.comm_split

```c
int (*comm_split)(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
```

Split communicator.

**Example:**
```c
MPI_Comm newcomm;
unimpi.comm_split(UNIMPI_COMM_WORLD, rank % 2, rank, &newcomm);
```

---

### unimpi.comm_dup

```c
int (*comm_dup)(MPI_Comm comm, MPI_Comm *newcomm);
```

Duplicate communicator.

---

### unimpi.group_incl

```c
int (*group_incl)(MPI_Group group, int n, const int *ranks, MPI_Group *newgroup);
```

Create group from ranks.

---

## RMA (One-Sided Communication)

### unimpi.win_create

```c
int (*win_create)(void *base, MPI_Aint size, int disp_unit,
                  MPI_Info info, MPI_Comm comm, MPI_Win *win);
```

Create RMA window.

Focused tests cover fence-based `Put`, `Get`, and `Accumulate`. Other RMA
fields may exist without focused cross-backend semantic coverage; see the
support matrix.

---

### unimpi.put

```c
int (*put)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
           int target_rank, MPI_Aint target_disp, int target_count,
           MPI_Datatype target_datatype, MPI_Win win);
```

Put data to remote window.

---

### unimpi.get

```c
int (*get)(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
           int target_rank, MPI_Aint target_disp, int target_count,
           MPI_Datatype target_datatype, MPI_Win win);
```

Get data from remote window.

---

### unimpi.win_fence

```c
int (*win_fence)(int assert, MPI_Win win);
```

Fence synchronization.

---

## Parallel I/O

### unimpi.file_open

```c
int (*file_open)(MPI_Comm comm, const char *filename, int amode,
                 MPI_Info info, MPI_File *fh);
```

Open file for parallel I/O.

Focused tests cover selected positioned independent, collective, and
nonblocking operations. File views, shared/ordered pointers, and other fields
remain outside the verified boundary.

---

### unimpi.file_read

```c
int (*file_read)(MPI_File fh, void *buf, int count,
                 MPI_Datatype datatype, UNIMPI_Status *status);
```

Read from file.

---

### unimpi.file_write

```c
int (*file_write)(MPI_File fh, const void *buf, int count,
                  MPI_Datatype datatype, UNIMPI_Status *status);
```

Write to file.

---

## Process Management

### unimpi.comm_spawn

```c
int (*comm_spawn)(const char *command, char *argv[], int maxprocs,
                  MPI_Info info, int root, MPI_Comm comm,
                  MPI_Comm *intercomm, int array_of_errcodes[]);
```

Spawn new processes.

Dynamic process management is present in the dispatch inventory but has no
focused cross-backend test. Treat it as unverified.

---

## Error Codes

| Code | Value | Description |
|------|-------|-------------|
| `UNIMPI_OK` | 0 | Success |
| `UNIMPI_ERR_NO_BACKEND` | -1 | No MPI backend found |
| `UNIMPI_ERR_BACKEND_LOAD` | -2 | Failed to load backend |
| `UNIMPI_ERR_ABI_MISMATCH` | -3 | Unsupported MPI ABI/library family |
| `UNIMPI_ERR_NOT_INITIALIZED` | -4 | Not initialized |
| `UNIMPI_ERR_ALREADY_INITIALIZED` | -5 | Already initialized |
| `UNIMPI_ERR_SYMBOL_NOT_FOUND` | -6 | Symbol not found |
| `UNIMPI_ERR_OUT_OF_MEMORY` | -7 | Allocation failed |
| `UNIMPI_ERR_INVALID_ARGUMENT` | -8 | Invalid argument |
| `UNIMPI_ERR_BACKEND_NOT_SUPPORTED` | -9 | Backend is unsupported on this platform |
| `UNIMPI_ERR_BACKEND_INIT_FAILED` | -10 | Backend initialization failed |
| `UNIMPI_ERR_FINALIZED` | -11 | Process has already finalized MPI |
| `UNIMPI_ERR_INVALID_STATE` | -12 | Lifecycle transition is invalid |

---

## Standard MPI Macros

Define `UNIMPI_USE_STD_NAMES` for standard MPI naming:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

// Now you can use standard MPI names
MPI_Init(&argc, &argv);
MPI_Send(buf, count, MPI_INT, dest, tag, MPI_COMM_WORLD);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Finalize();
```

See [BACKENDS.md](BACKENDS.md) for backend-specific information.
