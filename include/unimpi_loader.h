#ifndef UNIMPI_LOADER_H
#define UNIMPI_LOADER_H

#include "unimpi_platform.h"

/* Backend types */
typedef enum {
    UNIMPI_BACKEND_UNKNOWN = 0,
    UNIMPI_BACKEND_OPENMPI,
    UNIMPI_BACKEND_MPICH,
    UNIMPI_BACKEND_INTELMPI,
    UNIMPI_BACKEND_MSMPI
} unimpi_backend_type_t;


/* Backend info */
typedef struct {
    unimpi_backend_type_t type;
    const char *name;
    const char *lib_name;
    const char *lib_name_alt;   /* fallback when lib_name fails to dlopen (NULL = none) */
    int priority;
} unimpi_backend_info_t;

/* Backend detection and loading API */
const char* unimpi_loader_get_env_backend(void);
int unimpi_loader_detect_backend(const char **out_lib_path);
int unimpi_loader_load(const char *lib_path, unimpi_lib_handle_t *out_handle);
void unimpi_loader_unload(unimpi_lib_handle_t handle);
unimpi_backend_type_t unimpi_loader_identify_backend(unimpi_lib_handle_t handle);

/* Platform support and ABI validation */
int unimpi_loader_check_platform_support(unimpi_backend_type_t backend, const char *path);

/* Diagnostics */
void unimpi_diagnose_backend(const char *lib_path);
int unimpi_print_backend_info(void);

/* Known backends */
#define UNIMPI_MAX_BACKENDS 4
extern const unimpi_backend_info_t unimpi_backends[UNIMPI_MAX_BACKENDS];

#endif /* UNIMPI_LOADER_H */
