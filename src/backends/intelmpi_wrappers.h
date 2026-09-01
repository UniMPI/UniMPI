/* src/backends/intelmpi_wrappers.h
 * INTELMPI-specific wrapper functions for MPI_Request array operations.
 *
 * INTELMPI uses 4-byte integer MPI_Request handles while UnimPI uses 8-byte
 * intptr_t handles. These wrappers convert between the representations.
 */

#ifndef UNIMPI_INTELMPI_WRAPPERS_H
#define UNIMPI_INTELMPI_WRAPPERS_H

#include "unimpi_vtable.h"

/* Backend function pointers - set directly by intelmpi.c during init */
extern int (*intelmpi_waitall)(int, int*, MPI_Status*);
extern int (*intelmpi_testany)(int, int*, int*, int*, MPI_Status*);
extern int (*intelmpi_testsome)(int, int*, int*, int*, MPI_Status*);
extern int (*intelmpi_testall)(int, int*, int*, MPI_Status*);
extern int (*intelmpi_waitany)(int, int*, int*, MPI_Status*);
extern int (*intelmpi_waitsome)(int, int*, int*, int*, MPI_Status*);
extern int (*intelmpi_startall)(int, int*);
extern int (*intelmpi_alltoallw)(const void *, const int *, const int *, const int *, void *, const int *, const int *, const int *, MPI_Comm);
extern int (*intelmpi_ialltoallw)(const void *, const int *, const int *, const int *, void *, const int *, const int *, const int *, MPI_Comm, MPI_Request *);
extern int (*intelmpi_type_create_struct)(int, const int *, const MPI_Aint *,
    const int *, MPI_Datatype *);
extern int (*intelmpi_type_struct)(int, const int *, const MPI_Aint *,
    const int *, MPI_Datatype *);
extern int (*intelmpi_type_get_contents)(MPI_Datatype, int, int, int,
    int *, MPI_Aint *, MPI_Datatype *);
extern int (*intelmpi_comm_spawn_multiple)(int, char *[], char **[],
    const int[], const int[], int, MPI_Comm, MPI_Comm *, int[]);

/* Wrapper entry points for INTELMPI vtable */
int intelmpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                        MPI_Status *array_of_statuses);
int intelmpi_wrap_testany(int count, MPI_Request *array_of_requests,
                        int *index, int *flag, MPI_Status *status);
int intelmpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses);
int intelmpi_wrap_testall(int count, MPI_Request *array_of_requests,
                        int *flag, MPI_Status *array_of_statuses);
int intelmpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                        int *index, MPI_Status *status);
int intelmpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses);
int intelmpi_wrap_startall(int count, MPI_Request *array_of_requests);

/* INTELMPI datatype array wrappers */
int intelmpi_wrap_alltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm);
int intelmpi_wrap_ialltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm, MPI_Request *request);
int intelmpi_wrap_type_create_struct(int count, const int *array_of_blocklengths,
    const MPI_Aint *array_of_displacements, const MPI_Datatype *array_of_types,
    MPI_Datatype *newtype);
int intelmpi_wrap_type_struct(int count, const int *array_of_blocklengths,
    const MPI_Aint *array_of_displacements, const MPI_Datatype *array_of_types,
    MPI_Datatype *newtype);
int intelmpi_wrap_type_get_contents(MPI_Datatype datatype, int max_integers,
    int max_addresses, int max_datatypes, int *array_of_integers,
    MPI_Aint *array_of_addresses, MPI_Datatype *array_of_datatypes);
int intelmpi_wrap_comm_spawn_multiple(int count, char *array_of_commands[],
    char **array_of_argv[], const int array_of_maxprocs[],
    const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm,
    int array_of_errcodes[]);

#endif /* UNIMPI_INTELMPI_WRAPPERS_H */
