/* Completion-array ABI adapters for runtime-selected MPI backends. */
#ifndef UNIMPI_REQUEST_ARRAY_WRAPPERS_H
#define UNIMPI_REQUEST_ARRAY_WRAPPERS_H

#include "unimpi_vtable.h"

/*
 * Open MPI public status layout (mpi.h struct ompi_status_public_t).
 * Native array entry points take this tag, not the UniMPI facade union.
 */
struct ompi_request_t;

struct ompi_status_public_t {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int _cancelled;
    size_t _ucount;
};

typedef int (UNIMPI_MPI_CALL *unimpi_native_error_class_fn)(int, int *);
/* Integer backends export struct MPI_Status * (incomplete here). */
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_waitall_fn)(
    int, int *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_any_fn)(
    int, int *, int *, int *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_some_fn)(
    int, int *, int *, int *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_testall_fn)(
    int, int *, int *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_waitany_fn)(
    int, int *, int *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_startall_fn)(int, int *);

/* Open MPI: MPI_Request is struct ompi_request_t *; arrays are pointer arrays.
 * Status arrays use struct ompi_status_public_t. */
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_waitall_fn)(
    int, struct ompi_request_t **, struct ompi_status_public_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_some_fn)(
    int, struct ompi_request_t **, int *, int *,
    struct ompi_status_public_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_testall_fn)(
    int, struct ompi_request_t **, int *, struct ompi_status_public_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_testany_fn)(
    int, struct ompi_request_t **, int *, int *,
    struct ompi_status_public_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_waitany_fn)(
    int, struct ompi_request_t **, int *, struct ompi_status_public_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_startall_fn)(
    int, struct ompi_request_t **);

/* Backend error-class query used to classify encoded error codes safely. */
void unimpi_wrapper_set_error_class(unimpi_native_error_class_fn fn);

/* Integer-handle backend function registration. */
void unimpi_wrapper_set_waitall(unimpi_native_legacy_waitall_fn fn);
void unimpi_wrapper_set_testany(unimpi_native_legacy_any_fn fn);
void unimpi_wrapper_set_testsome(unimpi_native_legacy_some_fn fn);
void unimpi_wrapper_set_testall(unimpi_native_legacy_testall_fn fn);
void unimpi_wrapper_set_waitany(unimpi_native_legacy_waitany_fn fn);
void unimpi_wrapper_set_waitsome(unimpi_native_legacy_some_fn fn);
void unimpi_wrapper_set_startall(unimpi_native_legacy_startall_fn fn);

/* Open MPI array function registration. */
void unimpi_wrapper_set_openmpi_waitall(
    unimpi_native_openmpi_waitall_fn fn);
void unimpi_wrapper_set_openmpi_testsome(
    unimpi_native_openmpi_some_fn fn);
void unimpi_wrapper_set_openmpi_testall(
    unimpi_native_openmpi_testall_fn fn);
void unimpi_wrapper_set_openmpi_waitsome(
    unimpi_native_openmpi_some_fn fn);
void unimpi_wrapper_set_openmpi_testany(
    unimpi_native_openmpi_testany_fn fn);
void unimpi_wrapper_set_openmpi_waitany(
    unimpi_native_openmpi_waitany_fn fn);
void unimpi_wrapper_set_openmpi_startall(
    unimpi_native_openmpi_startall_fn fn);

/* Integer-handle backend wrappers. */
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

/* Open MPI request/status array wrappers. */
int unimpi_wrap_openmpi_waitall(int count,
                                MPI_Request *array_of_requests,
                                MPI_Status *array_of_statuses);
int unimpi_wrap_openmpi_testsome(int incount,
                                 MPI_Request *array_of_requests,
                                 int *outcount,
                                 int *array_of_indices,
                                 MPI_Status *array_of_statuses);
int unimpi_wrap_openmpi_testall(int count,
                                MPI_Request *array_of_requests,
                                int *flag,
                                MPI_Status *array_of_statuses);
int unimpi_wrap_openmpi_waitsome(int incount,
                                 MPI_Request *array_of_requests,
                                 int *outcount,
                                 int *array_of_indices,
                                 MPI_Status *array_of_statuses);
int unimpi_wrap_openmpi_testany(int count,
                                MPI_Request *array_of_requests,
                                int *index, int *flag, MPI_Status *status);
int unimpi_wrap_openmpi_waitany(int count,
                                MPI_Request *array_of_requests,
                                int *index, MPI_Status *status);
int unimpi_wrap_openmpi_startall(int count,
                                 MPI_Request *array_of_requests);

#endif /* UNIMPI_REQUEST_ARRAY_WRAPPERS_H */
