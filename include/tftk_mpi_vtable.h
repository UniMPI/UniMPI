#ifndef TFTK_MPI_VTABLE_H
#define TFTK_MPI_VTABLE_H

#include <stddef.h>

/* MPI opaque types - forward declarations */
typedef int MPI_Comm;
typedef int MPI_Datatype;
typedef int MPI_Op;
typedef int MPI_Group;
typedef int MPI_Request;
typedef int MPI_Status;
typedef int MPI_Info;
typedef int MPI_Win;
typedef int MPI_File;
typedef int MPI_Errhandler;
typedef int MPI_Message;
typedef long MPI_Aint;
typedef long long MPI_Offset;

/* MPI predefined constants (will be resolved at runtime) */
extern MPI_Comm TFTK_MPI_COMM_WORLD;
extern MPI_Comm TFTK_MPI_COMM_SELF;

/* Status struct */
struct TFTK_MPI_Status {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int count;
    int cancelled;
};
typedef struct TFTK_MPI_Status TFTK_MPI_Status;

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
                int source, int tag, MPI_Comm comm, TFTK_MPI_Status *status);
    int (*isend)(const void *buf, int count, MPI_Datatype datatype,
                 int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*irecv)(void *buf, int count, MPI_Datatype datatype,
                 int source, int tag, MPI_Comm comm, MPI_Request *request);
    int (*wait)(MPI_Request *request, TFTK_MPI_Status *status);
    int (*waitall)(int count, MPI_Request *array_of_requests,
                   TFTK_MPI_Status *array_of_statuses);
    int (*sendrecv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    int dest, int sendtag,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int source, int recvtag,
                    MPI_Comm comm, TFTK_MPI_Status *status);

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
    int (*test)(MPI_Request *request, int *flag, TFTK_MPI_Status *status);
    int (*testany)(int count, MPI_Request *array_of_requests, int *index,
                   int *flag, TFTK_MPI_Status *status);
    int (*testsome)(int incount, MPI_Request *array_of_requests, int *outcount,
                    int *array_of_indices, TFTK_MPI_Status *array_of_statuses);
    int (*testall)(int count, MPI_Request *array_of_requests, int *flag,
                   TFTK_MPI_Status *array_of_statuses);
    int (*waitany)(int count, MPI_Request *array_of_requests, int *index,
                   TFTK_MPI_Status *status);
    int (*waitsome)(int incount, MPI_Request *array_of_requests, int *outcount,
                    int *array_of_indices, TFTK_MPI_Status *array_of_statuses);

    /* Message probing */
    int (*probe)(int source, int tag, MPI_Comm comm, TFTK_MPI_Status *status);
    int (*iprobe)(int source, int tag, MPI_Comm comm, int *flag, TFTK_MPI_Status *status);

    /* Persistent communication */
    int (*send_init)(const void *buf, int count, MPI_Datatype datatype,
                     int dest, int tag, MPI_Comm comm, MPI_Request *request);
    int (*recv_init)(void *buf, int count, MPI_Datatype datatype,
                     int source, int tag, MPI_Comm comm, MPI_Request *request);
    int (*start)(MPI_Request *request);
    int (*startall)(int count, MPI_Request *array_of_requests);
    int (*request_free)(MPI_Request *request);

    /* Cancel and status query */
    int (*cancel)(MPI_Request *request);
    int (*test_cancelled)(const TFTK_MPI_Status *status, int *flag);
    int (*get_count)(const TFTK_MPI_Status *status, MPI_Datatype datatype, int *count);
    int (*get_elements)(const TFTK_MPI_Status *status, MPI_Datatype datatype, int *count);

    /* Collective */
    int (*bcast)(void *buffer, int count, MPI_Datatype datatype,
                 int root, MPI_Comm comm);
    int (*reduce)(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
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

    /* Reduce-scatter and scan */
    int (*reduce_scatter)(const void *sendbuf, void *recvbuf, const int *recvcounts,
                          MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*reduce_scatter_block)(const void *sendbuf, void *recvbuf, int recvcount,
                                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*scan)(const void *sendbuf, void *recvbuf, int count,
                MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
    int (*exscan)(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

    /* Communicator */
    int (*comm_size)(MPI_Comm comm, int *size);
    int (*comm_rank)(MPI_Comm comm, int *rank);
    int (*comm_dup)(MPI_Comm comm, MPI_Comm *newcomm);
    int (*comm_split)(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
    int (*comm_free)(MPI_Comm *comm);

    /* Datatypes - creation */
    int (*type_commit)(MPI_Datatype *datatype);
    int (*type_free)(MPI_Datatype *datatype);
    int (*type_contiguous)(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_vector)(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_indexed)(int count, const int *array_of_blocklengths, const int *array_of_displacements,
                        MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_indexed_block)(int count, int blocklength, const int *array_of_displacements,
                                     MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_create_subarray)(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                                const int *array_of_starts, int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
    int (*type_dup)(MPI_Datatype oldtype, MPI_Datatype *newtype);

    /* Datatypes - query */
    int (*type_get_extent)(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
    int (*type_get_size)(MPI_Datatype datatype, int *size);
    int (*type_get_name)(MPI_Datatype datatype, char *type_name, int *resultlen);
    int (*type_set_name)(MPI_Datatype datatype, const char *type_name);

    /* Pack/Unpack */
    int (*pack)(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf,
                int outsize, int *position, MPI_Comm comm);
    int (*unpack)(const void *inbuf, int insize, int *position, void *outbuf,
                  int outcount, MPI_Datatype datatype, MPI_Comm comm);
    int (*pack_size)(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);

} tftk_mpi_vtable_t;

/* Global vtable instance */
extern tftk_mpi_vtable_t tftk_mpi;

/* Function count for sanity checks */
#define TFTK_MPI_VTABLE_COUNT \
    (sizeof(tftk_mpi_vtable_t) / sizeof(void*))

#endif /* TFTK_MPI_VTABLE_H */
