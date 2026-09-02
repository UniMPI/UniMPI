#ifndef UNIMPI_VTABLE_H
#define UNIMPI_VTABLE_H

#include <stddef.h>
#include <stdint.h>
#include "unimpi_version.h"
#include "unimpi_platform.h"
#include "unimpi_loader.h"

/* MPI opaque types - forward declarations
 * Use intptr_t to handle both integer handles (MPICH/MS-MPI)
 * and pointer handles (OpenMPI) on 64-bit systems
 */
typedef intptr_t MPI_Comm;
typedef intptr_t MPI_Datatype;
typedef intptr_t MPI_Op;
typedef intptr_t MPI_Group;
typedef intptr_t MPI_Request;
typedef intptr_t MPI_Info;
typedef intptr_t MPI_Win;
typedef intptr_t MPI_File;
typedef intptr_t MPI_Errhandler;
typedef intptr_t MPI_Message;
typedef intptr_t MPI_Aint;
typedef long long MPI_Offset;

/* MPI_Fint - the C type representing a Fortran INTEGER, element type of the
 * status arrays used by MPI_Status_f2c/c2f. The MPI standard defines it to
 * match the C binding of Fortran INTEGER, i.e. int. */
typedef int MPI_Fint;

#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI_Count - MPI-3.0 large-count type (>= 64-bit unsigned). Non-handle,
 * passed by value; must remain 64-bit to match every backend's MPI_Count. */
typedef unsigned long long MPI_Count;
#endif

/* MPI attribute callback function types (comm/type/win) */
typedef int (MPI_Comm_copy_attr_function)(MPI_Comm oldcomm, int comm_keyval, void *extra_state,
                                          void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPI_Comm_delete_attr_function)(MPI_Comm comm, int comm_keyval, void *attribute_val,
                                            void *extra_state);
typedef int (MPI_Type_copy_attr_function)(MPI_Datatype oldtype, int type_keyval, void *extra_state,
                                          void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPI_Type_delete_attr_function)(MPI_Datatype type, int type_keyval, void *attribute_val,
                                            void *extra_state);
typedef int (MPI_Win_copy_attr_function)(MPI_Win oldwin, int win_keyval, void *extra_state,
                                         void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPI_Win_delete_attr_function)(MPI_Win win, int win_keyval, void *attribute_val,
                                           void *extra_state);

/* MPI-1 deprecated attribute callback types (for MPI_Keyval_create/free) */
typedef int (MPI_Copy_function)(MPI_Comm oldcomm, int keyval, void *extra_state,
                                void *attribute_val_in, void *attribute_val_out, int *flag);
typedef int (MPI_Delete_function)(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state);

/* MPI datarep callback types (for MPI_Register_datarep) */
typedef int (MPI_Datarep_conversion_function)(void *userbuf, MPI_Datatype datatype, int count,
                                              void *filebuf, MPI_Offset position, void *extra_state);
typedef int (MPI_Datarep_extent_function)(MPI_Datatype datatype, MPI_Aint *extent,
                                          void *extra_state);

/* Predefined attribute callback VALUES. These are not vtable function-pointer
 * fields (there is no dlsym-able uniform symbol): each backend resolves them to
 * its own compile-time value at init, exactly like the extern MPI_ERR_* ints.
 * OpenMPI provides real OMPI_C_MPI_*_FN functions; MPICH-family provides a
 * shared MPIR_Dup_fn (cast) plus NULL for the null copy/delete values. */
extern MPI_Comm_copy_attr_function *MPI_COMM_DUP_FN;
extern MPI_Comm_copy_attr_function *MPI_COMM_NULL_COPY_FN;
extern MPI_Comm_delete_attr_function *MPI_COMM_NULL_DELETE_FN;
extern MPI_Type_copy_attr_function *MPI_TYPE_DUP_FN;
extern MPI_Type_copy_attr_function *MPI_TYPE_NULL_COPY_FN;
extern MPI_Type_delete_attr_function *MPI_TYPE_NULL_DELETE_FN;
extern MPI_Win_copy_attr_function *MPI_WIN_DUP_FN;
extern MPI_Win_copy_attr_function *MPI_WIN_NULL_COPY_FN;
extern MPI_Win_delete_attr_function *MPI_WIN_NULL_DELETE_FN;
extern MPI_Copy_function *MPI_DUP_FN;
extern MPI_Copy_function *MPI_NULL_COPY_FN;
extern MPI_Delete_function *MPI_NULL_DELETE_FN;

/* Status struct - must be defined before MPI_Status
 * Union accommodating the real backend status layouts (verified against
 * system mpi.h headers):
 * - Legacy (MPICH/Intel MPI/MS-MPI): {count_lo, count_hi_and_cancelled,
 *   MPI_SOURCE@8, MPI_TAG@12, MPI_ERROR@16} = 20 bytes
 * - OpenMPI: {MPI_SOURCE@0, MPI_TAG, MPI_ERROR, _cancelled, size_t _ucount}
 *   = 24 bytes
 * The union is 24 bytes (max of the two row layouts). Single status values
 * are treated as opaque 24-byte containers whose fields are read directly
 * (status.MPI_SOURCE etc.); only status *arrays* need compress/expand
 * because the native stride (20 vs 24) differs from the union stride (24).
 */

/* Legacy backend status layout (MPICH/Intel MPI/MS-MPI) - 20 bytes */
struct unimpi_status_legacy {
    int count_lo;                /* Lower 32-bits of count */
    int count_hi_and_cancelled;  /* Upper 32-bits and cancelled flag */
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
};

/* OpenMPI status layout (ompi_status_public_t) - 24 bytes */
struct unimpi_status_openmpi {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int _cancelled;
    size_t _ucount;              /* Actual count as size_t */
};

/* Union accommodating all backend status layouts */
union MPI_Status {
    /* Common interface - standard field access */
    struct {
        int MPI_SOURCE;
        int MPI_TAG;
        int MPI_ERROR;
        int _internal[3];        /* Reserved for internal use */
    } base;

    /* Backend-specific layouts */
    struct unimpi_status_legacy legacy;
    struct unimpi_status_openmpi openmpi;
};
typedef union MPI_Status MPI_Status;
/* Verify union is exactly 24 bytes (offset/size of largest member) */
typedef char unimpi_status_union_size_is_24[(sizeof(union MPI_Status) == 24) ? 1 : -1];

/* Generalized-request callback function types (MPI-2). Defined after MPI_Status
 * because MPI_Grequest_query_function's signature references it. */
typedef int (MPI_Grequest_query_function)(void *extra_state, MPI_Status *status);
typedef int (MPI_Grequest_free_function)(void *extra_state);
typedef int (MPI_Grequest_cancel_function)(void *extra_state, int complete);

/* MPI predefined operations - will be resolved at runtime from backend */
extern MPI_Op UNIMPI_MAX;
extern MPI_Op UNIMPI_MIN;
extern MPI_Op UNIMPI_SUM;
extern MPI_Op UNIMPI_PROD;
extern MPI_Op UNIMPI_LAND;
extern MPI_Op UNIMPI_BAND;
extern MPI_Op UNIMPI_LOR;
extern MPI_Op UNIMPI_BOR;
extern MPI_Op UNIMPI_LXOR;
extern MPI_Op UNIMPI_BXOR;
extern MPI_Op UNIMPI_MINLOC;
extern MPI_Op UNIMPI_MAXLOC;

/* MPI predefined communicators */
extern MPI_Comm UNIMPI_COMM_WORLD;
extern MPI_Comm UNIMPI_COMM_SELF;

/* MPI predefined file and error-handler handles. Like all predefined MPI
 * values these are backend-resolved at init: the integer-handle backends
 * (MPICH/Intel/MS-MPI) use their magic handle values, Open MPI exposes symbol
 * addresses. They are required because MPI_File_set_errhandler(MPI_FILE_NULL,
 * MPI_ERRORS_RETURN) must reach the backend with a handle it recognizes. */
extern MPI_File UNIMPI_FILE_NULL;
extern MPI_Errhandler UNIMPI_ERRORS_ARE_FATAL;
extern MPI_Errhandler UNIMPI_ERRORS_RETURN;
extern MPI_Errhandler UNIMPI_ERRORS_ABORT;

/* MPI predefined Info */
extern MPI_Info UNIMPI_INFO_NULL;

/* MPI predefined request constant */
extern MPI_Request UNIMPI_REQUEST_NULL;

/* Runtime-selected native status-ignore sentinel. Open MPI uses NULL while
 * integer-handle backends use pointer value 1. As with other predefined MPI
 * values, applications must evaluate it after successful initialization.
 * Singular and plural share the selected backend value. */
extern MPI_Status *UNIMPI_STATUS_IGNORE;
#define UNIMPI_STATUSES_IGNORE UNIMPI_STATUS_IGNORE

/* MPI topology types - resolved at runtime from backend */
extern int UNIMPI_CART;
extern int UNIMPI_GRAPH;
extern int UNIMPI_DIST_GRAPH;

/* MPI predefined datatypes - will be resolved at runtime from backend */
extern MPI_Datatype UNIMPI_CHAR;
extern MPI_Datatype UNIMPI_SIGNED_CHAR;
extern MPI_Datatype UNIMPI_UNSIGNED_CHAR;
extern MPI_Datatype UNIMPI_BYTE;
extern MPI_Datatype UNIMPI_SHORT;
extern MPI_Datatype UNIMPI_UNSIGNED_SHORT;
extern MPI_Datatype UNIMPI_INT;
extern MPI_Datatype UNIMPI_UNSIGNED;
extern MPI_Datatype UNIMPI_LONG;
extern MPI_Datatype UNIMPI_UNSIGNED_LONG;
extern MPI_Datatype UNIMPI_FLOAT;
extern MPI_Datatype UNIMPI_DOUBLE;
extern MPI_Datatype UNIMPI_LONG_DOUBLE;
extern MPI_Datatype UNIMPI_LONG_LONG_INT;
extern MPI_Datatype UNIMPI_LONG_LONG;
extern MPI_Datatype UNIMPI_UNSIGNED_LONG_LONG;

/* MPI predefined operations - will be resolved at runtime from backend */
extern MPI_Op UNIMPI_MAX;
extern MPI_Op UNIMPI_MIN;
extern MPI_Op UNIMPI_SUM;
extern MPI_Op UNIMPI_PROD;
extern MPI_Op UNIMPI_LAND;
extern MPI_Op UNIMPI_BAND;
extern MPI_Op UNIMPI_LOR;
extern MPI_Op UNIMPI_BOR;
extern MPI_Op UNIMPI_LXOR;
extern MPI_Op UNIMPI_BXOR;
extern MPI_Op UNIMPI_MINLOC;
extern MPI_Op UNIMPI_MAXLOC;

/* Vtable structure - Environment functions first */
typedef struct {
    /* Environment Management */
    int (*init)(int *argc, char ***argv);
    int (*finalize)(void);
    int (*initialized)(int *flag);
    int (*finalized)(int *flag);
    int (*abort)(MPI_Comm comm, int errorcode);
    int (*get_processor_name)(char *name, int *resultlen);
    int (*get_version)(int *version, int *subversion);
    int (*get_library_version)(char *version, int *resultlen);
    double (*wtime)(void);
    double (*wtick)(void);
    int (*barrier)(MPI_Comm comm);

    /* Point-to-Point */
    int (*send)(const void *buf, int count, MPI_Datatype datatype,
                int dest, int tag, MPI_Comm comm);
    int (*recv)(void *buf, int count, MPI_Datatype datatype,
                int source, int tag, MPI_Comm comm, MPI_Status *status);
    int (*isend)(const void *buf, int count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*irecv)(void *buf, int count, MPI_Datatype datatype,
                 int source, int tag, MPI_Comm comm, MPI_Request *request);
    int (*ibsend)(const void *buf, int count, MPI_Datatype datatype,
                  int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*irsend)(const void *buf, int count, MPI_Datatype datatype,
                  int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*issend)(const void *buf, int count, MPI_Datatype datatype,
                  int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*wait)(MPI_Request *request, MPI_Status *status);
    int (*waitall)(int count, MPI_Request *array_of_requests,
                   MPI_Status *array_of_statuses);
    int (*sendrecv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    int dest, int sendtag,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int source, int recvtag,
                    MPI_Comm comm, MPI_Status *status);
    int (*sendrecv_replace)(void *buf, int count, MPI_Datatype datatype,
                            int dest, int sendtag, int source, int recvtag,
                            MPI_Comm comm, MPI_Status *status);

    /* Synchronous, Buffered, and Ready sends */
    int (*ssend)(const void *buf, int count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm);
    int (*bsend)(const void *buf, int count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm);
    int (*rsend)(const void *buf, int count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm);
    int (*ssend_init)(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*bsend_init)(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*rsend_init)(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*buffer_attach)(void *buffer, int size);
    int (*buffer_detach)(void *buffer_addr, int *size);

    /* Nonblocking test and wait variants */
    int (*test)(MPI_Request *request, int *flag, MPI_Status *status);
    int (*testany)(int count, MPI_Request *array_of_requests, int *index,
                   int *flag, MPI_Status *status);
    int (*testsome)(int incount, MPI_Request *array_of_requests, int *outcount,
                    int *array_of_indices, MPI_Status *array_of_statuses);
    int (*testall)(int count, MPI_Request *array_of_requests, int *flag,
                   MPI_Status *array_of_statuses);
    int (*waitany)(int count, MPI_Request *array_of_requests, int *index,
                   MPI_Status *status);
    int (*waitsome)(int incount, MPI_Request *array_of_requests, int *outcount,
                    int *array_of_indices, MPI_Status *array_of_statuses);

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 matched_probe */
    int (*mprobe)(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status);
    int (*improbe)(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status);
    int (*mrecv)(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);
    int (*imrecv)(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);
#endif

    /* Message probing */
    int (*probe)(int source, int tag, MPI_Comm comm, MPI_Status *status);
    int (*iprobe)(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);

    /* Persistent communication */
    int (*send_init)(const void *buf, int count, MPI_Datatype datatype,
                     int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*recv_init)(void *buf, int count, MPI_Datatype datatype,
                     int source, int tag, MPI_Comm comm, MPI_Request *request);
    int (*start)(MPI_Request *request);
    int (*startall)(int count, MPI_Request *array_of_requests);
    int (*request_free)(MPI_Request *request);
    int (*request_get_status)(MPI_Request request, int *flag, MPI_Status *status);
    int (*grequest_start)(MPI_Grequest_query_function *query_fn,
                          MPI_Grequest_free_function *free_fn,
                          MPI_Grequest_cancel_function *cancel_fn,
                          void *extra_state, MPI_Request *request);
    int (*grequest_complete)(MPI_Request request);

    /* Cancel and status query */
    int (*cancel)(MPI_Request *request);
    int (*test_cancelled)(const MPI_Status *status, int *flag);
    int (*get_count)(const MPI_Status *status, MPI_Datatype datatype, int *count);
    int (*get_elements)(const MPI_Status *status, MPI_Datatype datatype, int *count);

    /* Collective */
    int (*bcast)(void *buffer, int count, MPI_Datatype datatype,
                 int root, MPI_Comm comm);
    int (*reduce)(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
    int (*reduce_local)(const void *inbuf, void *inoutbuf, int count,
                        MPI_Datatype datatype, MPI_Op op);
    int (*allreduce)(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*gather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm);
    int (*allgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     MPI_Comm comm);
    int (*scatter)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPI_Comm comm);

    /* Variable-length collectives */
    int (*gatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                   int root, MPI_Comm comm);
    int (*allgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                      MPI_Comm comm);
    int (*scatterv)(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm);
    int (*alltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
    int (*alltoallv)(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype,
                     void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                     MPI_Comm comm);
    int (*alltoallw)(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes,
                     void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
                     MPI_Comm comm);

    /* Reduce-scatter and scan */
    int (*reduce_scatter)(const void *sendbuf, void *recvbuf, const int *recvcounts,
                          MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*reduce_scatter_block)(const void *sendbuf, void *recvbuf, int recvcount,
                                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*scan)(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*exscan)(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 nonblocking_collectives */
    int (*ibarrier)(MPI_Comm comm, MPI_Request *request);
    int (*ibcast)(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
    int (*igather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPI_Comm comm, MPI_Request *request);
    int (*igatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                    int root, MPI_Comm comm, MPI_Request *request);
    int (*iscatter)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm, MPI_Request *request);
    int (*iscatterv)(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     int root, MPI_Comm comm, MPI_Request *request);
    int (*iallgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                      MPI_Comm comm, MPI_Request *request);
    int (*iallgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                       MPI_Comm comm, MPI_Request *request);
    int (*ialltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                     MPI_Comm comm, MPI_Request *request);
    int (*ialltoallv)(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype,
                      void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
                      MPI_Comm comm, MPI_Request *request);
    int (*ialltoallw)(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes,
                      void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
                      MPI_Comm comm, MPI_Request *request);
    int (*ireduce)(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
    int (*iallreduce)(const void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
    int (*ireduce_scatter)(const void *sendbuf, void *recvbuf, const int *recvcounts,
                           MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
    int (*ireduce_scatter_block)(const void *sendbuf, void *recvbuf, int recvcount,
                                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
    int (*iscan)(const void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
    int (*iexscan)(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
#endif

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 neighbor_collectives */
    int (*neighbor_allgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm);
    int (*neighbor_allgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                               MPI_Comm comm);
    int (*neighbor_alltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                             MPI_Comm comm);
    int (*neighbor_alltoallv)(const void *sendbuf, const int *sendcounts, const MPI_Aint *sdispls,
                              const MPI_Datatype *sendtypes, void *recvbuf,
                              const int *recvcounts, const MPI_Aint *rdispls,
                              const MPI_Datatype *recvtypes, MPI_Comm comm);
    int (*neighbor_alltoallw)(const void *sendbuf, const int *sendcounts, const MPI_Aint *sdispls,
                              const MPI_Datatype *sendtypes, void *recvbuf,
                              const int *recvcounts, const MPI_Aint *rdispls,
                              const MPI_Datatype *recvtypes, MPI_Comm comm);
    int (*ineighbor_allgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                               MPI_Comm comm, MPI_Request *request);
    int (*ineighbor_allgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                                MPI_Comm comm, MPI_Request *request);
    int (*ineighbor_alltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                              void *recvbuf, int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm, MPI_Request *request);
    int (*ineighbor_alltoallv)(const void *sendbuf, const int *sendcounts, const MPI_Aint *sdispls,
                               const MPI_Datatype *sendtypes, void *recvbuf,
                               const int *recvcounts, const MPI_Aint *rdispls,
                               const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request);
    int (*ineighbor_alltoallw)(const void *sendbuf, const int *sendcounts, const MPI_Aint *sdispls,
                               const MPI_Datatype *sendtypes, void *recvbuf,
                               const int *recvcounts, const MPI_Aint *rdispls,
                               const MPI_Datatype *recvtypes, MPI_Comm comm, MPI_Request *request);
#endif

    /* Group operations */
    int (*group_size)(MPI_Group group, int *size);
    int (*group_rank)(MPI_Group group, int *rank);
    int (*group_translate_ranks)(MPI_Group group1, int n, const int *ranks1, MPI_Group group2, int *ranks2);
    int (*group_compare)(MPI_Group group1, MPI_Group group2, int *result);
    int (*group_union)(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
    int (*group_intersection)(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
    int (*group_difference)(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
    int (*group_incl)(MPI_Group group, int n, const int *ranks, MPI_Group *newgroup);
    int (*group_excl)(MPI_Group group, int n, const int *ranks, MPI_Group *newgroup);
    int (*group_range_incl)(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
    int (*group_range_excl)(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
    int (*group_free)(MPI_Group *group);

    /* Communicator extended operations */
    int (*comm_size)(MPI_Comm comm, int *size);
    int (*comm_rank)(MPI_Comm comm, int *rank);
    int (*comm_dup)(MPI_Comm comm, MPI_Comm *newcomm);
    int (*comm_split)(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
    int (*comm_free)(MPI_Comm *comm);
    int (*comm_create)(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
    int (*comm_group)(MPI_Comm comm, MPI_Group *group);
    int (*comm_compare)(MPI_Comm comm1, MPI_Comm comm2, int *result);
    int (*comm_set_name)(MPI_Comm comm, const char *comm_name);
    int (*comm_get_name)(MPI_Comm comm, char *comm_name, int *resultlen);
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 comm_3x */
    int (*comm_dup_with_info)(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);
    int (*comm_split_type)(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm);
    int (*comm_create_group)(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm);
    int (*comm_get_info)(MPI_Comm comm, MPI_Info *info_used);
    int (*comm_set_info)(MPI_Comm comm, MPI_Info info);
    int (*comm_idup)(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request);
#endif

    /* Intercommunicator Operations (MPI-2.2) */
    int (*intercomm_create)(MPI_Comm local_comm, int local_leader,
                            MPI_Comm peer_comm, int remote_leader, int tag,
                            MPI_Comm *newintercomm);
    int (*intercomm_merge)(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
    int (*comm_remote_size)(MPI_Comm comm, int *size);
    int (*comm_remote_group)(MPI_Comm comm, MPI_Comm *group);
    int (*comm_test_inter)(MPI_Comm comm, int *flag);

    /* Process Topologies - Cartesian */
    int (*cart_create)(MPI_Comm comm_old, int ndims, const int *dims,
                      const int *periods, int reorder, MPI_Comm *comm_cart);
    int (*dims_create)(int nnodes, int ndims, int *dims);
    int (*cartdim_get)(MPI_Comm comm, int *ndims);
    int (*cart_get)(MPI_Comm comm, int maxdims, int *dims,
                    int *periods, int *coords);
    int (*cart_rank)(MPI_Comm comm, const int *coords, int *rank);
    int (*cart_coords)(MPI_Comm comm, int rank, int maxdims, int *coords);
    int (*cart_shift)(MPI_Comm comm, int direction, int disp,
                      int *rank_source, int *rank_dest);
    int (*cart_sub)(MPI_Comm comm, const int *remain_dims, MPI_Comm *newcomm);
    int (*cart_map)(MPI_Comm comm, int ndims, const int *dims,
                    const int *periods, int *newrank);

    /* Process Topologies - Graph */
    int (*graph_create)(MPI_Comm comm_old, int nnodes, const int *index,
                       const int *edges, int reorder, MPI_Comm *comm_graph);
    int (*graphdims_get)(MPI_Comm comm, int *nnodes, int *nedges);
    int (*graph_get)(MPI_Comm comm, int maxindex, int maxedges,
                     int *index, int *edges);
    int (*graph_neighbors_count)(MPI_Comm comm, int rank, int *nneighbors);
    int (*graph_neighbors)(MPI_Comm comm, int rank, int maxneighbors, int *neighbors);
    int (*graph_map)(MPI_Comm comm, int nnodes, const int *index,
                     const int *edges, int *newrank);

    /* Process Topologies - Distributed Graph */
    int (*dist_graph_create)(MPI_Comm comm_old, int n, const int *sources,
                             const int *degrees, const int *destinations,
                             const int *weights, MPI_Info info, int reorder,
                             MPI_Comm *comm_dist_graph);
    int (*dist_graph_create_adjacent)(MPI_Comm comm_old, int indegree,
                                      const int *sources, const int *sourceweights,
                                      int outdegree, const int *destinations,
                                      const int *destweights, MPI_Info info,
                                      int reorder, MPI_Comm *comm_dist_graph);
    int (*dist_graph_neighbors_count)(MPI_Comm comm, int *indegree,
                                      int *outdegree, int *weighted);
    int (*dist_graph_neighbors)(MPI_Comm comm, int maxindegree, int *sources,
                                int *sourceweights, int maxoutdegree,
                                int *destinations, int *destweights);

    /* Topology Testing */
    int (*topo_test)(MPI_Comm comm, int *status);

    /* RMA/One-Sided - Window creation */
    int (*win_create)(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 win_alloc_shared */
    int (*win_allocate)(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
    int (*win_allocate_shared)(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
    int (*win_create_dynamic)(MPI_Info info, MPI_Comm comm, MPI_Win *win);
#endif
    int (*win_free)(MPI_Win *win);
    int (*win_set_name)(MPI_Win win, const char *win_name);
    int (*win_get_name)(MPI_Win win, char *win_name, int *resultlen);

    /* RMA Operations */
    int (*put)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
               int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
    int (*get)(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
               int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
    int (*accumulate)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                      int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                      MPI_Op op, MPI_Win win);
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 rma_atomics */
    int (*get_accumulate)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                          void *result_addr, int result_count, MPI_Datatype result_datatype,
                          int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                          MPI_Op op, MPI_Win win);
    int (*fetch_and_op)(const void *origin_addr, void *result_addr, MPI_Datatype datatype,
                        int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win);
    int (*compare_and_swap)(const void *origin_addr, const void *compare_addr, void *result_addr,
                            MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win);
    int (*rput)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                MPI_Win win, MPI_Request *request);
    int (*rget)(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                MPI_Win win, MPI_Request *request);
    int (*raccumulate)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                       int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                       MPI_Op op, MPI_Win win, MPI_Request *request);
    int (*rget_accumulate)(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                           void *result_addr, int result_count, MPI_Datatype result_datatype,
                           int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype,
                           MPI_Op op, MPI_Win win, MPI_Request *request);
#endif

    /* RMA Synchronization */
    int (*win_fence)(int assert, MPI_Win win);
    int (*win_start)(MPI_Group group, int assert, MPI_Win win);
    int (*win_complete)(MPI_Win win);
    int (*win_post)(MPI_Group group, int assert, MPI_Win win);
    int (*win_wait)(MPI_Win win);
    int (*win_test)(MPI_Win win, int *flag);
    int (*win_lock)(int lock_type, int rank, int assert, MPI_Win win);
    int (*win_unlock)(int rank, MPI_Win win);
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 rma_sync_3x */
    int (*win_lock_all)(int assert, MPI_Win win);
    int (*win_unlock_all)(MPI_Win win);
    int (*win_flush)(int rank, MPI_Win win);
    int (*win_flush_all)(MPI_Win win);
    int (*win_flush_local)(int rank, MPI_Win win);
    int (*win_sync)(MPI_Win win);
#endif

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 win_dynamic */
    int (*win_attach)(MPI_Win win, void *base, MPI_Aint size);
    int (*win_detach)(MPI_Win win, void *base);
    int (*win_shared_query)(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr);
    int (*win_flush_local_all)(MPI_Win win);
    int (*win_get_info)(MPI_Win win, MPI_Info *info_used);
    int (*win_set_info)(MPI_Win win, MPI_Info info);
#endif

    /* Parallel I/O - File operations */
    int (*file_open)(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh);
    int (*file_close)(MPI_File *fh);
    int (*file_delete)(const char *filename, MPI_Info info);
    int (*file_set_size)(MPI_File fh, MPI_Offset size);
    int (*file_preallocate)(MPI_File fh, MPI_Offset size);
    int (*file_get_size)(MPI_File fh, MPI_Offset *size);
    int (*file_get_group)(MPI_File fh, MPI_Group *group);
    int (*file_get_amode)(MPI_File fh, int *amode);
    int (*file_get_info)(MPI_File fh, MPI_Info *info_used);
    int (*file_set_info)(MPI_File fh, MPI_Info info);
    int (*file_seek)(MPI_File fh, MPI_Offset offset, int whence);
    int (*file_get_position)(MPI_File fh, MPI_Offset *offset);
    int (*file_get_byte_offset)(MPI_File fh, MPI_Offset offset, MPI_Offset *disp);
    int (*file_seek_shared)(MPI_File fh, MPI_Offset offset, int whence);
    int (*file_get_position_shared)(MPI_File fh, MPI_Offset *offset);
    int (*file_get_type_extent)(MPI_File fh, MPI_Datatype datatype, MPI_Aint *extent);

    /* Parallel I/O - Read/Write */
    int (*file_read)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_read_all)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_write)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_write_all)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_read_at)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_read_at_all)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_write_at)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_write_at_all)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_read_shared)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_write_shared)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_read_ordered)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_write_ordered)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
    int (*file_read_all_begin)(MPI_File fh, void *buf, int count, MPI_Datatype datatype);
    int (*file_read_all_end)(MPI_File fh, void *buf, MPI_Status *status);
    int (*file_read_at_all_begin)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype);
    int (*file_read_at_all_end)(MPI_File fh, void *buf, MPI_Status *status);
    int (*file_read_ordered_begin)(MPI_File fh, void *buf, int count, MPI_Datatype datatype);
    int (*file_read_ordered_end)(MPI_File fh, void *buf, MPI_Status *status);
    int (*file_write_all_begin)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype);
    int (*file_write_all_end)(MPI_File fh, const void *buf, MPI_Status *status);
    int (*file_write_at_all_begin)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype);
    int (*file_write_at_all_end)(MPI_File fh, const void *buf, MPI_Status *status);
    int (*file_write_ordered_begin)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype);
    int (*file_write_ordered_end)(MPI_File fh, const void *buf, MPI_Status *status);

    /* Parallel I/O - Nonblocking */
    int (*file_iread)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
    int (*file_iwrite)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
    int (*file_iread_at)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
    int (*file_iwrite_at)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
    int (*file_iread_shared)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
    int (*file_iwrite_shared)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);

    /* Parallel I/O - Views */
    int (*file_set_view)(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char *datarep, MPI_Info info);
    int (*file_get_view)(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype, char *datarep);

    /* Dynamic Process - Spawn */
    int (*comm_spawn)(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
    int (*comm_spawn_multiple)(int count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
    int (*comm_accept)(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
    int (*comm_connect)(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
    int (*comm_disconnect)(MPI_Comm *comm);
    int (*comm_join)(int fd, MPI_Comm *intercomm);
    int (*comm_get_parent)(MPI_Comm *parent);

    /* Dynamic Process - Port and Name Service */
    int (*open_port)(MPI_Info info, char *port_name);
    int (*close_port)(const char *port_name);
    int (*publish_name)(const char *service_name, MPI_Info info, const char *port_name);
    int (*unpublish_name)(const char *service_name, MPI_Info info, const char *port_name);
    int (*lookup_name)(const char *service_name, MPI_Info info, char *port_name);

    /* Info operations */
    int (*info_create)(MPI_Info *info);
    int (*info_free)(MPI_Info *info);
    int (*info_set)(MPI_Info info, const char *key, const char *value);
    int (*info_get)(MPI_Info info, const char *key, int valuelen, char *value, int *flag);
    int (*info_delete)(MPI_Info info, const char *key);
    int (*info_get_nkeys)(MPI_Info info, int *nkeys);
    int (*info_get_nthkey)(MPI_Info info, int n, char *key);
    int (*info_dup)(MPI_Info info, MPI_Info *newinfo);
    int (*info_get_valuelen)(MPI_Info info, const char *key, int *valuelen, int *flag);

    /* Thread support */
    int (*init_thread)(int *argc, char ***argv, int required, int *provided);
    int (*query_thread)(int *provided);
    int (*is_thread_main)(int *flag);

    /* Memory */
    int (*alloc_mem)(MPI_Aint size, MPI_Info info, void *baseptr);
    int (*free_mem)(void *baseptr);

    /* Reduction operations */
    int (*op_create)(void (*user_fn)(void *, void *, int *, MPI_Datatype *), int commute, MPI_Op *op);
    int (*op_free)(MPI_Op *op);
    int (*op_commutative)(MPI_Op op, int *commute);

    /* Status manipulation */
    int (*status_set_elements)(MPI_Status *status, MPI_Datatype datatype, int count);
    int (*status_set_cancelled)(MPI_Status *status, int flag);
    int (*status_f2c)(const MPI_Fint *f_status, MPI_Status *c_status);
    int (*status_c2f)(const MPI_Status *c_status, MPI_Fint *f_status);

    /* Error handling */
    int (*errhandler_create)(void (*handler_fn)(MPI_Comm *, int *, ...), MPI_Errhandler *errhandler);
    int (*errhandler_free)(MPI_Errhandler *errhandler);
    int (*errhandler_set)(MPI_Comm comm, MPI_Errhandler errhandler);
    int (*errhandler_get)(MPI_Comm comm, MPI_Errhandler *errhandler);
    int (*comm_get_errhandler)(MPI_Comm comm, MPI_Errhandler *errhandler);
    int (*comm_set_errhandler)(MPI_Comm comm, MPI_Errhandler errhandler);
    int (*win_get_errhandler)(MPI_Win win, MPI_Errhandler *errhandler);
    int (*win_set_errhandler)(MPI_Win win, MPI_Errhandler errhandler);
    int (*comm_create_errhandler)(void (*handler_fn)(MPI_Comm *, int *, ...), MPI_Errhandler *errhandler);
    int (*comm_call_errhandler)(MPI_Comm comm, int errorcode);
    int (*win_create_errhandler)(void (*handler_fn)(MPI_Win *, int *, ...), MPI_Errhandler *errhandler);
    int (*win_create_keyval)(MPI_Win_copy_attr_function *copy_fn,
                             MPI_Win_delete_attr_function *delete_fn,
                             int *keyval, void *extra_state);
    int (*win_free_keyval)(int *keyval);
    int (*win_set_attr)(MPI_Win win, int win_keyval, void *attribute_val);
    int (*win_get_attr)(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
    int (*win_delete_attr)(MPI_Win win, int win_keyval);
    int (*comm_create_keyval)(MPI_Comm_copy_attr_function *copy_fn,
                              MPI_Comm_delete_attr_function *delete_fn,
                              int *keyval, void *extra_state);
    int (*comm_free_keyval)(int *keyval);
    int (*comm_set_attr)(MPI_Comm comm, int keyval, void *attribute_val);
    int (*comm_get_attr)(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
    int (*comm_delete_attr)(MPI_Comm comm, int keyval);
    int (*type_create_keyval)(MPI_Type_copy_attr_function *copy_fn,
                              MPI_Type_delete_attr_function *delete_fn,
                              int *keyval, void *extra_state);
    int (*type_free_keyval)(int *keyval);
    int (*type_set_attr)(MPI_Datatype type, int keyval, void *attribute_val);
    int (*type_get_attr)(MPI_Datatype type, int keyval, void *attribute_val, int *flag);
    int (*type_delete_attr)(MPI_Datatype type, int keyval);
    int (*keyval_create)(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn,
                         int *keyval, void *extra_state);
    int (*keyval_free)(int *keyval);
    int (*register_datarep)(const char *datarep,
                            MPI_Datarep_conversion_function *read_conversion_fn,
                            MPI_Datarep_conversion_function *write_conversion_fn,
                            MPI_Datarep_extent_function *dtype_file_extent_fn,
                            void *extra_state);
    int (*win_get_group)(MPI_Win win, MPI_Group *group);
    int (*win_call_errhandler)(MPI_Win win, int errorcode);
    int (*file_create_errhandler)(void (*handler_fn)(MPI_File *, int *, ...), MPI_Errhandler *errhandler);
    int (*file_set_atomicity)(MPI_File fh, int flag);
    int (*file_get_atomicity)(MPI_File fh, int *flag);
    int (*file_sync)(MPI_File fh);
    int (*file_call_errhandler)(MPI_File fh, int errorcode);
    int (*file_set_errhandler)(MPI_File fh, MPI_Errhandler errhandler);
    int (*file_get_errhandler)(MPI_File fh, MPI_Errhandler *errhandler);
    int (*add_error_class)(int *errorclass);
    int (*add_error_code)(int errorclass, int *errorcode);
    int (*add_error_string)(int errorcode, const char *string);
    int (*error_class)(int errorcode, int *errorclass);
    int (*error_string)(int errorcode, char *string, int *resultlen);
    int (*pcontrol)(const int level, ...);

    /* Attributes */
    int (*attr_put)(MPI_Comm comm, int keyval, void *attribute_val);
    int (*attr_get)(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
    int (*attr_delete)(MPI_Comm comm, int keyval);

    /* Datatypes - creation */
    int (*type_commit)(MPI_Datatype *datatype);
    int (*type_free)(MPI_Datatype *datatype);
    int (*type_contiguous)(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_vector)(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_hvector)(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_indexed)(int count, const int *array_of_blocklengths, const int *array_of_displacements,
                        MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_hindexed)(int count, const int *array_of_blocklengths, const MPI_Aint *array_of_displacements,
                         MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_indexed_block)(int count, int blocklength, const int *array_of_displacements,
                                     MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_subarray)(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                                const int *array_of_starts, int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_darray)(int size, int rank, int ndims, const int *array_of_gsizes,
                              const int *array_of_distribs, const int *array_of_dargs,
                              const int *array_of_psizes, int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_dup)(MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_resized)(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                               MPI_Datatype *newtype);
    int (*type_create_hvector)(int count, int blocklength, MPI_Aint stride,
                               MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_hindexed)(int count, const int *array_of_blocklengths,
                                const MPI_Aint *array_of_displacements,
                                MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_struct)(int count, const int *array_of_blocklengths,
                              const MPI_Aint *array_of_displacements,
                              const MPI_Datatype *array_of_types, MPI_Datatype *newtype);
    int (*type_struct)(int count, const int *array_of_blocklengths,
                       const MPI_Aint *array_of_displacements,
                       const MPI_Datatype *array_of_types, MPI_Datatype *newtype);
    int (*type_match_size)(MPI_Datatype typeclass, int size, MPI_Datatype *datatype);
    int (*type_create_f90_integer)(int range, MPI_Datatype *newtype);
    int (*type_create_f90_real)(int precision, int range, MPI_Datatype *newtype);
    int (*type_create_f90_complex)(int precision, int range, MPI_Datatype *newtype);

    /* Address arithmetic */
    int (*get_address)(const void *location, MPI_Aint *address);
    int (*address)(void *location, MPI_Aint *address);

    /* Datatypes - query */
    int (*type_get_extent)(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
    int (*type_get_true_extent)(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
    int (*type_get_envelope)(MPI_Datatype datatype, int *num_integers,
                             int *num_addresses, int *num_datatypes,
                             int *combiner);
    int (*type_get_contents)(MPI_Datatype datatype, int max_integers,
                             int max_addresses, int max_datatypes,
                             int *array_of_integers, MPI_Aint *array_of_addresses,
                             MPI_Datatype *array_of_datatypes);
    int (*type_get_size)(MPI_Datatype datatype, int *size);
    int (*type_size)(MPI_Datatype datatype, int *size);
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 large_count */
    int (*type_size_x)(MPI_Datatype datatype, MPI_Count *size);
    int (*type_get_extent_x)(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
    int (*type_get_true_extent_x)(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
    int (*type_create_hindexed_block)(int count, int blocklength,
                                      const MPI_Aint *array_of_displacements,
                                      MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*get_elements_x)(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
    int (*status_set_elements_x)(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
#endif
    int (*type_get_name)(MPI_Datatype datatype, char *type_name, int *resultlen);
    int (*type_set_name)(MPI_Datatype datatype, const char *type_name);
    int (*type_extent)(MPI_Datatype datatype, MPI_Aint *extent);
    int (*type_lb)(MPI_Datatype datatype, MPI_Aint *displacement);
    int (*type_ub)(MPI_Datatype datatype, MPI_Aint *displacement);

    /* Pack/Unpack */
    int (*pack)(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf,
                int outsize, int *position, MPI_Comm comm);
    int (*unpack)(const void *inbuf, int insize, int *position, void *outbuf,
                  int outcount, MPI_Datatype datatype, MPI_Comm comm);
    int (*pack_size)(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);
    int (*pack_external)(const char *datarep, const void *inbuf, int incount,
                         MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position);
    int (*unpack_external)(const char *datarep, const void *inbuf, MPI_Aint insize,
                           MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype);
    int (*pack_external_size)(const char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size);

} unimpi_vtable_t;

/* Global vtable instance */
extern unimpi_vtable_t unimpi;

/* Function count for sanity checks */
#define UNIMPI_VTABLE_COUNT \
    (sizeof(unimpi_vtable_t) / sizeof(void*))

/* Vtable initialization and cleanup */
int unimpi_vtable_init(unimpi_lib_handle_t handle);
void unimpi_vtable_cleanup(void);

/* Get current backend type */
unimpi_backend_type_t unimpi_get_backend_type(void);

#endif /* UNIMPI_VTABLE_H */
