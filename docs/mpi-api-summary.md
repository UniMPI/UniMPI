# MPI API Quick Reference

A concise reference for the most commonly used MPI functions (MPI-3.1). For complete API documentation, see [MPI References](mpi-references.md).

## Legend

- **Comm**: Communicator (e.g., `MPI_COMM_WORLD`)
- **Rank**: Process rank (integer)
- **Tag**: Message tag (integer)
- **Datatype**: MPI_Datatype (e.g., `MPI_INT`, `MPI_DOUBLE`)
- **Op**: Reduction operation (e.g., `MPI_SUM`, `MPI_MAX`)
- **Request**: MPI_Request handle for non-blocking operations
- **Status**: MPI_Status structure for receive information

---

## Environment Management

Initialize and finalize the MPI environment.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Init` | `int MPI_Init(int *argc, char ***argv)` | Initialize MPI |
| `MPI_Init_thread` | `int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)` | Initialize with threading support |
| `MPI_Finalize` | `int MPI_Finalize(void)` | Terminate MPI |
| `MPI_Initialized` | `int MPI_Initialized(int *flag)` | Check if MPI is initialized |
| `MPI_Finalized` | `int MPI_Finalized(int *flag)` | Check if MPI is finalized |
| `MPI_Abort` | `int MPI_Abort(MPI_Comm comm, int errorcode)` | Abort MPI execution |
| `MPI_Wtime` | `double MPI_Wtime(void)` | Wall-clock time |
| `MPI_Wtick` | `double MPI_Wtick(void)` | Clock resolution |

---

## Communicator Information

Query process ranks and communicator sizes.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Comm_rank` | `int MPI_Comm_rank(MPI_Comm comm, int *rank)` | Get process rank |
| `MPI_Comm_size` | `int MPI_Comm_size(MPI_Comm comm, int *size)` | Get number of processes |
| `MPI_Comm_compare` | `int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)` | Compare communicators |
| `MPI_Comm_dup` | `int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)` | Duplicate communicator |
| `MPI_Comm_free` | `int MPI_Comm_free(MPI_Comm *comm)` | Free communicator |
| `MPI_Comm_split` | `int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)` | Split communicator |

---

## Point-to-Point Communication (Blocking)

Standard blocking send and receive operations.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Send` | `int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)` | Standard send |
| `MPI_Recv` | `int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)` | Standard receive |
| `MPI_Ssend` | `int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)` | Synchronous send |
| `MPI_Bsend` | `int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)` | Buffered send |
| `MPI_Rsend` | `int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)` | Ready send |
| `MPI_Sendrecv` | `int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)` | Combined send-receive |
| `MPI_Sendrecv_replace` | `int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status)` | In-place send-receive |
| `MPI_Get_count` | `int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count)` | Get received count |

---

## Point-to-Point Communication (Non-blocking)

Non-blocking operations return immediately with a request handle.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Isend` | `int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)` | Non-blocking send |
| `MPI_Irecv` | `int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)` | Non-blocking receive |
| `MPI_Issend` | `int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)` | Non-blocking sync send |
| `MPI_Ibsend` | `int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)` | Non-blocking buffered send |
| `MPI_Irsend` | `int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)` | Non-blocking ready send |
| `MPI_Wait` | `int MPI_Wait(MPI_Request *request, MPI_Status *status)` | Wait for completion |
| `MPI_Test` | `int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)` | Test for completion |
| `MPI_Waitany` | `int MPI_Waitany(int count, MPI_Request array_of_requests[], int *index, MPI_Status *status)` | Wait for any |
| `MPI_Testany` | `int MPI_Testany(int count, MPI_Request array_of_requests[], int *index, int *flag, MPI_Status *status)` | Test for any |
| `MPI_Waitall` | `int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[])` | Wait for all |
| `MPI_Testall` | `int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])` | Test for all |
| `MPI_Waitsome` | `int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])` | Wait for some |
| `MPI_Testsome` | `int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])` | Test for some |
| `MPI_Request_free` | `int MPI_Request_free(MPI_Request *request)` | Free request |
| `MPI_Cancel` | `int MPI_Cancel(MPI_Request *request)` | Cancel request |

---

## Collective Communication (One-to-All)

Broadcast and scatter operations.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Bcast` | `int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)` | Broadcast |
| `MPI_Scatter` | `int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)` | Scatter |
| `MPI_Scatterv` | `int MPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)` | Variable scatter |

---

## Collective Communication (All-to-One)

Gather and reduce operations.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Gather` | `int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)` | Gather |
| `MPI_Gatherv` | `int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm)` | Variable gather |
| `MPI_Reduce` | `int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)` | Reduction |
| `MPI_Reduce_scatter` | `int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)` | Reduce-scatter |
| `MPI_Reduce_scatter_block` | `int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)` | Block reduce-scatter |

---

## Collective Communication (All-to-All)

All-to-all and scan operations.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Allgather` | `int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)` | All-gather |
| `MPI_Allgatherv` | `int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm)` | Variable all-gather |
| `MPI_Alltoall` | `int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)` | All-to-all |
| `MPI_Alltoallv` | `int MPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)` | Variable all-to-all |
| `MPI_Allreduce` | `int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)` | All-reduce |
| `MPI_Scan` | `int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)` | Prefix scan |
| `MPI_Exscan` | `int MPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)` | Exclusive scan |
| `MPI_Barrier` | `int MPI_Barrier(MPI_Comm comm)` | Barrier synchronization |

---

## Derived Datatypes

Create custom datatypes for complex data structures.

| Function | C Signature | Description |
|----------|-------------|-------------|
| `MPI_Type_contiguous` | `int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype)` | Contiguous |
| `MPI_Type_vector` | `int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype)` | Vector |
| `MPI_Type_create_hvector` | `int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype)` | Heterogeneous vector |
| `MPI_Type_indexed` | `int MPI_Type_indexed(int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype)` | Indexed |
| `MPI_Type_create_hindexed` | `int MPI_Type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype)` | Heterogeneous indexed |
| `MPI_Type_create_struct` | `int MPI_Type_create_struct(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype)` | Struct |
| `MPI_Type_commit` | `int MPI_Type_commit(MPI_Datatype *datatype)` | Commit type |
| `MPI_Type_free` | `int MPI_Type_free(MPI_Datatype *datatype)` | Free type |
| `MPI_Type_size` | `int MPI_Type_size(MPI_Datatype datatype, int *size)` | Type size |
| `MPI_Type_extent` | `int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent)` | Type extent |
| `MPI_Get_address` | `int MPI_Get_address(const void *location, MPI_Aint *address)` | Get address |
| `MPI_Pack` | `int MPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm)` | Pack data |
| `MPI_Unpack` | `int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)` | Unpack data |
| `MPI_Pack_size` | `int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)` | Pack size |

---

## Common Constants

### Communicators

| Constant | Value | Description |
|----------|-------|-------------|
| `MPI_COMM_WORLD` | (impl-defined) | All processes |
| `MPI_COMM_SELF` | (impl-defined) | Current process only |
| `MPI_COMM_NULL` | 0x04000000 | Null communicator |

### Error Codes

| Constant | Value | Description |
|----------|-------|-------------|
| `MPI_SUCCESS` | 0 | Success |
| `MPI_ERR_BUFFER` | 1 | Invalid buffer pointer |
| `MPI_ERR_COUNT` | 2 | Invalid count |
| `MPI_ERR_TYPE` | 3 | Invalid datatype |
| `MPI_ERR_TAG` | 4 | Invalid tag |
| `MPI_ERR_COMM` | 5 | Invalid communicator |
| `MPI_ERR_RANK` | 6 | Invalid rank |
| `MPI_ERR_REQUEST` | 7 | Invalid request |
| `MPI_ERR_ROOT` | 7 | Invalid root |
| `MPI_ERR_GROUP` | 8 | Invalid group |
| `MPI_ERR_OP` | 9 | Invalid operation |
| `MPI_ERR_TOPOLOGY` | 10 | Invalid topology |
| `MPI_ERR_DIMS` | 11 | Invalid dimensions |
| `MPI_ERR_ARG` | 12 | Invalid argument |
| `MPI_ERR_UNKNOWN` | 13 | Unknown error |
| `MPI_ERR_TRUNCATE` | 14 | Message truncated |
| `MPI_ERR_OTHER` | 15 | Other error |
| `MPI_ERR_INTERN` | 16 | Internal error |
| `MPI_ERR_PENDING` | 18 | Pending request |
| `MPI_ERR_IN_STATUS` | 19 | Error in status |

### Predefined Datatypes

| C Type | MPI Datatype |
|--------|--------------|
| `char` | `MPI_CHAR` |
| `signed char` | `MPI_SIGNED_CHAR` |
| `unsigned char` | `MPI_UNSIGNED_CHAR` |
| `short` | `MPI_SHORT` |
| `unsigned short` | `MPI_UNSIGNED_SHORT` |
| `int` | `MPI_INT` |
| `unsigned int` | `MPI_UNSIGNED` |
| `long` | `MPI_LONG` |
| `unsigned long` | `MPI_UNSIGNED_LONG` |
| `long long` | `MPI_LONG_LONG` |
| `float` | `MPI_FLOAT` |
| `double` | `MPI_DOUBLE` |
| `long double` | `MPI_LONG_DOUBLE` |
| `uint8_t` | `MPI_UINT8_T` |
| `uint16_t` | `MPI_UINT16_T` |
| `uint32_t` | `MPI_UINT32_T` |
| `uint64_t` | `MPI_UINT64_T` |
| `int8_t` | `MPI_INT8_T` |
| `int16_t` | `MPI_INT16_T` |
| `int32_t` | `MPI_INT32_T` |
| `int64_t` | `MPI_INT64_T` |
| `bool` | `MPI_C_BOOL` |
| `float complex` | `MPI_C_COMPLEX` |
| `double complex` | `MPI_C_DOUBLE_COMPLEX` |
| `void*` (for `MPI_Send`) | `MPI_BYTE` |

### Reduction Operations

| Operation | Description |
|-----------|-------------|
| `MPI_MAX` | Maximum |
| `MPI_MIN` | Minimum |
| `MPI_SUM` | Sum |
| `MPI_PROD` | Product |
| `MPI_LAND` | Logical AND |
| `MPI_BAND` | Bitwise AND |
| `MPI_LOR` | Logical OR |
| `MPI_BOR` | Bitwise OR |
| `MPI_LXOR` | Logical XOR |
| `MPI_BXOR` | Bitwise XOR |
| `MPI_MAXLOC` | Maximum value and location |
| `MPI_MINLOC` | Minimum value and location |
| `MPI_REPLACE` | Replace (for RMA) |
| `MPI_NO_OP` | No operation |

### Special Constants

| Constant | Description |
|----------|-------------|
| `MPI_ANY_SOURCE` | Receive from any sender |
| `MPI_ANY_TAG` | Receive with any tag |
| `MPI_STATUS_IGNORE` | Ignore status |
| `MPI_STATUSES_IGNORE` | Ignore statuses array |
| `MPI_REQUEST_NULL` | Null request |
| `MPI_PROC_NULL` | Null process (for send) |
| `MPI_UNDEFINED` | Undefined value |
| `MPI_BOTTOM` | Base address |

### Thread Levels

| Level | Value | Description |
|-------|-------|-------------|
| `MPI_THREAD_SINGLE` | 0 | Only main thread |
| `MPI_THREAD_FUNNELED` | 1 | Only main makes MPI calls |
| `MPI_THREAD_SERIALIZED` | 2 | Only one thread at a time |
| `MPI_THREAD_MULTIPLE` | 3 | Multiple threads concurrent |

---

## See Also

- [MPI References](mpi-references.md) - Official documentation links
- [API.md](API.md) - UNIMPI-specific API documentation
- [BACKENDS.md](BACKENDS.md) - Backend-specific information

## Version

This reference covers MPI-3.1 standard functions.
