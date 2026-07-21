#include "unimpi_platform.h"
#include <dlfcn.h>

unimpi_lib_handle_t unimpi_platform_dlopen(const char *path) {
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}

void unimpi_platform_dlclose(unimpi_lib_handle_t handle) {
    if (handle) {
        dlclose(handle);
    }
}

void* unimpi_platform_dlsym(unimpi_lib_handle_t handle, const char *symbol) {
    return dlsym(handle, symbol);
}

const char* unimpi_platform_dlerror(void) {
    return dlerror();
}

const char* unimpi_platform_load_advice(void) {
#ifdef __APPLE__
    return "Troubleshooting:\n"
           "1. Check if the library exists: ls -la <library_path>\n"
           "2. Check library dependencies: otool -L <library_path>\n"
           "3. Ensure DYLD_LIBRARY_PATH includes the MPI library directory\n"
           "4. Ensure DYLD_FALLBACK_LIBRARY_PATH includes the MPI library directory\n"
           "5. Verify MPI is installed: mpirun --version\n";
#else
    return "Troubleshooting:\n"
           "1. Check if the library exists: ls -la <library_path>\n"
           "2. Check library dependencies: ldd <library_path>\n"
           "3. List all cached libraries: ldconfig -p | grep <library_name>\n"
           "4. Ensure LD_LIBRARY_PATH includes the MPI library directory\n"
           "5. Verify MPI is installed: mpirun --version\n";
#endif
}
