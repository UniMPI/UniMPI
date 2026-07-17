#include "unimpi.h"
#include <stdio.h>
#include <stdarg.h>

/* Global debug level */
static int g_debug_level = UNIMPI_DBG_NONE;

const char* unimpi_error_string(int error_code) {
    switch (error_code) {
        case UNIMPI_OK:
            return "Success";
        case UNIMPI_ERR_NO_BACKEND:
            return "No MPI backend found";
        case UNIMPI_ERR_BACKEND_LOAD:
            return "Failed to load MPI backend library";
        case UNIMPI_ERR_ABI_MISMATCH:
            return "MPI ABI version mismatch";
        case UNIMPI_ERR_NOT_INITIALIZED:
            return "TFTK-MPI not initialized";
        case UNIMPI_ERR_ALREADY_INITIALIZED:
            return "TFTK-MPI already initialized";
        case UNIMPI_ERR_SYMBOL_NOT_FOUND:
            return "Required MPI symbol not found in backend";
        case UNIMPI_ERR_OUT_OF_MEMORY:
            return "Out of memory";
        case UNIMPI_ERR_INVALID_ARGUMENT:
            return "Invalid argument";
        case UNIMPI_ERR_BACKEND_NOT_SUPPORTED:
            return "Backend not supported on this platform";
        case UNIMPI_ERR_BACKEND_INIT_FAILED:
            return "Backend initialization failed";
        default:
            return "Unknown error";
    }
}

int unimpi_error_class(int error_code, int *error_class) {
    /* Map TFTK errors to MPI error classes */
    switch (error_code) {
        case UNIMPI_OK:
            *error_class = 0; /* MPI_SUCCESS */
            break;
        case UNIMPI_ERR_INVALID_ARGUMENT:
            *error_class = 2; /* MPI_ERR_ARG */
            break;
        case UNIMPI_ERR_NO_BACKEND:
        case UNIMPI_ERR_BACKEND_LOAD:
        case UNIMPI_ERR_BACKEND_INIT_FAILED:
            *error_class = 15; /* MPI_ERR_OTHER */
            break;
        default:
            *error_class = 15; /* MPI_ERR_OTHER */
            break;
    }
    return UNIMPI_OK;
}

void unimpi_set_debug_level(int level) {
    g_debug_level = level;
}

int unimpi_get_debug_level(void) {
    return g_debug_level;
}

static const char* debug_level_str(int level) {
    switch (level) {
        case UNIMPI_DBG_ERROR: return "ERROR";
        case UNIMPI_DBG_WARN:  return "WARN";
        case UNIMPI_DBG_INFO:  return "INFO";
        case UNIMPI_DBG_DEBUG: return "DEBUG";
        case UNIMPI_DBG_TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

void unimpi_debug_print(const char *fmt, ...) {
    va_list args;

    /* Check environment variable for debug level */
    const char *env_debug = getenv("UNIMPI_DEBUG");
    int level = UNIMPI_DBG_INFO;
    if (env_debug) {
        level = atoi(env_debug);
    } else {
        level = g_debug_level;
    }

    if (level < UNIMPI_DBG_INFO) {
        return;
    }

    fprintf(stderr, "[unimpi] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void unimpi_debug_print_level(int level, const char *fmt, ...) {
    va_list args;

    /* Check environment variable for debug level */
    const char *env_debug = getenv("UNIMPI_DEBUG");
    int current_level = g_debug_level;
    if (env_debug) {
        current_level = atoi(env_debug);
    }

    if (level > current_level) {
        return;
    }

    fprintf(stderr, "[unimpi:%s] ", debug_level_str(level));
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
