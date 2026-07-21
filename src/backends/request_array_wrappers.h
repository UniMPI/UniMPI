/* src/backends/request_array_wrappers.h
 * Wrapper functions for MPI_Request array operations.
 *
 * Intel MPI and MPICH use typedef int MPI_Request (4 bytes), while UNIMPI
 * uses typedef intptr_t MPI_Request (8 bytes) to also support OpenMPI's
 * pointer-based requests.
 *
 * When passing arrays of MPI_Request to the native backend, the element
 * stride differs (4 vs 8 bytes), causing memory corruption and crashes
 * (e.g. MPI_Waitall with "Invalid MPI_Request").
 *
 * These wrappers convert between the 8-byte and 4-byte representations
 * for all MPI functions that accept request arrays.
 */

#ifndef UNIMPI_REQUEST_ARRAY_WRAPPERS_H
#define UNIMPI_REQUEST_ARRAY_WRAPPERS_H

#include "unimpi_vtable.h"

/* Setter functions — each backend calls the relevant setters during init */
void unimpi_wrapper_set_waitall(int (*fn)(int, int*, MPI_Status*));
void unimpi_wrapper_set_testany(int (*fn)(int, int*, int*, int*, MPI_Status*));
void unimpi_wrapper_set_testsome(int (*fn)(int, int*, int*, int*, MPI_Status*));
void unimpi_wrapper_set_testall(int (*fn)(int, int*, int*, MPI_Status*));
void unimpi_wrapper_set_waitany(int (*fn)(int, int*, int*, MPI_Status*));
void unimpi_wrapper_set_waitsome(int (*fn)(int, int*, int*, int*, MPI_Status*));
void unimpi_wrapper_set_startall(int (*fn)(int, int*));

/* Wrapper functions — assign these to the vtable */
int unimpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                         MPI_Status *array_of_statuses);
int unimpi_wrap_testany(int count, MPI_Request *array_of_requests,
                         int *index, int *flag, MPI_Status *status);
int unimpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                          int *outcount, int *array_of_indices,
                          MPI_Status *array_of_statuses);
int unimpi_wrap_testall(int count, MPI_Request *array_of_requests,
                         int *flag, MPI_Status *array_of_statuses);
int unimpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                         int *index, MPI_Status *status);
int unimpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                          int *outcount, int *array_of_indices,
                          MPI_Status *array_of_statuses);
int unimpi_wrap_startall(int count, MPI_Request *array_of_requests);

#endif /* UNIMPI_REQUEST_ARRAY_WRAPPERS_H */
