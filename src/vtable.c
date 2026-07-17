#include "unimpi_vtable.h"
#include "unimpi_loader.h"
#include "unimpi_platform.h"
#include "unimpi.h"
#include <stdlib.h>
#include <string.h>

/* Global vtable instance - initialized to zeros */
unimpi_vtable_t unimpi = {0};

/* MPI predefined values - filled at runtime */
MPI_Comm UNIMPI_COMM_WORLD = 0;
MPI_Comm UNIMPI_COMM_SELF = 0;

static unimpi_lib_handle_t g_backend_handle = NULL;
static unimpi_backend_type_t g_backend_type = UNIMPI_BACKEND_UNKNOWN;

/* Forward declarations */
int unimpi_vtable_init_openmpi(unimpi_lib_handle_t handle);
int unimpi_vtable_init_mpich(unimpi_lib_handle_t handle);
int unimpi_vtable_init_intelmpi(unimpi_lib_handle_t handle);

/* Get current backend type */
unimpi_backend_type_t unimpi_get_backend_type(void) {
    return g_backend_type;
}

/* Core symbol loading helper */
static void* load_symbol(unimpi_lib_handle_t handle, const char *name) {
    return unimpi_platform_dlsym(handle, name);
}

/* Validate required symbols exist */
int unimpi_vtable_validate_core(unimpi_lib_handle_t handle) {
    if (!load_symbol(handle, "MPI_Init") ||
        !load_symbol(handle, "MPI_Finalize") ||
        !load_symbol(handle, "MPI_Comm_size") ||
        !load_symbol(handle, "MPI_Comm_rank")) {
        return UNIMPI_ERR_SYMBOL_NOT_FOUND;
    }
    return UNIMPI_OK;
}

/* Initialize vtable based on backend type */
int unimpi_vtable_init(unimpi_lib_handle_t handle) {
    int ret;

    /* Validate core symbols */
    ret = unimpi_vtable_validate_core(handle);
    if (ret != UNIMPI_OK) {
        return ret;
    }

    /* Identify backend */
    g_backend_type = unimpi_loader_identify_backend(handle);
    g_backend_handle = handle;

    /* Initialize based on backend type */
    switch (g_backend_type) {
        case UNIMPI_BACKEND_OPENMPI:
            return unimpi_vtable_init_openmpi(handle);
        case UNIMPI_BACKEND_INTELMPI:
            return unimpi_vtable_init_intelmpi(handle);
        case UNIMPI_BACKEND_MPICH:
        case UNIMPI_BACKEND_MSMPI:     /* MS-MPI is MPICH-derived */
            return unimpi_vtable_init_mpich(handle);
        default:
            /* Try generic initialization - use MPICH style as default */
            return unimpi_vtable_init_mpich(handle);
    }
}

/* Cleanup vtable */
void unimpi_vtable_cleanup(void) {
    memset(&unimpi, 0, sizeof(unimpi_vtable_t));
    UNIMPI_COMM_WORLD = 0;
    UNIMPI_COMM_SELF = 0;
    g_backend_type = UNIMPI_BACKEND_UNKNOWN;
    g_backend_handle = NULL;
}
