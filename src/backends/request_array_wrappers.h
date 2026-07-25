/* Completion-array ABI adapters for runtime-selected MPI backends. */
#ifndef UNIMPI_REQUEST_ARRAY_WRAPPERS_H
#define UNIMPI_REQUEST_ARRAY_WRAPPERS_H

#include "unimpi_vtable.h"

/*
 * Open MPI exposes this public native layout.  Keep the compact native form
 * separate from UniMPI's padded facade status so arrays have the stride the
 * backend expects.
 */
typedef struct {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int cancelled;
    size_t count;
} unimpi_openmpi_native_status_t;

typedef int (UNIMPI_MPI_CALL *unimpi_native_error_class_fn)(int, int *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_waitall_fn)(
    int, int *, struct unimpi_status_legacy *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_any_fn)(
    int, int *, int *, int *, struct unimpi_status_legacy *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_some_fn)(
    int, int *, int *, int *, struct unimpi_status_legacy *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_testall_fn)(
    int, int *, int *, struct unimpi_status_legacy *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_waitany_fn)(
    int, int *, int *, struct unimpi_status_legacy *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_legacy_startall_fn)(int, int *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_waitall_fn)(
    int, MPI_Request *, unimpi_openmpi_native_status_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_some_fn)(
    int, MPI_Request *, int *, int *, unimpi_openmpi_native_status_t *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_openmpi_testall_fn)(
    int, MPI_Request *, int *, unimpi_openmpi_native_status_t *);

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

/* Open MPI compact-status-array function registration. */
void unimpi_wrapper_set_openmpi_waitall(
    unimpi_native_openmpi_waitall_fn fn);
void unimpi_wrapper_set_openmpi_testsome(
    unimpi_native_openmpi_some_fn fn);
void unimpi_wrapper_set_openmpi_testall(
    unimpi_native_openmpi_testall_fn fn);
void unimpi_wrapper_set_openmpi_waitsome(
    unimpi_native_openmpi_some_fn fn);

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

/* Open MPI status-array wrappers. */
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

#endif /* UNIMPI_REQUEST_ARRAY_WRAPPERS_H */
