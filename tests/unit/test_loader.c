#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "tftk_mpi_loader.h"

void test_backend_detection_env(void) {
    /* Test with no environment variable */
    unsetenv("TFTK_MPI_BACKEND");
    const char *backend = tftk_mpi_loader_get_env_backend();
    assert(backend == NULL);

    /* Test with environment variable set */
    setenv("TFTK_MPI_BACKEND", "openmpi", 1);
    backend = tftk_mpi_loader_get_env_backend();
    assert(backend != NULL);
    assert(strcmp(backend, "openmpi") == 0);

    /* Cleanup */
    unsetenv("TFTK_MPI_BACKEND");

    printf("  Backend detection tests passed\n");
}

void test_backend_info(void) {
    /* Test backend info array */
    assert(TFTK_MPI_MAX_BACKENDS == 4);

    /* OpenMPI should be first */
    assert(tftk_mpi_backends[0].type == TFTK_MPI_BACKEND_OPENMPI);
    assert(strcmp(tftk_mpi_backends[0].name, "openmpi") == 0);

    /* MPICH should be present */
    int found_mpich = 0;
    for (int i = 0; i < TFTK_MPI_MAX_BACKENDS; i++) {
        if (tftk_mpi_backends[i].type == TFTK_MPI_BACKEND_MPICH) {
            found_mpich = 1;
            break;
        }
    }
    assert(found_mpich);

    printf("  Backend info tests passed\n");
}

int main(void) {
    printf("Running loader tests...\n");

    test_backend_detection_env();
    test_backend_info();


    printf("All loader tests passed!\n");
    return 0;
}
