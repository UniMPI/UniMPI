/* tests/mpi/test_mpi30_rma.c
 * MPI-3.0 one-sided (RMA) epoch-model semantic tests: the passive-target
 * (lock_all / lock) family, the flush* family, Win_sync, and window Info
 * plumbing, plus a graceful-degrade cover of shared windows.
 *
 * Each sub-test asserts real data round-trips (not just a clean exit). The
 * inherently environment-dependent bits -- win_allocate_shared and
 * win_shared_query -- degrade to a documented skip rather than failing the
 * suite, matching the dynamic-window handling in test_mpi30_core.c.
 *
 * Runs only when UniMPI's target is >= MPI-3.0 (see tests/CMakeLists.txt).
 */

#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

#define CHECK(expr) do { \
    if ((expr) != MPI_SUCCESS) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } } while (0)

/* lock_all epoch: every rank puts a marker into its next neighbor's window,
 * flushes all, syncs, then unlocks. Assert each rank observes its neighbor's
 * marker after a barrier. */
static int test_lock_all_epoch(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int buf = rank;
    MPI_Win win;
    CHECK(MPI_Win_create(&buf, sizeof(int), sizeof(int), MPI_INFO_NULL,
                         comm, &win));
    int next = (rank + 1) % size;
    int v = 100 + rank;
    CHECK(MPI_Win_lock_all(0, win));
    CHECK(MPI_Put(&v, 1, MPI_INT, next, 0, 1, MPI_INT, win));
    CHECK(MPI_Win_flush_all(win));   /* all puts complete at their targets */
    CHECK(MPI_Win_sync(win));        /* end of access epoch, start of exposure */
    CHECK(MPI_Win_unlock_all(win));
    CHECK(MPI_Barrier(comm));        /* all targets have been written */
    int expected = 100 + (rank + size - 1) % size;
    if (buf != expected) {
        fprintf(stderr, "FAIL rank %d lock_all round-trip buf=%d expect=%d\n",
                rank, buf, expected);
        return 1;
    }
    printf("  lock_all/flush_all/sync/unlock_all OK (rank %d buf %d)\n",
           rank, buf);
    CHECK(MPI_Win_free(&win));
    return 0;
}

/* lock (passive target, exclusive) epoch with a REMOTE target: every rank
 * locks its next neighbor's window, puts a marker, flushes that target, then
 * unlocks. A barrier + owner-side read verifies the flush made the put
 * visible at the target before the epoch tore down. (Self-lock + self-put is
 * rejected by some backends, so targets are cross-rank.) */
static int test_lock_flush_epoch(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int buf = rank;
    MPI_Win win;
    CHECK(MPI_Win_create(&buf, sizeof(int), sizeof(int), MPI_INFO_NULL,
                         comm, &win));
    int next = (rank + 1) % size;
    int v = 1000 + rank;
    CHECK(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, next, 0, win));
    CHECK(MPI_Put(&v, 1, MPI_INT, next, 0, 1, MPI_INT, win));
    CHECK(MPI_Win_flush(next, win)); /* put completed at the target */
    CHECK(MPI_Win_unlock(next, win));
    CHECK(MPI_Barrier(comm));
    int expected = 1000 + (rank + size - 1) % size;
    if (buf != expected) {
        fprintf(stderr, "FAIL rank %d lock/flush buf=%d expect=%d\n",
                rank, buf, expected);
        return 1;
    }
    printf("  lock/flush/unlock(remote) OK (rank %d buf %d)\n", rank, buf);
    CHECK(MPI_Win_free(&win));
    return 0;
}

/* flush_local only guarantees local (not remote) completion; the target-side
 * value is asserted only after the unlock tears the epoch down (unlock
 * completes every prior op). Targets are cross-rank for backend portability. */
static int test_lock_flush_local(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int buf = rank;
    MPI_Win win;
    CHECK(MPI_Win_create(&buf, sizeof(int), sizeof(int), MPI_INFO_NULL,
                         comm, &win));
    int next = (rank + 1) % size;
    int v = 2000 + rank;
    CHECK(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, next, 0, win));
    CHECK(MPI_Put(&v, 1, MPI_INT, next, 0, 1, MPI_INT, win));
    CHECK(MPI_Win_flush_local(next, win)); /* local completion only */
    CHECK(MPI_Win_unlock(next, win));      /* full completion at epoch end */
    CHECK(MPI_Barrier(comm));
    int expected = 2000 + (rank + size - 1) % size;
    if (buf != expected) {
        fprintf(stderr, "FAIL rank %d lock/flush_local buf=%d expect=%d\n",
                rank, buf, expected);
        return 1;
    }
    printf("  lock/flush_local/unlock(remote) OK (rank %d buf %d)\n", rank, buf);
    CHECK(MPI_Win_free(&win));
    return 0;
}

/* Window Info plumbing: the portable contract is that set_info and get_info
 * both succeed and get_info returns an object that can be enumerated and
 * freed. Window Info keys are implementation-defined, so an arbitrary key may
 * legitimately be dropped -- only assert its value when the backend keeps it. */
static int test_win_info(MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);
    int buf = 0;
    MPI_Win win;
    CHECK(MPI_Win_create(&buf, sizeof(int), sizeof(int), MPI_INFO_NULL,
                         comm, &win));
    MPI_Info info;
    CHECK(MPI_Info_create(&info));
    CHECK(MPI_Info_set(info, "unimpi_test_key", "unimpi_test_val"));
    CHECK(MPI_Win_set_info(win, info));
    MPI_Info got;
    CHECK(MPI_Win_get_info(win, &got));
    int nkeys = 0;
    CHECK(MPI_Info_get_nkeys(got, &nkeys));
    if (nkeys < 0 || nkeys > 1024) {
        fprintf(stderr, "FAIL rank %d get_info nkeys=%d\n", rank, nkeys);
        return 1;
    }
    for (int i = 0; i < nkeys; i++) {
        char key[64];
        CHECK(MPI_Info_get_nthkey(got, i, key));
    }
    char value[64];
    int flag = 0;
    CHECK(MPI_Info_get(got, "unimpi_test_key", (int)sizeof(value), value, &flag));
    if (flag && strcmp(value, "unimpi_test_val") != 0) {
        fprintf(stderr, "FAIL rank %d get_info key retained but value='%s'\n",
                rank, value);
        return 1;
    }
    CHECK(MPI_Info_free(&got));
    CHECK(MPI_Info_free(&info));
    CHECK(MPI_Win_free(&win));
    printf("  win_set_info/get_info OK (%d key%s%s)\n", nkeys,
           nkeys == 1 ? "" : "s", flag ? ", custom key retained" : "");
    return 0;
}

/* Shared window + shared_query. Backends/configs may not support SHARED-flavor
 * windows; degrade to a documented skip when the create fails. */
static int test_win_allocate_shared(MPI_Comm comm) {
    MPI_Errhandler prev;
    MPI_Comm_get_errhandler(MPI_COMM_WORLD, &prev);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Win win;
    void *baseptr = NULL;
    int rc = MPI_Win_allocate_shared(2 * sizeof(int), sizeof(int), MPI_INFO_NULL,
                                     comm, &baseptr, &win);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, prev);
    if (rc != MPI_SUCCESS) {
        printf("  win_allocate_shared unsupported (rc=%d); skip\n", rc);
        return 0;
    }
    int rank;
    MPI_Comm_rank(comm, &rank);
    MPI_Aint size = 0;
    int disp = 0;
    void *ptr = NULL;
    if (MPI_Win_shared_query(win, 0, &size, &disp, &ptr) != MPI_SUCCESS) {
        fprintf(stderr, "FAIL rank %d shared_query\n", rank);
        return 1;
    }
    if (size < (MPI_Aint)sizeof(int) || disp <= 0 || ptr == NULL) {
        fprintf(stderr, "FAIL rank %d shared_query size=%llu disp=%d ptr=%p\n",
                rank, (unsigned long long)size, disp, ptr);
        return 1;
    }
    CHECK(MPI_Win_free(&win));
    printf("  win_allocate_shared/shared_query OK\n");
    return 0;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    printf("=== MPI-3.0 RMA epoch-model tests ===\n");
    if (test_lock_all_epoch(MPI_COMM_WORLD)) goto fail;
    if (test_lock_flush_epoch(MPI_COMM_WORLD)) goto fail;
    if (test_lock_flush_local(MPI_COMM_WORLD)) goto fail;
    if (test_win_info(MPI_COMM_WORLD)) goto fail;
    if (test_win_allocate_shared(MPI_COMM_WORLD)) goto fail;
    printf("=== All MPI-3.0 RMA epoch tests passed ===\n");
    MPI_Finalize();
    return 0;
fail:
    MPI_Finalize();
    return 1;
}
