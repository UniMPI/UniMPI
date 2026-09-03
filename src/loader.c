#include "unimpi_loader.h"
#include "unimpi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Backend definitions */
const unimpi_backend_info_t unimpi_backends[UNIMPI_MAX_BACKENDS] = {
    {UNIMPI_BACKEND_OPENMPI,   "openmpi",   "libmpi.so.40", NULL,         2},  /* Try .40 first, then .20 */
    {UNIMPI_BACKEND_MPICH,     "mpich",     "libmpich.so",  "libmpi.so",  4},  /* Prefer libmpich.so; generic libmpi.so is alternatives-ambiguous */
    {UNIMPI_BACKEND_INTELMPI,  "intelmpi",  "libmpi.so",    NULL,         3},  /* Intel MPI uses libmpi.so */
    {UNIMPI_BACKEND_MSMPI,     "msmpi",     "msmpi.dll",    NULL,         5}   /* Windows only */
};

/* Check if a backend is supported on the current platform */
static int backend_supported_on_platform(unimpi_backend_type_t type) {
#ifdef _WIN32
    /* Windows only supports MS-MPI */
    return type == UNIMPI_BACKEND_MSMPI;
#elif defined(__APPLE__)
    /* macOS supports OpenMPI and MPICH */
    return type == UNIMPI_BACKEND_OPENMPI || type == UNIMPI_BACKEND_MPICH;
#elif defined(__linux__)
    /* Linux supports OpenMPI, MPICH, and Intel MPI */
    return type == UNIMPI_BACKEND_OPENMPI ||
           type == UNIMPI_BACKEND_MPICH ||
           type == UNIMPI_BACKEND_INTELMPI;
#else
    (void)type;
    return 0;
#endif
}

static const char* backend_name_from_type(unimpi_backend_type_t type) {
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        if (unimpi_backends[i].type == type) {
            return unimpi_backends[i].name;
        }
    }
    return "unknown";
}

static const char* get_nonempty_env(const char *name) {
    const char *value = getenv(name);
    return value && value[0] != '\0' ? value : NULL;
}

/* Check if the library path indicates a standard MPI ABI library */
static int path_names_standard_abi(const char *path) {
    if (!path) return 0;

    size_t length = strlen(path);
    char lowered[256];

    if (length >= sizeof(lowered)) {
        length = sizeof(lowered) - 1;
    }

    for (size_t i = 0; i < length; i++) {
        lowered[i] = (char)tolower((unsigned char)path[i]);
    }
    lowered[length] = '\0';

    return strstr(lowered, "mpi_abi") != NULL ||
           strstr(lowered, "mpi-abi") != NULL;
}


const char* unimpi_loader_get_env_backend(void) {
    return get_nonempty_env("UNIMPI_BACKEND");
}

const char* unimpi_loader_get_env_libpath(void) {
    return get_nonempty_env("UNIMPI_LIBRARY");
}

int unimpi_loader_detect_backend(const char **out_lib_path) {
    if (!out_lib_path) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }

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
    if (!out_handle) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }
    *out_handle = NULL;

    if (!lib_path) {
        fprintf(stderr, "[unimpi:ERROR] No backend library path provided\n");
        return UNIMPI_ERR_NO_BACKEND;
    }

    /* Reject standard MPI ABI libraries */
    if (path_names_standard_abi(lib_path)) {
        fprintf(stderr, "[unimpi:ERROR] Standard MPI ABI library is not supported: %s\n", lib_path);
        fprintf(stderr, "  unimpi requires a native MPI implementation (OpenMPI, MPICH, etc.)\n");
        return UNIMPI_ERR_ABI_MISMATCH;
    }

    fprintf(stderr, "[unimpi] Loading backend library: %s\n", lib_path);

    unimpi_lib_handle_t handle = unimpi_platform_dlopen(lib_path);
    if (!handle) {
        /* Fallback: a backend may prefer a specific library name (e.g. MPICH
         * prefers libmpich.so) but fall back to a generic name (libmpi.so)
         * when the preferred one is absent. */
        const char *alt = NULL;
        for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
            if (unimpi_backends[i].lib_name &&
                strcmp(unimpi_backends[i].lib_name, lib_path) == 0) {
                alt = unimpi_backends[i].lib_name_alt;
                break;
            }
        }
        if (alt) {
            fprintf(stderr, "[unimpi] %s not found, trying fallback %s\n", lib_path, alt);
            handle = unimpi_platform_dlopen(alt);
        }
    }
    if (!handle) {
        fprintf(stderr, "[unimpi:ERROR] Failed to load backend library: %s\n", lib_path);
        fprintf(stderr, "[unimpi:ERROR] %s\n", unimpi_platform_dlerror());
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

    /* Check for Intel MPI-specific symbols */
    if (unimpi_platform_dlsym(handle, "__I_MPI___cpu_core_type") != NULL) {
        fprintf(stderr, "[unimpi] Detected Intel MPI backend\n");
        return UNIMPI_BACKEND_INTELMPI;
    }

    /* Check for MPICH-specific symbols (e.g., MPIR_Err_create_code, MPIR_Dup_fn)
     * MPICH 4.x uses different internal symbols than older versions */
    if (unimpi_platform_dlsym(handle, "MPIR_Err_create_code") != NULL ||
        unimpi_platform_dlsym(handle, "MPIR_Dup_fn") != NULL) {
        fprintf(stderr, "[unimpi] Detected MPICH backend\n");
        return UNIMPI_BACKEND_MPICH;
    }

    /* Legacy check for older MPICH/Intel MPI versions */
    if (unimpi_platform_dlsym(handle, "MPIR_Comm_world") != NULL) {
        fprintf(stderr, "[unimpi] Detected MPICH backend (legacy interface)\n");
        return UNIMPI_BACKEND_MPICH;
    }

    fprintf(stderr, "[unimpi:WARN] Could not identify backend type\n");
    return UNIMPI_BACKEND_UNKNOWN;
}

int unimpi_loader_check_platform_support(unimpi_backend_type_t backend, const char *path) {
    /* Check if the backend is supported on this platform */
    if (!backend_supported_on_platform(backend)) {
        fprintf(stderr, "[unimpi:ERROR] Backend '%s' is not supported on this platform\n",
                backend_name_from_type(backend));
#ifdef _WIN32
        fprintf(stderr, "  Only MS-MPI is supported on Windows\n");
#elif defined(__APPLE__)
        fprintf(stderr, "  OpenMPI and MPICH are supported on macOS\n");
#elif defined(__linux__)
        fprintf(stderr, "  OpenMPI, MPICH, and Intel MPI are supported on Linux\n");
#endif
        return UNIMPI_ERR_BACKEND_NOT_SUPPORTED;
    }

    /* Additional check: reject standard ABI libraries for MPICH/IntelMPI */
    if ((backend == UNIMPI_BACKEND_MPICH || backend == UNIMPI_BACKEND_INTELMPI) &&
        path_names_standard_abi(path)) {
        fprintf(stderr, "[unimpi:ERROR] Standard MPI ABI library is not supported: %s\n", path);
        fprintf(stderr, "  unimpi requires native MPICH/Intel MPI libraries\n");
        return UNIMPI_ERR_ABI_MISMATCH;
    }

    return UNIMPI_OK;
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

        if (ret == UNIMPI_ERR_BACKEND_LOAD) {
            fprintf(stderr, "%s", unimpi_platform_load_advice());
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
