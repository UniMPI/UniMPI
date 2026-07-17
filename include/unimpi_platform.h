#ifndef UNIMPI_PLATFORM_H
#define UNIMPI_PLATFORM_H

/* Platform detection */
#ifdef _WIN32
    #define UNIMPI_WINDOWS
#else
    #define UNIMPI_POSIX
#endif

/* Dynamic library handle */
#ifdef UNIMPI_WINDOWS
    #include <windows.h>
    typedef HMODULE unimpi_lib_handle_t;
#else
    typedef void* unimpi_lib_handle_t;
#endif

/* Function prototypes */
unimpi_lib_handle_t unimpi_platform_dlopen(const char *path);
void unimpi_platform_dlclose(unimpi_lib_handle_t handle);
void* unimpi_platform_dlsym(unimpi_lib_handle_t handle, const char *symbol);
const char* unimpi_platform_dlerror(void);

#endif /* UNIMPI_PLATFORM_H */
