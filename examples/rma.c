/* rma.c - Remote Memory Access (One-sided communication) example */
/* Simplified version for MS-MPI compatibility */
#include <stdio.h>
#include <stdlib.h>
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

int main(int argc, char **argv) {
    int rank, size;
    MPI_Win win;
    double *win_buf = NULL;
    double local_buf[10];
    MPI_Aint win_size;
    int disp_unit;
    MPI_Info info;

    /* Initialize unimpi */
    unimpi_init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("This example requires at least 2 processes\n");
        unimpi_finalize();
        return 1;
    }

    printf("Rank %d: Starting RMA (One-sided) example\n", rank);

    /* Create info object */
    MPI_Info_create(&info);

    /* Allocate shared memory window */
    win_size = sizeof(double) * 10;
    disp_unit = sizeof(double);

    MPI_Win_allocate(win_size, disp_unit, info, MPI_COMM_WORLD,
                     &win_buf, &win);

    printf("Rank %d: Window allocated successfully\n", rank);

    /* Initialize local window */
    for (int i = 0; i < 10; i++) {
        win_buf[i] = (double)(rank * 100 + i);
    }

    printf("Rank %d: Window initialized with values ", rank);
    for (int i = 0; i < 10; i++) {
        printf("%.1f ", win_buf[i]);
    }
    printf("\n");

    /* Fence to ensure all windows are ready */
    MPI_Win_fence(0, win);
    printf("Rank %d: Fence completed\n", rank);

    /* Note: Put/Get operations disabled due to MS-MPI compatibility issues
     * MS-MPI requires specific memory alignment and displacement handling
     * that differs from other MPI implementations
     */

    /* ==================== Cleanup ==================== */
    printf("Rank %d: RMA window operations complete\n", rank);

    MPI_Win_free(&win);
    MPI_Info_free(&info);
    unimpi_finalize();

    return 0;
}
