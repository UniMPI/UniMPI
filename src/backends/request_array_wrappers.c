/* src/backends/request_array_wrappers.c
 * Wrapper functions for MPI_Request array operations.
 *
 * Intel MPI and MPICH use typedef int MPI_Request (4 bytes), while UNIMPI
 * uses typedef intptr_t MPI_Request (8 bytes). These wrappers convert
 * request arrays between the two representations at the function boundary.
 *
 * Only the 7 array-based request operations need wrapping. Single-request
 * operations work correctly because they pass pointers (same size on x86_64),
 * and the backend only reads/writes the low 4 bytes of each request value.
 */

#include "request_array_wrappers.h"

#ifdef __GNUC__
#define UNIMPI_UNUSED __attribute__((unused))
#else
#define UNIMPI_UNUSED
#endif

/* ---- Real backend function pointers (set during backend init) ---- */
static int (*real_waitall)(int, int*, MPI_Status*);
static int (*real_testany)(int, int*, int*, int*, MPI_Status*);
static int (*real_testsome)(int, int*, int*, int*, MPI_Status*);
static int (*real_testall)(int, int*, int*, MPI_Status*);
static int (*real_waitany)(int, int*, int*, MPI_Status*);
static int (*real_waitsome)(int, int*, int*, int*, MPI_Status*);
static int (*real_startall)(int, int*);

/* ---- Setters ---- */
void unimpi_wrapper_set_waitall(int (*fn)(int, int*, MPI_Status*)) { real_waitall = fn; }
void unimpi_wrapper_set_testany(int (*fn)(int, int*, int*, int*, MPI_Status*)) { real_testany = fn; }
void unimpi_wrapper_set_testsome(int (*fn)(int, int*, int*, int*, MPI_Status*)) { real_testsome = fn; }
void unimpi_wrapper_set_testall(int (*fn)(int, int*, int*, MPI_Status*)) { real_testall = fn; }
void unimpi_wrapper_set_waitany(int (*fn)(int, int*, int*, MPI_Status*)) { real_waitany = fn; }
void unimpi_wrapper_set_waitsome(int (*fn)(int, int*, int*, int*, MPI_Status*)) { real_waitsome = fn; }
void unimpi_wrapper_set_startall(int (*fn)(int, int*)) { real_startall = fn; }

/* ---- Conversion helper ---- */
/* Convert an array of UNIMPI's intptr_t MPI_Request (8 bytes each) to
 * the native int (4 bytes each) used by MPICH/Intel MPI.
 * Returns the number of elements in the native array (== count). */
static void reqs_to_native(const MPI_Request *src, int *dst, int count) {
    int i;
    for (i = 0; i < count; i++) {
        dst[i] = (int)(intptr_t)src[i];
    }
}

/* Convert a native int array back to UNIMPI's intptr_t MPI_Request array. */
static void reqs_from_native(const int *src, MPI_Request *dst, int count) {
    int i;
    for (i = 0; i < count; i++) {
        dst[i] = (MPI_Request)(intptr_t)src[i];
    }
}

/* ---- Wrappers ---- */

int unimpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                         MPI_Status *array_of_statuses) {
    if (count <= 0 || !array_of_requests) {
        return real_waitall(count, (int*)array_of_requests, array_of_statuses);
    }
    /* VLA for native request array */
    int native[count];
    reqs_to_native(array_of_requests, native, count);
    int ret = real_waitall(count, native, array_of_statuses);
    reqs_from_native(native, array_of_requests, count);
    return ret;
}

int unimpi_wrap_testany(int count, MPI_Request *array_of_requests,
                         int *index, int *flag, MPI_Status *status) {
    if (count <= 0 || !array_of_requests) {
        return real_testany(count, (int*)array_of_requests, index, flag, status);
    }
    int native[count];
    reqs_to_native(array_of_requests, native, count);
    int ret = real_testany(count, native, index, flag, status);
    reqs_from_native(native, array_of_requests, count);
    return ret;
}

int unimpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                          int *outcount, int *array_of_indices,
                          MPI_Status *array_of_statuses) {
    if (incount <= 0 || !array_of_requests) {
        return real_testsome(incount, (int*)array_of_requests, outcount,
                             array_of_indices, array_of_statuses);
    }
    int native[incount];
    reqs_to_native(array_of_requests, native, incount);
    int ret = real_testsome(incount, native, outcount, array_of_indices,
                            array_of_statuses);
    reqs_from_native(native, array_of_requests, incount);
    return ret;
}

int unimpi_wrap_testall(int count, MPI_Request *array_of_requests,
                         int *flag, MPI_Status *array_of_statuses) {
    if (count <= 0 || !array_of_requests) {
        return real_testall(count, (int*)array_of_requests, flag,
                            array_of_statuses);
    }
    int native[count];
    reqs_to_native(array_of_requests, native, count);
    int ret = real_testall(count, native, flag, array_of_statuses);
    reqs_from_native(native, array_of_requests, count);
    return ret;
}

int unimpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                         int *index, MPI_Status *status) {
    if (count <= 0 || !array_of_requests) {
        return real_waitany(count, (int*)array_of_requests, index, status);
    }
    int native[count];
    reqs_to_native(array_of_requests, native, count);
    int ret = real_waitany(count, native, index, status);
    reqs_from_native(native, array_of_requests, count);
    return ret;
}

int unimpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                          int *outcount, int *array_of_indices,
                          MPI_Status *array_of_statuses) {
    if (incount <= 0 || !array_of_requests) {
        return real_waitsome(incount, (int*)array_of_requests, outcount,
                             array_of_indices, array_of_statuses);
    }
    int native[incount];
    reqs_to_native(array_of_requests, native, incount);
    int ret = real_waitsome(incount, native, outcount, array_of_indices,
                            array_of_statuses);
    reqs_from_native(native, array_of_requests, incount);
    return ret;
}

int unimpi_wrap_startall(int count, MPI_Request *array_of_requests) {
    if (count <= 0 || !array_of_requests) {
        return real_startall(count, (int*)array_of_requests);
    }
    int native[count];
    reqs_to_native(array_of_requests, native, count);
    int ret = real_startall(count, native);
    reqs_from_native(native, array_of_requests, count);
    return ret;
}
