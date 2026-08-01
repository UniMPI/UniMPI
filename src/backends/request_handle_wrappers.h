/* Single-request / message ABI adapters for integer-handle MPI backends.
 *
 * UniMPI facade handles are intptr_t.  MPICH, Intel MPI, and MS-MPI use native
 * int for Comm/Datatype/Op/Win/Request/Message and struct ADIOI_FileD * for
 * File.  Adapters convert every by-value handle at the call boundary, use a
 * local native request/message int for pointer arguments, and store a
 * full-width facade value only when the native call defines the output.
 *
 * Array completion/start paths remain in request_array_wrappers.h.
 * Open MPI keeps direct pointer-handle assignment in its backend.
 */
#ifndef UNIMPI_REQUEST_HANDLE_WRAPPERS_H
#define UNIMPI_REQUEST_HANDLE_WRAPPERS_H

#include "unimpi_platform.h"
#include "unimpi_vtable.h"

/* Bind every enabled single-request and array request vtable entry for an
 * integer-handle backend.  Missing symbols leave the corresponding unimpi
 * slot NULL.  Integer-backend Ialltoallw is always left NULL.
 */
void unimpi_bind_integer_request_apis(unimpi_lib_handle_t handle);

/* Canonical facade conversion matching C conversion of native int to intptr_t. */
MPI_Request unimpi_request_from_native(int native);
int unimpi_request_to_native(MPI_Request facade);
MPI_Message unimpi_message_from_native(int native);
int unimpi_message_to_native(MPI_Message facade);

/* Single-request wrappers installed by unimpi_bind_integer_request_apis. */
int unimpi_wrap_isend(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_irecv(void *buf, int count, MPI_Datatype datatype,
                      int source, int tag, MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_wait(MPI_Request *request, MPI_Status *status);
int unimpi_wrap_test(MPI_Request *request, int *flag, MPI_Status *status);
int unimpi_wrap_ssend_init(const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm,
                           MPI_Request *request);
int unimpi_wrap_bsend_init(const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm,
                           MPI_Request *request);
int unimpi_wrap_rsend_init(const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm,
                           MPI_Request *request);
int unimpi_wrap_send_init(const void *buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm,
                          MPI_Request *request);
int unimpi_wrap_recv_init(void *buf, int count, MPI_Datatype datatype,
                          int source, int tag, MPI_Comm comm,
                          MPI_Request *request);
int unimpi_wrap_start(MPI_Request *request);
int unimpi_wrap_request_free(MPI_Request *request);
int unimpi_wrap_cancel(MPI_Request *request);
int unimpi_wrap_imrecv(void *buf, int count, MPI_Datatype datatype,
                       MPI_Message *message, MPI_Request *request);
int unimpi_wrap_mprobe(int source, int tag, MPI_Comm comm,
                       MPI_Message *message, MPI_Status *status);
int unimpi_wrap_improbe(int source, int tag, MPI_Comm comm, int *flag,
                        MPI_Message *message, MPI_Status *status);
int unimpi_wrap_mrecv(void *buf, int count, MPI_Datatype datatype,
                      MPI_Message *message, MPI_Status *status);

int unimpi_wrap_ibarrier(MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_ibcast(void *buffer, int count, MPI_Datatype datatype,
                       int root, MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_igather(const void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf, int recvcount,
                        MPI_Datatype recvtype, int root, MPI_Comm comm,
                        MPI_Request *request);
int unimpi_wrap_igatherv(const void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         const int *recvcounts, const int *displs,
                         MPI_Datatype recvtype, int root, MPI_Comm comm,
                         MPI_Request *request);
int unimpi_wrap_iscatter(const void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                         MPI_Datatype recvtype, int root, MPI_Comm comm,
                         MPI_Request *request);
int unimpi_wrap_iscatterv(const void *sendbuf, const int *sendcounts,
                          const int *displs, MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                          int root, MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_iallgather(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request *request);
int unimpi_wrap_iallgatherv(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            const int *recvcounts, const int *displs,
                            MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request *request);
int unimpi_wrap_ialltoall(const void *sendbuf, int sendcount,
                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                          MPI_Datatype recvtype, MPI_Comm comm,
                          MPI_Request *request);
int unimpi_wrap_ialltoallv(const void *sendbuf, const int *sendcounts,
                           const int *sdispls, MPI_Datatype sendtype,
                           void *recvbuf, const int *recvcounts,
                           const int *rdispls, MPI_Datatype recvtype,
                           MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_ireduce(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, int root,
                        MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_iallreduce(const void *sendbuf, void *recvbuf, int count,
                           MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                           MPI_Request *request);
int unimpi_wrap_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                const int *recvcounts, MPI_Datatype datatype,
                                MPI_Op op, MPI_Comm comm,
                                MPI_Request *request);
int unimpi_wrap_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                      int recvcount, MPI_Datatype datatype,
                                      MPI_Op op, MPI_Comm comm,
                                      MPI_Request *request);
int unimpi_wrap_iscan(const void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Request *request);
int unimpi_wrap_iexscan(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                        MPI_Request *request);

int unimpi_wrap_rput(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Win win,
                     MPI_Request *request);
int unimpi_wrap_rget(void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Win win,
                     MPI_Request *request);
int unimpi_wrap_raccumulate(const void *origin_addr, int origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            MPI_Aint target_disp, int target_count,
                            MPI_Datatype target_datatype, MPI_Op op,
                            MPI_Win win, MPI_Request *request);
int unimpi_wrap_rget_accumulate(const void *origin_addr, int origin_count,
                                MPI_Datatype origin_datatype,
                                void *result_addr, int result_count,
                                MPI_Datatype result_datatype, int target_rank,
                                MPI_Aint target_disp, int target_count,
                                MPI_Datatype target_datatype, MPI_Op op,
                                MPI_Win win, MPI_Request *request);

int unimpi_wrap_file_iread(MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Request *request);
int unimpi_wrap_file_iwrite(MPI_File fh, const void *buf, int count,
                            MPI_Datatype datatype, MPI_Request *request);
int unimpi_wrap_file_iread_at(MPI_File fh, MPI_Offset offset, void *buf,
                              int count, MPI_Datatype datatype,
                              MPI_Request *request);
int unimpi_wrap_file_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf,
                               int count, MPI_Datatype datatype,
                               MPI_Request *request);


#endif /* UNIMPI_REQUEST_HANDLE_WRAPPERS_H */
