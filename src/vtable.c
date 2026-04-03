#include "tftk_mpi_vtable.h"
#include "tftk_mpi_loader.h"
#include "tftk_mpi_platform.h"
#include "tftk_mpi.h"
#include <stdlib.h>
#include <string.h>

/* Global vtable instance - initialized to zeros */
tftk_mpi_vtable_t tftk_mpi = {0};

/* MPI predefined values - filled at runtime */
MPI_Comm TFTK_MPI_COMM_WORLD = 0;
MPI_Comm TFTK_MPI_COMM_SELF = 0;

static tftk_mpi_lib_handle_t g_backend_handle = NULL;
static tftk_mpi_backend_type_t g_backend_type = TFTK_MPI_BACKEND_UNKNOWN;

/* Forward declarations */
int tftk_mpi_vtable_init_openmpi(tftk_mpi_lib_handle_t handle);
int tftk_mpi_vtable_init_mpich(tftk_mpi_lib_handle_t handle);

/* Get current backend type */
tftk_mpi_backend_type_t tftk_mpi_get_backend_type(void) {
    return g_backend_type;
}

/* Core symbol loading helper */
static void* load_symbol(tftk_mpi_lib_handle_t handle, const char *name) {
    return tftk_mpi_platform_dlsym(handle, name);
}

/* Validate required symbols exist */
int tftk_mpi_vtable_validate_core(tftk_mpi_lib_handle_t handle) {
    if (!load_symbol(handle, "MPI_Init") ||
        !load_symbol(handle, "MPI_Finalize") ||
        !load_symbol(handle, "MPI_Comm_size") ||
        !load_symbol(handle, "MPI_Comm_rank")) {
        return TFTK_MPI_ERR_SYMBOL_NOT_FOUND;
    }
    return TFTK_MPI_OK;
}

/* Initialize vtable based on backend type */
int tftk_mpi_vtable_init(tftk_mpi_lib_handle_t handle) {
    int ret;

    /* Validate core symbols */
    ret = tftk_mpi_vtable_validate_core(handle);
    if (ret != TFTK_MPI_OK) {
        return ret;
    }

    /* Identify backend */
    g_backend_type = tftk_mpi_loader_identify_backend(handle);
    g_backend_handle = handle;

    /* Initialize based on backend type */
    switch (g_backend_type) {
        case TFTK_MPI_BACKEND_OPENMPI:
        case TFTK_MPI_BACKEND_INTELMPI:  /* Intel-MPI is OpenMPI-compatible */
            return tftk_mpi_vtable_init_openmpi(handle);
        case TFTK_MPI_BACKEND_MPICH:
        case TFTK_MPI_BACKEND_MSMPI:     /* MS-MPI is MPICH-derived */
            return tftk_mpi_vtable_init_mpich(handle);
        default:
            /* Try generic initialization */
            return tftk_mpi_vtable_init_openmpi(handle);
    }
}

/* Cleanup vtable */
void tftk_mpi_vtable_cleanup(void) {
    memset(&tftk_mpi, 0, sizeof(tftk_mpi_vtable_t));
    TFTK_MPI_COMM_WORLD = 0;
    TFTK_MPI_COMM_SELF = 0;
    g_backend_type = TFTK_MPI_BACKEND_UNKNOWN;
    g_backend_handle = NULL;
}
