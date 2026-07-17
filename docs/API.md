# API Reference

Complete API reference for `unimpi`.

---

## ⭐ Recommended: Standard MPI Style

For most users, we recommend using **Standard MPI Style** with macros:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    printf("Hello from rank %d\n", rank);
    
    MPI_Finalize();
    return 0;
}
```

This provides:
- **Drop-in replacement**: Existing MPI code works without changes
- **Familiar syntax**: Standard MPI function names
- **Portability**: Easy to migrate to/from other MPI implementations
- **IDE support**: Autocomplete and documentation for standard MPI

The API reference below documents the underlying function pointer interface.
Use this only if you need explicit backend control.

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

### unimpi_init_with

```c
int unimpi_init_with(const char *backend_name);
```

Initialize with a specific backend.

**Parameters:**
- `backend_name` - "openmpi", "mpich", "intelmpi", or "msmpi"

**Example:**
```c
unimpi_init_with("intelmpi");
```

---

### unimpi_finalize

```c
int unimpi_finalize(void);
```

Finalize the library.

**Returns:** `UNIMPI_OK` on success.

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
unimpi.send(&data, 1, MPI_INT, 1, 0, UNIMPI_COMM_WORLD);
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
unimpi.recv(&data, 1, MPI_INT, 0, 0, UNIMPI_COMM_WORLD, &status);
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
int data = 42;
unimpi.isend(&data, 1, MPI_INT, 1, 0, UNIMPI_COMM_WORLD, &req);
// ... do other work ...
unimpi.wait(&req, MPI_STATUS_IGNORE);
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
unimpi.bcast(&data, 1, MPI_INT, 0, UNIMPI_COMM_WORLD);
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
unimpi.reduce(&send, &recv, 1, MPI_INT, MPI_SUM, 0, UNIMPI_COMM_WORLD);
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

| Type | Description |
|------|-------------|
| `MPI_CHAR` | Character |
| `MPI_INT` | Integer |
| `MPI_FLOAT` | Float |
| `MPI_DOUBLE` | Double |
| `MPI_LONG` | Long integer |
| `MPI_BYTE` | Byte |

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

---

## Error Codes

| Code | Value | Description |
|------|-------|-------------|
| `UNIMPI_OK` | 0 | Success |
| `UNIMPI_ERR_NO_BACKEND` | -1 | No MPI backend found |
| `UNIMPI_ERR_BACKEND_LOAD` | -2 | Failed to load backend |
| `UNIMPI_ERR_NOT_INITIALIZED` | -4 | Not initialized |
| `UNIMPI_ERR_ALREADY_INITIALIZED` | -5 | Already initialized |
| `UNIMPI_ERR_SYMBOL_NOT_FOUND` | -6 | Symbol not found |

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
