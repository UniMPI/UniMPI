#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi_loader.h"

void test_backend_detection_env(void) {
    /* Test with no environment variable */
    unsetenv("UNIMPI_BACKEND");
    const char *backend = unimpi_loader_get_env_backend();
    assert(backend == NULL);

    /* Test with environment variable set */
    setenv("UNIMPI_BACKEND", "openmpi", 1);
    backend = unimpi_loader_get_env_backend();
    assert(backend != NULL);
    assert(strcmp(backend, "openmpi") == 0);

    /* Cleanup */
    unsetenv("UNIMPI_BACKEND");

    printf("  Backend detection tests passed\n");
}

void test_backend_info(void) {
    /* Test backend info array */
    assert(UNIMPI_MAX_BACKENDS == 4);

    /* OpenMPI should be first */
    assert(unimpi_backends[0].type == UNIMPI_BACKEND_OPENMPI);
    assert(strcmp(unimpi_backends[0].name, "openmpi") == 0);

    /* MPICH should be present */
    int found_mpich = 0;
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        if (unimpi_backends[i].type == UNIMPI_BACKEND_MPICH) {
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
