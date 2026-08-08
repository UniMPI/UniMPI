/* src/backends/mpich_wrappers.h
 * MPICH-specific wrapper functions for MPI_Request array operations.
 *
 * MPICH uses 4-byte integer MPI_Request handles while UnimPI uses 8-byte
 * intptr_t handles. These wrappers convert between the representations.
 */

#ifndef UNIMPI_MPICH_WRAPPERS_H
#define UNIMPI_MPICH_WRAPPERS_H

#include "unimpi_vtable.h"

/* Backend function pointers - set directly by mpich.c during init */
extern int (*mpich_waitall)(int, int*, MPI_Status*);
extern int (*mpich_testany)(int, int*, int*, int*, MPI_Status*);
extern int (*mpich_testsome)(int, int*, int*, int*, MPI_Status*);
extern int (*mpich_testall)(int, int*, int*, MPI_Status*);
extern int (*mpich_waitany)(int, int*, int*, MPI_Status*);
extern int (*mpich_waitsome)(int, int*, int*, int*, MPI_Status*);
extern int (*mpich_startall)(int, int*);
extern int (*mpich_alltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm);
extern int (*mpich_ialltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm,
    MPI_Request *);

/* Wrapper entry points for MPICH vtable */
int mpich_wrap_waitall(int count, MPI_Request *array_of_requests,
                        MPI_Status *array_of_statuses);
int mpich_wrap_testany(int count, MPI_Request *array_of_requests,
                        int *index, int *flag, MPI_Status *status);
int mpich_wrap_testsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses);
int mpich_wrap_testall(int count, MPI_Request *array_of_requests,
                        int *flag, MPI_Status *array_of_statuses);
int mpich_wrap_waitany(int count, MPI_Request *array_of_requests,
                        int *index, MPI_Status *status);
int mpich_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses);
int mpich_wrap_startall(int count, MPI_Request *array_of_requests);

/* MPICH datatype array wrappers */
int mpich_wrap_alltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm);
int mpich_wrap_ialltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm, MPI_Request *request);

#endif /* UNIMPI_MPICH_WRAPPERS_H */
