#include "unimpi.h"
#include "unimpi_vtable.h"
#include "unimpi_loader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Lifecycle states for state machine */
typedef enum {
    UNIMPI_STATE_NEVER = 0,
    UNIMPI_STATE_INITIALIZING,
    UNIMPI_STATE_ACTIVE,
    UNIMPI_STATE_INIT_FAILED,
    UNIMPI_STATE_FINALIZING,
    UNIMPI_STATE_FINALIZED,
    UNIMPI_STATE_FINALIZE_FAILED
} unimpi_state_t;

/* Backend identity information */
typedef struct {
    unimpi_backend_type_t backend_type;
    char *backend_name;
    char *library_path;
} unimpi_backend_identity_t;

/* Global state */
static unimpi_state_t g_state = UNIMPI_STATE_NEVER;
static unimpi_backend_identity_t g_backend_identity = {
    UNIMPI_BACKEND_UNKNOWN,
    NULL,
    NULL
};
static unimpi_lib_handle_t g_handle = NULL;

/* Version info cache */
static int g_mpi_version = UNIMPI_MPI_VERSION;
static int g_mpi_subversion = UNIMPI_MPI_SUBVERSION;
static char g_mpi_library_version[UNIMPI_MAX_LIBRARY_VERSION_STRING] =
    "unimpi 0.1.0 (no active MPI backend)";
static int g_mpi_library_version_length =
    (int)(sizeof("unimpi 0.1.0 (no active MPI backend)") - 1);

/* Thread level if initialized with unimpi_init_thread */
static int g_thread_level = UNIMPI_THREAD_SINGLE;

/* Helper: Copy string to dynamically allocated memory */
static char* copy_string(const char *value) {
    if (!value) {
        return NULL;
    }
    size_t length = strlen(value) + 1;
    char *copy = (char*)malloc(length);
    if (copy) {
        memcpy(copy, value, length);
    }
    return copy;
}

/* Helper: Clear backend identity */
static void clear_backend_identity(void) {
    free(g_backend_identity.backend_name);
    free(g_backend_identity.library_path);
    g_backend_identity.backend_type = UNIMPI_BACKEND_UNKNOWN;
    g_backend_identity.backend_name = NULL;
    g_backend_identity.library_path = NULL;
}

/* Helper: Save backend identity */
static int save_backend_identity(unimpi_backend_type_t backend_type,
                                   const char *library_path) {
    const char *backend_name = NULL;

    /* Look up backend name from type */
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        if (unimpi_backends[i].type == backend_type) {
            backend_name = unimpi_backends[i].name;
            break;
        }
    }

    clear_backend_identity();
    g_backend_identity.backend_type = backend_type;
    g_backend_identity.backend_name = copy_string(backend_name);
    g_backend_identity.library_path = copy_string(library_path);
    return UNIMPI_OK;
}

/* Check if we can initialize (state validation) */
static int check_can_initialize(void) {
    switch (g_state) {
        case UNIMPI_STATE_NEVER:
            return UNIMPI_OK;
        case UNIMPI_STATE_INITIALIZING:
            return UNIMPI_ERR_ALREADY_INITIALIZED;
        case UNIMPI_STATE_ACTIVE:
            return UNIMPI_ERR_ALREADY_INITIALIZED;
        case UNIMPI_STATE_INIT_FAILED:
            /* Allow retry after init failure */
            return UNIMPI_OK;
        case UNIMPI_STATE_FINALIZED:
            /* MPI standard: cannot re-initialize after finalize */
            return UNIMPI_ERR_FINALIZED;
        case UNIMPI_STATE_FINALIZING:
            return UNIMPI_ERR_INVALID_STATE;
        case UNIMPI_STATE_FINALIZE_FAILED:
            return UNIMPI_ERR_INVALID_STATE;
        default:
            return UNIMPI_ERR_INVALID_STATE;
    }
}

/* Internal initialization function with thread support */
static int initialize_backend(int *argc, char ***argv,
                               int thread_mode, int required_level, int *provided_level) {
    const char *lib_path;
    int ret;

    /* Detect backend */
    ret = unimpi_loader_detect_backend(&lib_path);
    if (ret != UNIMPI_OK) {
        g_state = UNIMPI_STATE_INIT_FAILED;
        return ret;
    }

    /* Load backend library */
    ret = unimpi_loader_load(lib_path, &g_handle);
    if (ret != UNIMPI_OK) {
        g_state = UNIMPI_STATE_INIT_FAILED;
        return ret;
    }

    /* Initialize vtable */
    ret = unimpi_vtable_init(g_handle);
    if (ret != UNIMPI_OK) {
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
        g_state = UNIMPI_STATE_INIT_FAILED;
        return ret;
    }

    /* Save backend identity */
    save_backend_identity(unimpi_get_backend_type(), lib_path);

    /* Call MPI_Init or MPI_Init_thread through vtable */
    if (thread_mode && unimpi.init_thread) {
        ret = unimpi.init_thread(argc, argv, required_level, provided_level);
        if (provided_level) {
            *provided_level = required_level; /* Assume we get what we ask for */
        }
    } else {
        ret = unimpi.init(argc, argv);
        if (thread_mode && provided_level) {
            *provided_level = UNIMPI_THREAD_SINGLE;
        }
    }

    if (ret != 0) {
        unimpi_vtable_cleanup();
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
        clear_backend_identity();
        g_state = UNIMPI_STATE_INIT_FAILED;
        return UNIMPI_ERR_BACKEND_LOAD;
    }

    /* Set active state */
    g_state = UNIMPI_STATE_ACTIVE;
    if (thread_mode) {
        g_thread_level = required_level;
    }

    return UNIMPI_OK;
}

int unimpi_init(int *argc, char ***argv) {
    int ret;

    ret = check_can_initialize();
    if (ret != UNIMPI_OK) {
        return ret;
    }

    g_state = UNIMPI_STATE_INITIALIZING;
    ret = initialize_backend(argc, argv, 0, UNIMPI_THREAD_SINGLE, NULL);

    return ret;
}

int unimpi_init_thread(int *argc, char ***argv, int required, int *provided) {
    int ret;

    /* Validate required thread level */
    if (required != UNIMPI_THREAD_SINGLE &&
        required != UNIMPI_THREAD_FUNNELED &&
        required != UNIMPI_THREAD_SERIALIZED &&
        required != UNIMPI_THREAD_MULTIPLE) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }

    if (!provided) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }

    ret = check_can_initialize();
    if (ret != UNIMPI_OK) {
        return ret;
    }

    g_state = UNIMPI_STATE_INITIALIZING;
    ret = initialize_backend(argc, argv, 1, required, provided);

    return ret;
}

int unimpi_init_with(const char *backend_name) {
    int ret;
    const char *lib_path = NULL;

    if (!backend_name) {
        return UNIMPI_ERR_NO_BACKEND;
    }

    ret = check_can_initialize();
    if (ret != UNIMPI_OK) {
        return ret;
    }

    g_state = UNIMPI_STATE_INITIALIZING;

    /* Find library path for the specified backend */
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        if (strcmp(unimpi_backends[i].name, backend_name) == 0) {
            lib_path = unimpi_backends[i].lib_name;
            break;
        }
    }

    /* If backend name not recognized, try using it directly as library path */
    if (!lib_path) {
        lib_path = backend_name;
    }

    fprintf(stderr, "[unimpi] Initializing with backend: %s (library: %s)\n", backend_name, lib_path);

    /* Load backend library */
    ret = unimpi_loader_load(lib_path, &g_handle);
    if (ret != UNIMPI_OK) {
        g_state = UNIMPI_STATE_INIT_FAILED;
        return ret;
    }

    /* Initialize vtable */
    ret = unimpi_vtable_init(g_handle);
    if (ret != UNIMPI_OK) {
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
        g_state = UNIMPI_STATE_INIT_FAILED;
        return ret;
    }

    /* Save backend identity */
    save_backend_identity(unimpi_get_backend_type(), lib_path);

    /* Call MPI_Init through vtable */
    ret = unimpi.init(NULL, NULL);
    if (ret != 0) {
        unimpi_vtable_cleanup();
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
        clear_backend_identity();
        g_state = UNIMPI_STATE_INIT_FAILED;
        return UNIMPI_ERR_BACKEND_LOAD;
    }

    g_state = UNIMPI_STATE_ACTIVE;
    return UNIMPI_OK;
}

int unimpi_finalize(void) {
    int ret;

    /* Check state */
    if (g_state == UNIMPI_STATE_NEVER ||
        g_state == UNIMPI_STATE_INIT_FAILED) {
        return UNIMPI_ERR_NOT_INITIALIZED;
    }
    if (g_state == UNIMPI_STATE_FINALIZED) {
        return UNIMPI_ERR_FINALIZED;
    }
    if (g_state != UNIMPI_STATE_ACTIVE) {
        return UNIMPI_ERR_INVALID_STATE;
    }

    g_state = UNIMPI_STATE_FINALIZING;

    /* Call MPI_Finalize through vtable */
    ret = unimpi.finalize();

    /* Cleanup regardless of MPI_Finalize result */
    unimpi_vtable_cleanup();

    if (g_handle) {
        unimpi_loader_unload(g_handle);
        g_handle = NULL;
    }

    clear_backend_identity();
    g_thread_level = UNIMPI_THREAD_SINGLE;

    if (ret == 0) {
        g_state = UNIMPI_STATE_FINALIZED;
        return UNIMPI_OK;
    } else {
        g_state = UNIMPI_STATE_FINALIZE_FAILED;
        return ret;
    }
}

int unimpi_mpi_initialized(int *flag) {
    if (!flag) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }
    /* MPI_Initialized returns true if MPI was ever initialized */
    *flag = (g_state == UNIMPI_STATE_ACTIVE ||
             g_state == UNIMPI_STATE_FINALIZING ||
             g_state == UNIMPI_STATE_FINALIZED ||
             g_state == UNIMPI_STATE_FINALIZE_FAILED) ? 1 : 0;
    return UNIMPI_OK;
}

int unimpi_mpi_finalized(int *flag) {
    if (!flag) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }
    *flag = (g_state == UNIMPI_STATE_FINALIZED) ? 1 : 0;
    return UNIMPI_OK;
}

int unimpi_mpi_get_version(int *version, int *subversion) {
    if (!version || !subversion) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }
    *version = g_mpi_version;
    *subversion = g_mpi_subversion;
    return UNIMPI_OK;
}

int unimpi_mpi_get_library_version(char *version, int *resultlen) {
    if (!version || !resultlen) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }
    int len = g_mpi_library_version_length;
    if (len >= UNIMPI_MAX_LIBRARY_VERSION_STRING) {
        len = UNIMPI_MAX_LIBRARY_VERSION_STRING - 1;
    }
    memcpy(version, g_mpi_library_version, len);
    version[len] = '\0';
    *resultlen = len;
    return UNIMPI_OK;
}

const char* unimpi_get_backend_name(void) {
    return g_backend_identity.backend_name;
}

const char* unimpi_get_library_path(void) {
    return g_backend_identity.library_path;
}

int unimpi_is_initialized(void) {
    return (g_state == UNIMPI_STATE_ACTIVE) ? 1 : 0;
}

