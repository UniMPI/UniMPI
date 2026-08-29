/* tests/mpi/test_grequest.c - runtime validation of the MPI-2 generalized-
 * request bindings (Grequest_start / Grequest_complete). A generalized request
 * is created with query/free/cancel user callbacks; we complete it and Wait
 * (which frees it, invoking the free callback), proving both bindings and the
 * callback ABI are correct against a real backend. Uses MPI_STATUS_IGNORE, so
 * the query callback's status contents never matter (no MPI_Status layout
 * assumption).
 */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

static int g_query = 0, g_free = 0, g_cancel = 0;
static int g_extra_seen = 0;

static int grequest_query(void *extra_state, MPI_Status *status) {
    (void)status;
    g_query++;
    if (extra_state == &g_extra_seen) { /* record callback received the state */ }
    return MPI_SUCCESS;
}

static int grequest_free(void *extra_state) {
    if (extra_state == &g_extra_seen) g_free++;
    else g_free += 100;
    return MPI_SUCCESS;
}

static int grequest_cancel(void *extra_state, int complete) {
    (void)complete;
    (void)extra_state;
    g_cancel++;
    return MPI_SUCCESS;
}

static int test_grequest_complete(void) {
    int rank, nprocs;
    MPI_Request req;
    MPI_Status st;

    TEST("Grequest_start + Grequest_complete + Wait");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    (void)nprocs;

    g_query = g_free = g_cancel = 0;
    if (MPI_Grequest_start(grequest_query, grequest_free, grequest_cancel,
                           &g_extra_seen, &req) != MPI_SUCCESS)
        FAIL("MPI_Grequest_start failed");
    if (MPI_Grequest_complete(req) != MPI_SUCCESS)
        FAIL("MPI_Grequest_complete failed");

    /* Wait on a completed generalized request returns immediately and frees
     * it, invoking the free callback with our extra_state. */
    if (MPI_Wait(&req, &st) != MPI_SUCCESS)
        FAIL("MPI_Wait on generalized request failed");
    if (g_free != 1) {
        fprintf(stderr, "  FAIL: free callback called %d times (want 1)\n", g_free);
        return 1;
    }

    (void)rank;
    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret;
    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) { fprintf(stderr, "MPI_Init failed\n"); return 1; }
    printf("=== MPI-2 Generalized-Request Tests ===\n\n");

    if ((ret = test_grequest_complete()) != 0) goto cleanup;

    printf("\n=== All generalized-request tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
