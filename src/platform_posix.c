#include "tftk_mpi_platform.h"
#include <dlfcn.h>

tftk_mpi_lib_handle_t tftk_mpi_platform_dlopen(const char *path) {
    return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}

void tftk_mpi_platform_dlclose(tftk_mpi_lib_handle_t handle) {
    if (handle) {
        dlclose(handle);
    }
}

void* tftk_mpi_platform_dlsym(tftk_mpi_lib_handle_t handle, const char *symbol) {
    return dlsym(handle, symbol);
}

const char* tftk_mpi_platform_dlerror(void) {
    return dlerror();
}
