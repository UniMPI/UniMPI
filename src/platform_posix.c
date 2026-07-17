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
