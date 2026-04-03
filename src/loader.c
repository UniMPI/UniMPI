#include "tftk_mpi_loader.h"
#include "tftk_mpi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Backend definitions */
const tftk_mpi_backend_info_t tftk_mpi_backends[TFTK_MPI_MAX_BACKENDS] = {
    {TFTK_MPI_BACKEND_OPENMPI,   "openmpi",   "libmpi.so.40", 2},  /* Try .40 first, then .20 */
    {TFTK_MPI_BACKEND_MPICH,     "mpich",     "libmpich.so",  4},
    {TFTK_MPI_BACKEND_INTELMPI,  "intelmpi",  "libmpi.so",    3},
    {TFTK_MPI_BACKEND_MSMPI,     "msmpi",     "msmpi.dll",    5}   /* Windows only */
};

const char* tftk_mpi_loader_get_env_backend(void) {
    return getenv("TFTK_MPI_BACKEND");
}

const char* tftk_mpi_loader_get_env_libpath(void) {
    return getenv("TFTK_MPI_LIBRARY");
}

int tftk_mpi_loader_detect_backend(const char **out_lib_path) {
    const char *env_backend = tftk_mpi_loader_get_env_backend();
    const char *env_libpath = tftk_mpi_loader_get_env_libpath();

    /* Priority 1: User-specified library path */
    if (env_libpath) {
        *out_lib_path = env_libpath;
        return TFTK_MPI_OK;
    }

    /* Priority 2: User-specified backend name */
    if (env_backend) {
        for (int i = 0; i < TFTK_MPI_MAX_BACKENDS; i++) {
            if (strcmp(tftk_mpi_backends[i].name, env_backend) == 0) {
                *out_lib_path = tftk_mpi_backends[i].lib_name;
                return TFTK_MPI_OK;
            }
        }
        /* Backend name not recognized, try as library path */
        *out_lib_path = env_backend;
        return TFTK_MPI_OK;
    }

    /* Priority 3: Auto-detect - try common paths */
    /* For now, return first available backend */
    /* In production, would dlopen each and verify */
    *out_lib_path = "libmpi.so.40";  /* OpenMPI v4.x default */
    return TFTK_MPI_OK;
}

int tftk_mpi_loader_load(const char *lib_path, tftk_mpi_lib_handle_t *out_handle) {
    if (!lib_path) {
        return TFTK_MPI_ERR_NO_BACKEND;
    }

    tftk_mpi_lib_handle_t handle = tftk_mpi_platform_dlopen(lib_path);
    if (!handle) {
        return TFTK_MPI_ERR_BACKEND_LOAD;
    }

    *out_handle = handle;
    return TFTK_MPI_OK;
}

void tftk_mpi_loader_unload(tftk_mpi_lib_handle_t handle) {
    tftk_mpi_platform_dlclose(handle);
}

tftk_mpi_backend_type_t tftk_mpi_loader_identify_backend(tftk_mpi_lib_handle_t handle) {
    /* Try to identify by looking for backend-specific symbols */
    if (!handle) {
        return TFTK_MPI_BACKEND_UNKNOWN;
    }

    /* Check for OpenMPI-specific symbols */
    if (tftk_mpi_platform_dlsym(handle, "ompi_mpi_comm_world") != NULL) {
        return TFTK_MPI_BACKEND_OPENMPI;
    }

    /* Check for MPICH-specific symbols */
    if (tftk_mpi_platform_dlsym(handle, "MPIR_Comm_world") != NULL) {
        return TFTK_MPI_BACKEND_MPICH;
    }


    return TFTK_MPI_BACKEND_UNKNOWN;
}
