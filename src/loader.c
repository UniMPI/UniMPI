#include "unimpi_loader.h"
#include "unimpi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Backend definitions */
const unimpi_backend_info_t unimpi_backends[UNIMPI_MAX_BACKENDS] = {
    {UNIMPI_BACKEND_OPENMPI,   "openmpi",   "libmpi.so.40", 2},  /* Try .40 first, then .20 */
    {UNIMPI_BACKEND_MPICH,     "mpich",     "libmpich.so",  4},
    {UNIMPI_BACKEND_INTELMPI,  "intelmpi",  "libmpi.so",    3},
    {UNIMPI_BACKEND_MSMPI,     "msmpi",     "msmpi.dll",    5}   /* Windows only */
};

const char* unimpi_loader_get_env_backend(void) {
    return getenv("UNIMPI_BACKEND");
}

const char* unimpi_loader_get_env_libpath(void) {
    return getenv("UNIMPI_LIBRARY");
}

int unimpi_loader_detect_backend(const char **out_lib_path) {
    const char *env_backend = unimpi_loader_get_env_backend();
    const char *env_libpath = unimpi_loader_get_env_libpath();

    /* Priority 1: User-specified library path */
    if (env_libpath) {
        fprintf(stderr, "[unimpi] Using library path from UNIMPI_LIBRARY: %s\n", env_libpath);
        *out_lib_path = env_libpath;
        return UNIMPI_OK;
    }

    /* Priority 2: User-specified backend name */
    if (env_backend) {
        fprintf(stderr, "[unimpi] Using backend from UNIMPI_BACKEND: %s\n", env_backend);
        for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
            if (strcmp(unimpi_backends[i].name, env_backend) == 0) {
                *out_lib_path = unimpi_backends[i].lib_name;
                return UNIMPI_OK;
            }
        }
        /* Backend name not recognized, try as library path */
        *out_lib_path = env_backend;
        return UNIMPI_OK;
    }

    /* Priority 3: Auto-detect - try common paths */
    fprintf(stderr, "[unimpi] Auto-detecting MPI backend...\n");
#ifdef _WIN32
    /* On Windows, try MS-MPI first */
    *out_lib_path = "msmpi.dll";
#else
    /* On Linux, try OpenMPI first */
    *out_lib_path = "libmpi.so.40";  /* OpenMPI v4.x default */
#endif
    return UNIMPI_OK;
}

int unimpi_loader_load(const char *lib_path, unimpi_lib_handle_t *out_handle) {
    if (!lib_path) {
        fprintf(stderr, "[unimpi:ERROR] No backend library path provided\n");
        return UNIMPI_ERR_NO_BACKEND;
    }

    fprintf(stderr, "[unimpi] Loading backend library: %s\n", lib_path);

    unimpi_lib_handle_t handle = unimpi_platform_dlopen(lib_path);
    if (!handle) {
        fprintf(stderr, "[unimpi:ERROR] Failed to load backend library: %s\n", lib_path);
        return UNIMPI_ERR_BACKEND_LOAD;
    }

    fprintf(stderr, "[unimpi] Successfully loaded backend library\n");

    *out_handle = handle;
    return UNIMPI_OK;
}

void unimpi_loader_unload(unimpi_lib_handle_t handle) {
    unimpi_platform_dlclose(handle);
}

unimpi_backend_type_t unimpi_loader_identify_backend(unimpi_lib_handle_t handle) {
    /* Try to identify by looking for backend-specific symbols */
    if (!handle) {
        fprintf(stderr, "[unimpi:WARN] Cannot identify backend: null handle\n");
        return UNIMPI_BACKEND_UNKNOWN;
    }

    fprintf(stderr, "[unimpi] Identifying backend type...\n");

    /* Check for MS-MPI first (Windows-specific) */
#ifdef _WIN32
    if (unimpi_platform_dlsym(handle, "MSMPI_Get_version") != NULL) {
        fprintf(stderr, "[unimpi] Detected MS-MPI backend\n");
        return UNIMPI_BACKEND_MSMPI;
    }
#endif

    /* Check for OpenMPI-specific symbols */
    if (unimpi_platform_dlsym(handle, "ompi_mpi_comm_world") != NULL) {
        fprintf(stderr, "[unimpi] Detected OpenMPI backend\n");
        return UNIMPI_BACKEND_OPENMPI;
    }

    /* Check for MPICH-specific symbols (also used by Intel MPI) */
    if (unimpi_platform_dlsym(handle, "MPIR_Comm_world") != NULL) {
        /* Check if it's actually Intel MPI by looking for Intel-specific symbols */
        if (unimpi_platform_dlsym(handle, "__I_MPI___cpu_core_type") != NULL) {
            fprintf(stderr, "[unimpi] Detected Intel MPI backend\n");
            return UNIMPI_BACKEND_INTELMPI;
        }
        fprintf(stderr, "[unimpi] Detected MPICH backend\n");
        return UNIMPI_BACKEND_MPICH;
    }

    /* Additional check for Intel MPI - look for I_MPI specific symbols */
    if (unimpi_platform_dlsym(handle, "__I_MPI___cpu_core_type") != NULL) {
        fprintf(stderr, "[unimpi] Detected Intel MPI backend (alternative detection)\n");
        return UNIMPI_BACKEND_INTELMPI;
    }

    fprintf(stderr, "[unimpi:WARN] Could not identify backend type\n");
    return UNIMPI_BACKEND_UNKNOWN;
}

void unimpi_diagnose_backend(const char *lib_path) {
    fprintf(stderr, "\n[unimpi] === Backend Diagnostics ===\n");
    fprintf(stderr, "Library path: %s\n", lib_path ? lib_path : "(null)");

    /* Check environment variables */
    const char *env_backend = getenv("UNIMPI_BACKEND");
    const char *env_libpath = getenv("UNIMPI_LIBRARY");
    fprintf(stderr, "UNIMPI_BACKEND: %s\n", env_backend ? env_backend : "(not set)");
    fprintf(stderr, "UNIMPI_LIBRARY: %s\n", env_libpath ? env_libpath : "(not set)");

    /* Try to load and identify */
    unimpi_lib_handle_t handle;
    int ret = unimpi_loader_load(lib_path, &handle);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to load backend: %s\n", unimpi_error_string(ret));

        /* Try to provide more specific advice */
        if (ret == UNIMPI_ERR_BACKEND_LOAD) {
            fprintf(stderr, "\nTroubleshooting:\n");
            fprintf(stderr, "1. Check if the library exists: ls -la %s\n", lib_path);
            fprintf(stderr, "2. Check library dependencies: ldd %s\n", lib_path);
            fprintf(stderr, "3. Ensure LD_LIBRARY_PATH includes the MPI library directory\n");
        }
        return;
    }

    /* Identify backend */
    unimpi_backend_type_t type = unimpi_loader_identify_backend(handle);
    fprintf(stderr, "Identified backend type: ");
    switch (type) {
        case UNIMPI_BACKEND_OPENMPI:   fprintf(stderr, "OpenMPI\n"); break;
        case UNIMPI_BACKEND_MPICH:   fprintf(stderr, "MPICH\n"); break;
        case UNIMPI_BACKEND_INTELMPI: fprintf(stderr, "Intel MPI\n"); break;
        case UNIMPI_BACKEND_MSMPI:   fprintf(stderr, "MS-MPI\n"); break;
        default:                       fprintf(stderr, "Unknown\n"); break;
    }

    /* Check for required symbols */
    fprintf(stderr, "\nChecking required symbols:\n");
    const char *required_symbols[] = {
        "MPI_Init",
        "MPI_Finalize",
        "MPI_Comm_size",
        "MPI_Comm_rank",
        "MPI_Send",
        "MPI_Recv",
        NULL
    };

    for (int i = 0; required_symbols[i] != NULL; i++) {
        void *sym = unimpi_platform_dlsym(handle, required_symbols[i]);
        fprintf(stderr, "  %s: %s\n", required_symbols[i], sym ? "OK" : "NOT FOUND");
    }

    unimpi_loader_unload(handle);
    fprintf(stderr, "\n=== End Diagnostics ===\n\n");
}

int unimpi_print_backend_info(void) {
    fprintf(stderr, "\n[unimpi] Supported Backends:\n");
    fprintf(stderr, "  %-10s %s\n", "Name", "Library Name");
    fprintf(stderr, "  %-10s %s\n", "----------", "------------");
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        fprintf(stderr, "  %-10s %s\n", unimpi_backends[i].name, unimpi_backends[i].lib_name);
    }
    fprintf(stderr, "\n");
    return UNIMPI_OK;
}
