#include "unimpi.h"
#include "unimpi_vtable.h"
#include "unimpi_loader.h"
#include <stdlib.h>

/* Global state */
static int g_initialized = 0;
static const char *g_backend_name = NULL;
static unimpi_lib_handle_t g_handle = NULL;

int unimpi_init(int *argc, char ***argv) {
    int ret;
    const char *lib_path;

    /* Check if already initialized */
    if (g_initialized) {
        return UNIMPI_ERR_ALREADY_INITIALIZED;
    }

    /* Detect backend */
    ret = unimpi_loader_detect_backend(&lib_path);
    if (ret != UNIMPI_OK) {
        return ret;
    }

    /* Load backend library */
    ret = unimpi_loader_load(lib_path, &g_handle);
    if (ret != UNIMPI_OK) {
        return ret;
    }

    /* Initialize vtable */
    ret = unimpi_vtable_init(g_handle);
    if (ret != UNIMPI_OK) {
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
        return ret;
    }

    /* Call MPI_Init through vtable */
    ret = unimpi.init(argc, argv);
    if (ret != 0) {
        unimpi_vtable_cleanup();
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
        return UNIMPI_ERR_BACKEND_LOAD;
    }

    /* Set initialized state */
    g_initialized = 1;
    g_backend_name = lib_path;

    return UNIMPI_OK;
}

int unimpi_init_with(const char *backend_name) {
    /* TODO: Implement backend-specific initialization */
    /* For now, set environment and call regular init */
    setenv("UNIMPI_BACKEND", backend_name, 1);
    return unimpi_init(NULL, NULL);
}

int unimpi_finalize(void) {
    int ret;

    /* Check if initialized */
    if (!g_initialized) {
        return UNIMPI_ERR_NOT_INITIALIZED;
    }

    /* Call MPI_Finalize through vtable */
    ret = unimpi.finalize();

    /* Cleanup regardless of MPI_Finalize result */
    unimpi_vtable_cleanup();

    if (g_handle) {
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
    }

    g_initialized = 0;
    g_backend_name = NULL;

    return (ret == 0) ? UNIMPI_OK : ret;
}

const char* unimpi_get_backend_name(void) {
    return g_backend_name;
}

int unimpi_is_initialized(void) {
    return g_initialized;
}
