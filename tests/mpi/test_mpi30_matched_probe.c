/* tests/mpi/test_mpi30_matched_probe.c
 * MPI-3.0 matched-probe semantic tests: the Mprobe / Improbe / Mrecv /
 * Imrecv family. Matched probe lets a receiver "match" a specific message
 * (by wildcard source/tag over messages) and then receive exactly that
 * message handle -- unlike blocking Probe, which only observes the first
 * matching message and requires a separate Recv that can race.
 *
 * Each sub-test asserts real data round-trips and the contents of the probe
 * Status (source, tag, count), not just a clean exit. Matched-probe is not
 * exported by some backends (e.g. MS-MPI); those sub-tests degrade to a
 * documented skip, matching the optional-use pattern in
 * test_collective_nonblocking.c.
 *
 * Runs only when UniMPI's target is >= MPI-3.0 (see tests/CMakeLists.txt).
 */

#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include "unimpi.h"

#define CHECK(expr) do { \
    if ((expr) != MPI_SUCCESS) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } } while (0)

/* Matched-probe forwarding is provided by the backend; if it does not export
 * mprobe, the whole family is unavailable and every sub-test skips. */
static int matched_probe_available(void) {
    return unimpi.mprobe != NULL;
}

/* Blocking Mprobe -> Mrecv: every rank sends a tagged payload to its next
 * neighbor (nonblocking, so it stays pending), then Mprobes with WILDCARD
 * source and tag. Assert the Status reports the true source, tag and count,
 * then Mrecv exactly that message and round-trip its payload. */
static int test_matched_probe_blocking(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size < 2) return 0;

    if (!matched_probe_available()) {
        if (rank == 0) printf("  matched_probe unavailable; skip\n");
        return 0;
    }

    const int tag = 77;
    int payload = 5000 + rank;
    int recv = -1;
    MPI_Request sreq;
    CHECK(MPI_Isend(&payload, 1, MPI_INT, (rank + 1) % size, tag, comm,
                    &sreq));

    MPI_Message msg;
    MPI_Status status;
    CHECK(MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &msg, &status));
    int expect_src = (rank + size - 1) % size;
    int src = -1;
    int t = -1;
    CHECK(MPI_Status_get_source(&status, &src));
    CHECK(MPI_Status_get_tag(&status, &t));
    if (src != expect_src) {
        fprintf(stderr, "FAIL rank %d mprobe source=%d expect=%d\n",
                rank, src, expect_src);
        return 1;
    }
    if (t != tag) {
        fprintf(stderr, "FAIL rank %d mprobe tag=%d expect=%d\n",
                rank, t, tag);
        return 1;
    }
    int n = 0;
    CHECK(MPI_Get_count(&status, MPI_INT, &n));
    if (n != 1) {
        fprintf(stderr, "FAIL rank %d mprobe get_count=%d expect=1\n",
                rank, n);
        return 1;
    }

    CHECK(MPI_Mrecv(&recv, 1, MPI_INT, &msg, &status));
    int expect_val = 5000 + (rank + size - 1) % size;
    if (recv != expect_val) {
        fprintf(stderr, "FAIL rank %d mrecv=%d expect=%d\n",
                rank, recv, expect_val);
        return 1;
    }
    CHECK(MPI_Wait(&sreq, MPI_STATUS_IGNORE));
    printf("  Mprobe/Mrecv (wildcard src+tag, get_count) OK (rank %d)\n",
           rank);
    return 0;
}

/* Nonblocking Improbe -> Imrecv: poll Improbe until the message handle
 * appears, assert its source, then Imrecv and Wait. Exercises the
 * nonblocking matched-probe path end to end. */
static int test_matched_probe_improbe(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size < 2) return 0;

    if (!matched_probe_available()) {
        if (rank == 0) printf("  matched_probe unavailable; skip\n");
        return 0;
    }

    const int tag = 88;
    int payload = 6000 + rank;
    int recv = -1;
    MPI_Request sreq;
    CHECK(MPI_Isend(&payload, 1, MPI_INT, (rank + 1) % size, tag, comm,
                    &sreq));

    MPI_Message msg;
    MPI_Status status;
    int flag = 0;
    int spins = 0;
    while (!flag && spins++ < 100000) {
        CHECK(MPI_Improbe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, &msg,
                          &status));
    }
    if (!flag) {
        fprintf(stderr, "FAIL rank %d improbe never matched\n", rank);
        return 1;
    }
    int expect_src = (rank + size - 1) % size;
    int src = -1;
    int t = -1;
    CHECK(MPI_Status_get_source(&status, &src));
    CHECK(MPI_Status_get_tag(&status, &t));
    if (src != expect_src || t != tag) {
        fprintf(stderr, "FAIL rank %d improbe src=%d tag=%d\n",
                rank, src, t);
        return 1;
    }

    MPI_Request rreq;
    CHECK(MPI_Imrecv(&recv, 1, MPI_INT, &msg, &rreq));
    CHECK(MPI_Wait(&rreq, &status));
    int expect_val = 6000 + (rank + size - 1) % size;
    if (recv != expect_val) {
        fprintf(stderr, "FAIL rank %d imrecv=%d expect=%d\n",
                rank, recv, expect_val);
        return 1;
    }
    CHECK(MPI_Wait(&sreq, MPI_STATUS_IGNORE));
    printf("  Improbe/Imrecv (nonblocking) OK (rank %d)\n", rank);
    return 0;
}

/* Tag selectivity: every rank posts TWO pending tagged messages to its next
 * neighbor (tag 1 and tag 2). A Mprobe with a SPECIFIC tag must match only
 * the message carrying that tag -- not the first arrival -- and Mrecv must
 * receive exactly that message's payload. This is the semantic heart of
 * matched probe that plain Probe+Recv cannot express. */
static int test_matched_probe_tag_select(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size < 2) return 0;

    if (!matched_probe_available()) {
        if (rank == 0) printf("  matched_probe unavailable; skip\n");
        return 0;
    }

    int send_t1 = 7000 + rank;  /* tag 1 */
    int send_t2 = 8000 + rank;  /* tag 2 */
    MPI_Request s1, s2;
    CHECK(MPI_Isend(&send_t1, 1, MPI_INT, (rank + 1) % size, 1, comm, &s1));
    CHECK(MPI_Isend(&send_t2, 1, MPI_INT, (rank + 1) % size, 2, comm, &s2));

    /* Probe specifically for tag 2 first, even though tag 1 arrived too. */
    MPI_Message msg;
    MPI_Status status;
    CHECK(MPI_Mprobe(MPI_ANY_SOURCE, 2, comm, &msg, &status));
    int t = -1;
    CHECK(MPI_Status_get_tag(&status, &t));
    if (t != 2) {
        fprintf(stderr, "FAIL rank %d tag-select got tag=%d expect=2\n",
                rank, t);
        return 1;
    }
    int recv2 = -1;
    CHECK(MPI_Mrecv(&recv2, 1, MPI_INT, &msg, &status));
    int expect_t2 = 8000 + (rank + size - 1) % size;
    if (recv2 != expect_t2) {
        fprintf(stderr, "FAIL rank %d tag-select recv2=%d expect=%d\n",
                rank, recv2, expect_t2);
        return 1;
    }

    /* Now the tag-1 message remains; receive it too. */
    CHECK(MPI_Mprobe(MPI_ANY_SOURCE, 1, comm, &msg, &status));
    int recv1 = -1;
    CHECK(MPI_Mrecv(&recv1, 1, MPI_INT, &msg, &status));
    int expect_t1 = 7000 + (rank + size - 1) % size;
    if (recv1 != expect_t1) {
        fprintf(stderr, "FAIL rank %d tag-select recv1=%d expect=%d\n",
                rank, recv1, expect_t1);
        return 1;
    }

    CHECK(MPI_Wait(&s1, MPI_STATUS_IGNORE));
    CHECK(MPI_Wait(&s2, MPI_STATUS_IGNORE));
    printf("  Mprobe tag-selectivity (2 pending tags) OK (rank %d)\n",
           rank);
    return 0;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    printf("=== MPI-3.0 matched-probe tests ===\n");
    if (test_matched_probe_blocking(MPI_COMM_WORLD)) goto fail;
    if (test_matched_probe_improbe(MPI_COMM_WORLD)) goto fail;
    if (test_matched_probe_tag_select(MPI_COMM_WORLD)) goto fail;
    printf("=== All MPI-3.0 matched-probe tests passed ===\n");
    MPI_Finalize();
    return 0;
fail:
    MPI_Finalize();
    return 1;
}
