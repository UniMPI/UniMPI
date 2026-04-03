#ifndef TFTK_MPI_PLATFORM_H
#define TFTK_MPI_PLATFORM_H

/* Platform detection */
#ifdef _WIN32
    #define TFTK_MPI_WINDOWS
#else
    #define TFTK_MPI_POSIX
#endif

/* Dynamic library handle */
#ifdef TFTK_MPI_WINDOWS
    #include <windows.h>
    typedef HMODULE tftk_mpi_lib_handle_t;
#else
    typedef void* tftk_mpi_lib_handle_t;
#endif

/* Function prototypes */
tftk_mpi_lib_handle_t tftk_mpi_platform_dlopen(const char *path);
void tftk_mpi_platform_dlclose(tftk_mpi_lib_handle_t handle);
void* tftk_mpi_platform_dlsym(tftk_mpi_lib_handle_t handle, const char *symbol);
const char* tftk_mpi_platform_dlerror(void);

#endif /* TFTK_MPI_PLATFORM_H */
