#include "tftk_mpi.h"
#include "tftk_mpi_vtable.h"
#include "tftk_mpi_loader.h"
#include <stdlib.h>

/* Global state */
static int g_initialized = 0;
static const char *g_backend_name = NULL;
static tftk_mpi_lib_handle_t g_handle = NULL;

int tftk_mpi_init(int *argc, char ***argv) {
    int ret;
    const char *lib_path;

    /* Check if already initialized */
    if (g_initialized) {
        return TFTK_MPI_ERR_ALREADY_INITIALIZED;
    }

    /* Detect backend */
    ret = tftk_mpi_loader_detect_backend(&lib_path);
    if (ret != TFTK_MPI_OK) {
        return ret;
    }

    /* Load backend library */
    ret = tftk_mpi_loader_load(lib_path, &g_handle);
    if (ret != TFTK_MPI_OK) {
        return ret;
    }

    /* Initialize vtable */
    ret = tftk_mpi_vtable_init(g_handle);
    if (ret != TFTK_MPI_OK) {
        tftk_mpi_loader_unload(g_handle);
        g_handle = NULL;
        return ret;
    }

    /* Call MPI_Init through vtable */
    ret = tftk_mpi.init(argc, argv);
    if (ret != 0) {
        tftk_mpi_vtable_cleanup();
        tftk_mpi_loader_unload(g_handle);
        g_handle = NULL;
        return TFTK_MPI_ERR_BACKEND_LOAD;
    }

    /* Set initialized state */
    g_initialized = 1;
    g_backend_name = lib_path;

    return TFTK_MPI_OK;
}

int tftk_mpi_init_with(const char *backend_name) {
    /* TODO: Implement backend-specific initialization */
    /* For now, set environment and call regular init */
    setenv("TFTK_MPI_BACKEND", backend_name, 1);
    return tftk_mpi_init(NULL, NULL);
}

int tftk_mpi_finalize(void) {
    int ret;

    /* Check if initialized */
    if (!g_initialized) {
        return TFTK_MPI_ERR_NOT_INITIALIZED;
    }

    /* Call MPI_Finalize through vtable */
    ret = tftk_mpi.finalize();

    /* Cleanup regardless of MPI_Finalize result */
    tftk_mpi_vtable_cleanup();

    if (g_handle) {
        tftk_mpi_loader_unload(g_handle);
        g_handle = NULL;
    }

    g_initialized = 0;
    g_backend_name = NULL;

    return (ret == 0) ? TFTK_MPI_OK : ret;
}

const char* tftk_mpi_get_backend_name(void) {
    return g_backend_name;
}

int tftk_mpi_is_initialized(void) {
    return g_initialized;
}
