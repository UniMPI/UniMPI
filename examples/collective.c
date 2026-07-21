/* collective.c - Collective communication examples (Reduce, Allreduce, Broadcast, Gather) */
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;
    int sendbuf, recvbuf;
    int *sendbuf_all = NULL;
    int *recvbuf_all = NULL;
    int root = 0;
    size_t gathered_count;

    /* Initialize unimpi */
    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("This example requires at least 2 processes\n");
        unimpi_finalize();
        return 1;
    }

    if ((size_t)size > (size_t)-1 / (size_t)size / sizeof(*recvbuf_all)) {
        fprintf(stderr, "Collective buffer size is too large\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    gathered_count = (size_t)size * (size_t)size;

    printf("Rank %d: Starting collective operations example\n", rank);

    /* ==================== Broadcast ==================== */
    if (rank == root) {
        sendbuf = 100 + rank;
        printf("Rank %d: Broadcasting value %d to all processes\n", rank, sendbuf);
    } else {
        sendbuf = 0;  /* Other ranks will receive the value */
    }

    MPI_Bcast(&sendbuf, 1, MPI_INT, root, MPI_COMM_WORLD);
    printf("Rank %d: After Bcast, value = %d\n", rank, sendbuf);

    /* Barrier to synchronize */
    MPI_Barrier(MPI_COMM_WORLD);

    /* ==================== Reduce ==================== */
    sendbuf = rank + 1;  /* Each rank contributes its rank+1 */
    printf("Rank %d: Contributing %d to reduction\n", rank, sendbuf);

    MPI_Reduce(&sendbuf, &recvbuf, 1, MPI_INT, MPI_SUM, root, MPI_COMM_WORLD);

    if (rank == root) {
        int expected = size * (size + 1) / 2;  /* Sum of 1 to size */
        printf("Rank %d: Reduce result (SUM) = %d (expected: %d)\n", rank, recvbuf, expected);
    }

    /* ==================== AllReduce ==================== */
    sendbuf = rank + 1;
    MPI_Allreduce(&sendbuf, &recvbuf, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    printf("Rank %d: AllReduce result (MAX) = %d (expected: %d)\n", rank, recvbuf, size);

    /* ==================== Gather ==================== */
    sendbuf_all = (int *)malloc((size_t)size * sizeof(*sendbuf_all));
    recvbuf_all = (int *)malloc(gathered_count * sizeof(*recvbuf_all));
    if (!sendbuf_all || !recvbuf_all) {
        fprintf(stderr, "Failed to allocate collective buffers\n");
        free(sendbuf_all);
        free(recvbuf_all);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    for (int i = 0; i < size; i++) {
        sendbuf_all[i] = rank * 10 + i;  /* Each rank creates unique data */
    }

    printf("Rank %d: Sending data: ", rank);
    for (int i = 0; i < size; i++) {
        printf("%d ", sendbuf_all[i]);
    }
    printf("\n");

    MPI_Gather(sendbuf_all, size, MPI_INT,
               rank == root ? recvbuf_all : NULL, size, MPI_INT,
               root, MPI_COMM_WORLD);

    if (rank == root) {
        printf("Rank %d: Gather received: ", rank);
        for (size_t i = 0; i < gathered_count; i++) {
            printf("%d ", recvbuf_all[i]);
        }
        printf("\n");
    }

    /* ==================== AllGather ==================== */
    MPI_Allgather(sendbuf_all, size, MPI_INT,
                  recvbuf_all, size, MPI_INT,
                  MPI_COMM_WORLD);

    printf("Rank %d: AllGather received: ", rank);
    for (size_t i = 0; i < gathered_count; i++) {
        printf("%d ", recvbuf_all[i]);
    }
    printf("\n");

    /* ==================== Scatter ==================== */
    if (rank == root) {
        /* Root prepares data for scattering */
        for (int i = 0; i < size; i++) {
            sendbuf_all[i] = (i + 1) * 100;
        }
        printf("Rank %d: Scattering values: ", rank);
        for (int i = 0; i < size; i++) {
            printf("%d ", sendbuf_all[i]);
        }
        printf("\n");
    }

    MPI_Scatter(sendbuf_all, 1, MPI_INT,
                &recvbuf, 1, MPI_INT,
                root, MPI_COMM_WORLD);

    printf("Rank %d: Received scattered value = %d\n", rank, recvbuf);

    /* ==================== Scan (Prefix Sum) ==================== */
    sendbuf = rank + 1;
    MPI_Scan(&sendbuf, &recvbuf, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    printf("Rank %d: Scan (prefix sum) result = %d\n", rank, recvbuf);

    /* Cleanup */
    free(sendbuf_all);
    free(recvbuf_all);

    printf("Rank %d: Collective operations complete\n", rank);
    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n", unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
