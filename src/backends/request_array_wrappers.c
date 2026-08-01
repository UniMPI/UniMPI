/*
 * Completion-array ABI adapters.
 *
 * UniMPI uses intptr_t request handles and a padded 128-byte status facade.
 * MPICH-family backends and MS-MPI use 32-bit request handles plus compact
 * five-int statuses.  Open MPI request handles already match intptr_t, but
 * its native status stride is also smaller than the facade stride.  Native
 * temporary arrays keep both representations isolated at the call boundary.
 */
#include "request_array_wrappers.h"
#include "request_handle_wrappers.h"
#include "unimpi.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Integer-handle backend entry points. */
static unimpi_native_legacy_waitall_fn real_waitall;
static unimpi_native_legacy_any_fn real_testany;
static unimpi_native_legacy_some_fn real_testsome;
static unimpi_native_legacy_testall_fn real_testall;
static unimpi_native_legacy_waitany_fn real_waitany;
static unimpi_native_legacy_some_fn real_waitsome;
static unimpi_native_legacy_startall_fn real_startall;

/* Open MPI entry points needing compact status arrays. */
static unimpi_native_openmpi_waitall_fn real_openmpi_waitall;
static unimpi_native_openmpi_some_fn real_openmpi_testsome;
static unimpi_native_openmpi_testall_fn real_openmpi_testall;
static unimpi_native_openmpi_some_fn real_openmpi_waitsome;

static unimpi_native_error_class_fn real_error_class;

void unimpi_wrapper_set_error_class(unimpi_native_error_class_fn fn) {
    real_error_class = fn;
}

void unimpi_wrapper_set_waitall(unimpi_native_legacy_waitall_fn fn) {
    real_waitall = fn;
}

void unimpi_wrapper_set_testany(unimpi_native_legacy_any_fn fn) {
    real_testany = fn;
}

void unimpi_wrapper_set_testsome(unimpi_native_legacy_some_fn fn) {
    real_testsome = fn;
}

void unimpi_wrapper_set_testall(unimpi_native_legacy_testall_fn fn) {
    real_testall = fn;
}

void unimpi_wrapper_set_waitany(unimpi_native_legacy_waitany_fn fn) {
    real_waitany = fn;
}

void unimpi_wrapper_set_waitsome(unimpi_native_legacy_some_fn fn) {
    real_waitsome = fn;
}

void unimpi_wrapper_set_startall(unimpi_native_legacy_startall_fn fn) {
    real_startall = fn;
}

void unimpi_wrapper_set_openmpi_waitall(
    unimpi_native_openmpi_waitall_fn fn) {
    real_openmpi_waitall = fn;
}

void unimpi_wrapper_set_openmpi_testsome(
    unimpi_native_openmpi_some_fn fn) {
    real_openmpi_testsome = fn;
}

void unimpi_wrapper_set_openmpi_testall(
    unimpi_native_openmpi_testall_fn fn) {
    real_openmpi_testall = fn;
}

void unimpi_wrapper_set_openmpi_waitsome(
    unimpi_native_openmpi_some_fn fn) {
    real_openmpi_waitsome = fn;
}

static int no_memory_error(void) {
    return MPI_ERR_NO_MEM != MPI_SUCCESS ? MPI_ERR_NO_MEM : 39;
}


static int completion_error_class(int result) {
    int error_class;

    if (result == MPI_SUCCESS || !real_error_class) {
        return result;
    }
    error_class = result;
    if (real_error_class(result, &error_class) != MPI_SUCCESS) {
        return result;
    }
    return error_class;
}

static int should_store_completion(int result) {
    int result_class;

    if (result == MPI_SUCCESS) {
        return 1;
    }
    result_class = completion_error_class(result);
    return result_class == MPI_ERR_IN_STATUS;
}


static int statuses_are_ignored(const MPI_Status *statuses) {
    return statuses == NULL || statuses == UNIMPI_STATUSES_IGNORE;
}

/* Five-int native status stride used by MPICH/Intel/MS-MPI. */
enum {
    UNIMPI_NATIVE_STATUS_BYTES = sizeof(struct unimpi_status_legacy)
};

static struct MPI_Status *native_statuses_ignore(void) {
    return (struct MPI_Status *)UNIMPI_STATUSES_IGNORE;
}

static void *allocate_array(int count, size_t element_size) {
    if (count <= 0 || element_size == 0 ||
        (size_t)count > SIZE_MAX / element_size) {
        return NULL;
    }
    return calloc((size_t)count, element_size);
}

/* Opaque native status storage: allocate known five-int cells, never sizeof
 * the incomplete vendor struct MPI_Status. */
static void *native_status_cells_create(int count) {
    return allocate_array(count, UNIMPI_NATIVE_STATUS_BYTES);
}

static struct MPI_Status *as_native_status_ptr(void *cells) {
    return (struct MPI_Status *)cells;
}

static int *legacy_requests_create(const MPI_Request *requests, int count) {
    int *native_requests;
    int i;

    native_requests = (int *)allocate_array(count, sizeof(*native_requests));
    if (!native_requests) {
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        native_requests[i] = unimpi_request_to_native(requests[i]);
    }
    return native_requests;
}

static void legacy_requests_store(const int *native_requests,
                                  MPI_Request *requests,
                                  int count) {
    int i;

    for (i = 0; i < count; ++i) {
        requests[i] = unimpi_request_from_native(native_requests[i]);
    }
}

static void native_statuses_store_to_facade(
    const void *native_cells,
    MPI_Status *statuses,
    int count) {
    int i;
    const unsigned char *src = (const unsigned char *)native_cells;

    for (i = 0; i < count; ++i) {
        memset(&statuses[i], 0, sizeof(statuses[i]));
        memcpy(&statuses[i].legacy,
               src + (size_t)i * UNIMPI_NATIVE_STATUS_BYTES,
               UNIMPI_NATIVE_STATUS_BYTES);
    }
}

static void openmpi_statuses_store(
    const struct ompi_status_public_t *native_statuses,
    MPI_Status *statuses,
    int count) {
    int i;

    for (i = 0; i < count; ++i) {
        memset(&statuses[i], 0, sizeof(statuses[i]));
        memcpy(&statuses[i].openmpi, &native_statuses[i],
               sizeof(native_statuses[i]));
    }
}

static struct ompi_request_t **openmpi_requests_create(
    const MPI_Request *requests, int count) {
    struct ompi_request_t **native;
    int i;

    native = (struct ompi_request_t **)allocate_array(
        count, sizeof(*native));
    if (!native) {
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        native[i] = (struct ompi_request_t *)(intptr_t)requests[i];
    }
    return native;
}

static void openmpi_requests_store(
    struct ompi_request_t *const *native,
    MPI_Request *requests,
    int count) {
    int i;

    for (i = 0; i < count; ++i) {
        requests[i] = (MPI_Request)(intptr_t)native[i];
    }
}

static int completed_status_count(int requested, const int *outcount) {
    if (!outcount || *outcount <= 0) {
        return 0;
    }
    return *outcount < requested ? *outcount : requested;
}

int unimpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                        MPI_Status *array_of_statuses) {
    int *native_requests;
    void *native_status_cells = NULL;
    int ignore_statuses;
    int result;
    int result_class;

    if (count <= 0) {
        return real_waitall(
            count, NULL,
            statuses_are_ignored(array_of_statuses)
                ? native_statuses_ignore()
                : as_native_status_ptr(array_of_statuses));
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (!ignore_statuses) {
        native_status_cells = native_status_cells_create(count);
        if (!native_status_cells) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_waitall(
        count, native_requests,
        ignore_statuses ? native_statuses_ignore()
                        : as_native_status_ptr(native_status_cells));
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, count);
        if (native_status_cells &&
            (result_class == MPI_SUCCESS ||
             result_class == MPI_ERR_IN_STATUS)) {
            native_statuses_store_to_facade(
                native_status_cells, array_of_statuses, count);
        }
    }

    free(native_status_cells);
    free(native_requests);
    return result;
}

int unimpi_wrap_testany(int count, MPI_Request *array_of_requests,
                        int *index, int *flag, MPI_Status *status) {
    int *native_requests;
    void *native_status_cells = NULL;
    int ignore;
    int result;

    ignore = statuses_are_ignored(status);
    if (count <= 0) {
        return real_testany(
            count, NULL, index, flag,
            ignore ? native_statuses_ignore() : as_native_status_ptr(status));
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    if (!ignore) {
        native_status_cells = native_status_cells_create(1);
        if (!native_status_cells) {
            free(native_requests);
            return no_memory_error();
        }
    }
    result = real_testany(
        count, native_requests, index, flag,
        ignore ? native_statuses_ignore()
               : as_native_status_ptr(native_status_cells));
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, count);
        if (native_status_cells && flag && *flag) {
            native_statuses_store_to_facade(native_status_cells, status, 1);
        }
    }
    free(native_status_cells);
    free(native_requests);
    return result;
}

int unimpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses) {
    int *native_requests;
    void *native_status_cells = NULL;
    int ignore_statuses;
    int result;
    int result_class;
    int status_count;

    if (incount <= 0) {
        return real_testsome(
            incount, NULL, outcount, array_of_indices,
            statuses_are_ignored(array_of_statuses)
                ? native_statuses_ignore()
                : as_native_status_ptr(array_of_statuses));
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, incount);
    if (!native_requests) {
        return no_memory_error();
    }
    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (!ignore_statuses) {
        native_status_cells = native_status_cells_create(incount);
        if (!native_status_cells) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_testsome(
        incount, native_requests, outcount, array_of_indices,
        ignore_statuses ? native_statuses_ignore()
                        : as_native_status_ptr(native_status_cells));
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, incount);
        if (native_status_cells &&
            (result_class == MPI_SUCCESS ||
             result_class == MPI_ERR_IN_STATUS)) {
            status_count = completed_status_count(incount, outcount);
            if (status_count > 0) {
                native_statuses_store_to_facade(
                    native_status_cells, array_of_statuses, status_count);
            }
        }
    }

    free(native_status_cells);
    free(native_requests);
    return result;
}

int unimpi_wrap_testall(int count, MPI_Request *array_of_requests,
                        int *flag, MPI_Status *array_of_statuses) {
    int *native_requests;
    void *native_status_cells = NULL;
    int ignore_statuses;
    int result;
    int result_class;

    if (count <= 0) {
        return real_testall(
            count, NULL, flag,
            statuses_are_ignored(array_of_statuses)
                ? native_statuses_ignore()
                : as_native_status_ptr(array_of_statuses));
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (!ignore_statuses) {
        native_status_cells = native_status_cells_create(count);
        if (!native_status_cells) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_testall(
        count, native_requests, flag,
        ignore_statuses ? native_statuses_ignore()
                        : as_native_status_ptr(native_status_cells));
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, count);
        if (native_status_cells &&
            (result_class == MPI_ERR_IN_STATUS ||
             (result_class == MPI_SUCCESS && flag && *flag))) {
            native_statuses_store_to_facade(native_status_cells, array_of_statuses, count);
        }
    }

    free(native_status_cells);
    free(native_requests);
    return result;
}

int unimpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                        int *index, MPI_Status *status) {
    int *native_requests;
    void *native_status_cells = NULL;
    int ignore;
    int result;

    ignore = statuses_are_ignored(status);
    if (count <= 0) {
        return real_waitany(
            count, NULL, index,
            ignore ? native_statuses_ignore() : as_native_status_ptr(status));
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    if (!ignore) {
        native_status_cells = native_status_cells_create(1);
        if (!native_status_cells) {
            free(native_requests);
            return no_memory_error();
        }
    }
    result = real_waitany(
        count, native_requests, index,
        ignore ? native_statuses_ignore()
               : as_native_status_ptr(native_status_cells));
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, count);
        if (native_status_cells) {
            native_statuses_store_to_facade(native_status_cells, status, 1);
        }
    }
    free(native_status_cells);
    free(native_requests);
    return result;
}

int unimpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses) {
    int *native_requests;
    void *native_status_cells = NULL;
    int ignore_statuses;
    int result;
    int result_class;
    int status_count;

    if (incount <= 0) {
        return real_waitsome(
            incount, NULL, outcount, array_of_indices,
            statuses_are_ignored(array_of_statuses)
                ? native_statuses_ignore()
                : as_native_status_ptr(array_of_statuses));
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, incount);
    if (!native_requests) {
        return no_memory_error();
    }
    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (!ignore_statuses) {
        native_status_cells = native_status_cells_create(incount);
        if (!native_status_cells) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_waitsome(
        incount, native_requests, outcount, array_of_indices,
        ignore_statuses ? native_statuses_ignore()
                        : as_native_status_ptr(native_status_cells));
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, incount);
        if (native_status_cells &&
            (result_class == MPI_SUCCESS ||
             result_class == MPI_ERR_IN_STATUS)) {
            status_count = completed_status_count(incount, outcount);
            if (status_count > 0) {
                native_statuses_store_to_facade(
                    native_status_cells, array_of_statuses, status_count);
            }
        }
    }

    free(native_status_cells);
    free(native_requests);
    return result;
}

int unimpi_wrap_startall(int count, MPI_Request *array_of_requests) {
    int *native_requests;
    int result;

    if (count <= 0) {
        return real_startall(count, NULL);
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = legacy_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    result = real_startall(count, native_requests);
    if (should_store_completion(result)) {
        legacy_requests_store(native_requests, array_of_requests, count);
    }
    free(native_requests);
    return result;
}

int unimpi_wrap_openmpi_waitall(int count,
                                MPI_Request *array_of_requests,
                                MPI_Status *array_of_statuses) {
    struct ompi_request_t **native_requests = NULL;
    struct ompi_status_public_t *native_statuses = NULL;
    int ignore_statuses;
    int result;
    int result_class;

    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (count <= 0) {
        return real_openmpi_waitall(
            count, NULL,
            ignore_statuses ? NULL : (struct ompi_status_public_t *)array_of_statuses);
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = openmpi_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    if (!ignore_statuses) {
        native_statuses = (struct ompi_status_public_t *)allocate_array(
            count, sizeof(*native_statuses));
        if (!native_statuses) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_openmpi_waitall(count, native_requests, native_statuses);
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        openmpi_requests_store(native_requests, array_of_requests, count);
        if (native_statuses &&
            (result_class == MPI_SUCCESS ||
             result_class == MPI_ERR_IN_STATUS)) {
            openmpi_statuses_store(native_statuses, array_of_statuses, count);
        }
    }
    free(native_statuses);
    free(native_requests);
    return result;
}

int unimpi_wrap_openmpi_testsome(int incount,
                                 MPI_Request *array_of_requests,
                                 int *outcount,
                                 int *array_of_indices,
                                 MPI_Status *array_of_statuses) {
    struct ompi_request_t **native_requests = NULL;
    struct ompi_status_public_t *native_statuses = NULL;
    int ignore_statuses;
    int result;
    int result_class;
    int status_count;

    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (incount <= 0) {
        return real_openmpi_testsome(
            incount, NULL, outcount, array_of_indices,
            ignore_statuses ? NULL
                            : (struct ompi_status_public_t *)array_of_statuses);
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = openmpi_requests_create(array_of_requests, incount);
    if (!native_requests) {
        return no_memory_error();
    }
    if (!ignore_statuses) {
        native_statuses = (struct ompi_status_public_t *)allocate_array(
            incount, sizeof(*native_statuses));
        if (!native_statuses) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_openmpi_testsome(
        incount, native_requests, outcount, array_of_indices, native_statuses);
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        openmpi_requests_store(native_requests, array_of_requests, incount);
        if (native_statuses &&
            (result_class == MPI_SUCCESS ||
             result_class == MPI_ERR_IN_STATUS)) {
            status_count = completed_status_count(incount, outcount);
            if (status_count > 0) {
                openmpi_statuses_store(
                    native_statuses, array_of_statuses, status_count);
            }
        }
    }
    free(native_statuses);
    free(native_requests);
    return result;
}

int unimpi_wrap_openmpi_testall(int count,
                                MPI_Request *array_of_requests,
                                int *flag,
                                MPI_Status *array_of_statuses) {
    struct ompi_request_t **native_requests = NULL;
    struct ompi_status_public_t *native_statuses = NULL;
    int ignore_statuses;
    int result;
    int result_class;

    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (count <= 0) {
        return real_openmpi_testall(
            count, NULL, flag,
            ignore_statuses ? NULL
                            : (struct ompi_status_public_t *)array_of_statuses);
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = openmpi_requests_create(array_of_requests, count);
    if (!native_requests) {
        return no_memory_error();
    }
    if (!ignore_statuses) {
        native_statuses = (struct ompi_status_public_t *)allocate_array(
            count, sizeof(*native_statuses));
        if (!native_statuses) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_openmpi_testall(
        count, native_requests, flag, native_statuses);
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        openmpi_requests_store(native_requests, array_of_requests, count);
        if (native_statuses &&
            (result_class == MPI_ERR_IN_STATUS ||
             (result_class == MPI_SUCCESS && flag && *flag))) {
            openmpi_statuses_store(native_statuses, array_of_statuses, count);
        }
    }
    free(native_statuses);
    free(native_requests);
    return result;
}

int unimpi_wrap_openmpi_waitsome(int incount,
                                 MPI_Request *array_of_requests,
                                 int *outcount,
                                 int *array_of_indices,
                                 MPI_Status *array_of_statuses) {
    struct ompi_request_t **native_requests = NULL;
    struct ompi_status_public_t *native_statuses = NULL;
    int ignore_statuses;
    int result;
    int result_class;
    int status_count;

    ignore_statuses = statuses_are_ignored(array_of_statuses);
    if (incount <= 0) {
        return real_openmpi_waitsome(
            incount, NULL, outcount, array_of_indices,
            ignore_statuses ? NULL
                            : (struct ompi_status_public_t *)array_of_statuses);
    }
    if (!array_of_requests) {
        return MPI_ERR_REQUEST;
    }

    native_requests = openmpi_requests_create(array_of_requests, incount);
    if (!native_requests) {
        return no_memory_error();
    }
    if (!ignore_statuses) {
        native_statuses = (struct ompi_status_public_t *)allocate_array(
            incount, sizeof(*native_statuses));
        if (!native_statuses) {
            free(native_requests);
            return no_memory_error();
        }
    }

    result = real_openmpi_waitsome(
        incount, native_requests, outcount, array_of_indices, native_statuses);
    result_class = completion_error_class(result);
    if (should_store_completion(result)) {
        openmpi_requests_store(native_requests, array_of_requests, incount);
        if (native_statuses &&
            (result_class == MPI_SUCCESS ||
             result_class == MPI_ERR_IN_STATUS)) {
            status_count = completed_status_count(incount, outcount);
            if (status_count > 0) {
                openmpi_statuses_store(
                    native_statuses, array_of_statuses, status_count);
            }
        }
    }
    free(native_statuses);
    free(native_requests);
    return result;
}
