/* src/backends/request_array_wrappers.c
 * Optimized in-place wrapper functions for MPI_Request array operations.
 *
 * Intel MPI and MPICH use typedef int MPI_Request (4 bytes), while UNIMPI
 * uses typedef intptr_t MPI_Request (8 bytes). These wrappers perform
 * in-place conversion between the two representations with zero memory allocation.
 *
 * Optimization strategy:
 *   - Compression (8→4 bytes): forward scan, reading from even int offsets,
 *     writing to consecutive int positions. Safe because read > write.
 *   - Expansion (4→8 bytes): backward scan with sign extension. Safe because
 *     write > read (writes 8 bytes, reads 4 bytes).
 *
 * Only the 7 array-based request operations need wrapping. Single-request
 * operations work correctly because they pass pointers (same size on x86_64).
 */

#include "request_array_wrappers.h"
#include <stdint.h>

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

/* ---- In-place conversion helpers ---- */

/* Compress 8-byte MPI_Request array to 4-byte int array in-place.
 * Forward scan: for each i from 1 to n-1, read low 4 bytes from position i
 * and write to position i. Safe because source (2*i) > destination (i).
 *
 * Example (little-endian, each cell is 4 bytes):
 *   Before: [req0_lo][req0_hi][req1_lo][req1_hi][req2_lo][req2_hi]...
 *   After:  [req0_lo][req1_lo][req2_lo]...
 *                     ^ written here (reads from +2, +4, +6...)
 */
static inline void reqs_compress_inplace(MPI_Request *reqs, int n) {
    int32_t *p = (int32_t*)reqs;
    /* i=1: read from position 2, write to position 1 */
    /* i=2: read from position 4, write to position 2 */
    /* etc. Source always > destination, so no read-after-write hazard */
    for (int i = 1; i < n; i++) {
        p[i] = p[i * 2];
    }
}

/* Expand 4-byte int array back to 8-byte MPI_Request array in-place.
 * Backward scan with sign extension to preserve negative values.
 * Safe because writing 8 bytes at (i*8) never overlaps reading 4 bytes at (i*4).
 *
 * Example:
 *   Before: [val0][val1][val2]...
 *   After:  [val0 ext][val1 ext][val2 ext]...
 *   Writing at +8, +0, reading at +4, so write > read always.
 */
static inline void reqs_expand_inplace(MPI_Request *reqs, int n) {
    for (int i = n - 1; i >= 0; i--) {
        int32_t val = ((int32_t*)reqs)[i];
        /* Sign-extend to intptr_t, then store as MPI_Request */
        reqs[i] = (MPI_Request)(intptr_t)val;
    }
}

/* ---- Wrapper implementations ---- */

int unimpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                         MPI_Status *array_of_statuses) {
    if (count <= 0 || !array_of_requests) {
        return real_waitall(count, (int*)array_of_requests, array_of_statuses);
    }
    /* In-place compression: 8-byte elements → 4-byte elements */
    reqs_compress_inplace(array_of_requests, count);
    /* Call backend with compressed array (now valid 4-byte int array) */
    int ret = real_waitall(count, (int*)array_of_requests, array_of_statuses);
    /* In-place expansion: restore 8-byte elements with sign extension */
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int unimpi_wrap_testany(int count, MPI_Request *array_of_requests,
                         int *index, int *flag, MPI_Status *status) {
    if (count <= 0 || !array_of_requests) {
        return real_testany(count, (int*)array_of_requests, index, flag, status);
    }
    reqs_compress_inplace(array_of_requests, count);
    int ret = real_testany(count, (int*)array_of_requests, index, flag, status);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int unimpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                          int *outcount, int *array_of_indices,
                          MPI_Status *array_of_statuses) {
    if (incount <= 0 || !array_of_requests) {
        return real_testsome(incount, (int*)array_of_requests, outcount,
                             array_of_indices, array_of_statuses);
    }
    reqs_compress_inplace(array_of_requests, incount);
    int ret = real_testsome(incount, (int*)array_of_requests, outcount,
                           array_of_indices, array_of_statuses);
    reqs_expand_inplace(array_of_requests, incount);
    return ret;
}

int unimpi_wrap_testall(int count, MPI_Request *array_of_requests,
                         int *flag, MPI_Status *array_of_statuses) {
    if (count <= 0 || !array_of_requests) {
        return real_testall(count, (int*)array_of_requests, flag,
                            array_of_statuses);
    }
    reqs_compress_inplace(array_of_requests, count);
    int ret = real_testall(count, (int*)array_of_requests, flag, array_of_statuses);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int unimpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                         int *index, MPI_Status *status) {
    if (count <= 0 || !array_of_requests) {
        return real_waitany(count, (int*)array_of_requests, index, status);
    }
    reqs_compress_inplace(array_of_requests, count);
    int ret = real_waitany(count, (int*)array_of_requests, index, status);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int unimpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                          int *outcount, int *array_of_indices,
                          MPI_Status *array_of_statuses) {
    if (incount <= 0 || !array_of_requests) {
        return real_waitsome(incount, (int*)array_of_requests, outcount,
                             array_of_indices, array_of_statuses);
    }
    reqs_compress_inplace(array_of_requests, incount);
    int ret = real_waitsome(incount, (int*)array_of_requests, outcount,
                           array_of_indices, array_of_statuses);
    reqs_expand_inplace(array_of_requests, incount);
    return ret;
}

int unimpi_wrap_startall(int count, MPI_Request *array_of_requests) {
    if (count <= 0 || !array_of_requests) {
        return real_startall(count, (int*)array_of_requests);
    }
    reqs_compress_inplace(array_of_requests, count);
    int ret = real_startall(count, (int*)array_of_requests);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}
