#ifndef UNIMPI_H
#define UNIMPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "unimpi_export.h"
#include "unimpi_version.h"
#include "unimpi_vtable.h"
#include "unimpi_loader.h"
#include "unimpi_errors.h"

/* Compatibility typedef - UNIMPI_Status is the same as MPI_Status */
typedef MPI_Status UNIMPI_Status;

/* UniMPI library version macros live in unimpi_version.h (single source of
 * truth); they are re-exported here via the #include at the top of this file.
 * UNIMPI_MPI_VERSION / UNIMPI_MPI_SUBVERSION below are the MPI standard
 * revision the stable facade targets - unrelated to the library version. */

/* MPI profile version exposed by the stable facade */
#define UNIMPI_MPI_VERSION 3
#define UNIMPI_MPI_SUBVERSION 0

/* Fixed buffer sizes */
#define UNIMPI_MAX_PROCESSOR_NAME 256
#define UNIMPI_MAX_LIBRARY_VERSION_STRING 8192

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
#define UNIMPI_ERR_FINALIZED -11
#define UNIMPI_ERR_INVALID_STATE -12

/* Debug levels */
#define UNIMPI_DBG_NONE 0
#define UNIMPI_DBG_ERROR 1
#define UNIMPI_DBG_WARN 2
#define UNIMPI_DBG_INFO 3
#define UNIMPI_DBG_DEBUG 4
#define UNIMPI_DBG_TRACE 5

/* Thread support levels */
#define UNIMPI_THREAD_SINGLE 0
#define UNIMPI_THREAD_FUNNELED 1
#define UNIMPI_THREAD_SERIALIZED 2
#define UNIMPI_THREAD_MULTIPLE 3

/* Initialization and finalization */
UNIMPI_API int unimpi_init(int *argc, char ***argv);
UNIMPI_API int unimpi_init_thread(int *argc, char ***argv, int required, int *provided);
UNIMPI_API int unimpi_init_with(const char *backend_name);
UNIMPI_API int unimpi_finalize(void);
UNIMPI_API int unimpi_mpi_initialized(int *flag);
UNIMPI_API int unimpi_mpi_finalized(int *flag);
UNIMPI_API int unimpi_mpi_get_version(int *version, int *subversion);
UNIMPI_API int unimpi_mpi_get_library_version(char *version, int *resultlen);

/* Query functions */
UNIMPI_API int unimpi_is_initialized(void);
UNIMPI_API const char* unimpi_get_backend_name(void);
UNIMPI_API const char* unimpi_get_library_path(void);
UNIMPI_API const char* unimpi_get_version(void);   /* returns UNIMPI_VERSION_STRING */

/* Error handling */
UNIMPI_API const char* unimpi_error_string(int error_code);
UNIMPI_API int unimpi_error_class(int error_code, int *error_class);
UNIMPI_API const char* unimpi_mpi_error_string(int error_code);

/* Diagnostics */
UNIMPI_API void unimpi_set_debug_level(int level);
UNIMPI_API int unimpi_get_debug_level(void);
UNIMPI_API void unimpi_debug_print(const char *fmt, ...);
UNIMPI_API void unimpi_diagnose_backend(const char *lib_path);
UNIMPI_API int unimpi_print_backend_info(void);

/* Optional standard MPI macros */
#ifdef UNIMPI_USE_STD_NAMES
#include "unimpi_std_macros.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* UNIMPI_H */
