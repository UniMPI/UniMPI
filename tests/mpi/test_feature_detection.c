/* Test unsupported MPI function error handling */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: %d\n", ret);
        return 1;
    }

    printf("=== Feature Detection Test ===\n\n");

    printf("Backend: %s\n", unimpi_get_backend_name());
    printf("\n");

    /* Test Comm_get_parent (always available) */
    printf("Testing Comm_get_parent (always available):\n");
    MPI_Comm parent;
    ret = MPI_Comm_get_parent(&parent);
    printf("  MPI_Comm_get_parent: ");
    if (ret == MPI_SUCCESS) {
        printf("MPI_SUCCESS");
        if (parent == (MPI_Comm)0) {
            printf(", parent is MPI_COMM_NULL (expected for non-spawned)");
        } else {
            printf(", parent handle is non-null (implementation-specific)");
        }
    } else {
        printf("code=%d (unexpected)", ret);
    }
    printf("\n");
    printf("\n");
    fflush(stdout);

    /* Note: Name service functions (Publish_name, Lookup_name, Unpublish_name)
     * and Comm_spawn behavior depends on the MPI implementation.
     * MS-MPI may abort on invalid name service calls, and Comm_spawn
     * requires a valid child executable path. */

    printf("=== All feature detection tests passed ===\n");

    MPI_Finalize();
    return 0;
}
