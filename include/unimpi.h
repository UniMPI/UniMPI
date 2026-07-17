#ifndef UNIMPI_H
#define UNIMPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "unimpi_vtable.h"

/* Version */
#define UNIMPI_VERSION_MAJOR 0
#define UNIMPI_VERSION_MINOR 1
#define UNIMPI_VERSION_PATCH 0

/* Error codes */
#define UNIMPI_OK 0
#define UNIMPI_ERR_NO_BACKEND -1
#define UNIMPI_ERR_BACKEND_LOAD -2
#define UNIMPI_ERR_ABI_MISMATCH -3
#define UNIMPI_ERR_NOT_INITIALIZED -4
#define UNIMPI_ERR_ALREADY_INITIALIZED -5
#define UNIMPI_ERR_SYMBOL_NOT_FOUND -6
#define UNIMPI_ERR_OUT_OF_MEMORY -7
#define UNIMPI_ERR_INVALID_ARGUMENT -8
#define UNIMPI_ERR_BACKEND_NOT_SUPPORTED -9
#define UNIMPI_ERR_BACKEND_INIT_FAILED -10

/* Debug levels */
#define UNIMPI_DBG_NONE 0
#define UNIMPI_DBG_ERROR 1
#define UNIMPI_DBG_WARN 2
#define UNIMPI_DBG_INFO 3
#define UNIMPI_DBG_DEBUG 4
#define UNIMPI_DBG_TRACE 5

/* Initialization and finalization */
int unimpi_init(int *argc, char ***argv);
int unimpi_init_with(const char *backend_name);
int unimpi_finalize(void);

/* Query functions */
int unimpi_is_initialized(void);
const char* unimpi_get_backend_name(void);

/* Error handling */
const char* unimpi_error_string(int error_code);
int unimpi_error_class(int error_code, int *error_class);

/* Diagnostics */
void unimpi_set_debug_level(int level);
int unimpi_get_debug_level(void);
void unimpi_debug_print(const char *fmt, ...);
void unimpi_diagnose_backend(const char *lib_path);
int unimpi_print_backend_info(void);

/* Optional standard MPI macros */
#ifdef UNIMPI_USE_STD_NAMES
#include "unimpi_std_macros.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* UNIMPI_H */
