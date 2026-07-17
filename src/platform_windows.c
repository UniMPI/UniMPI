#include "unimpi_platform.h"
#include <stdlib.h>

static char error_buffer[256];

unimpi_lib_handle_t unimpi_platform_dlopen(const char *path) {
    return LoadLibraryA(path);
}

void unimpi_platform_dlclose(unimpi_lib_handle_t handle) {
    if (handle) {
        FreeLibrary(handle);
    }
}

void* unimpi_platform_dlsym(unimpi_lib_handle_t handle, const char *symbol) {
    return (void*)GetProcAddress(handle, symbol);
}

const char* unimpi_platform_dlerror(void) {
    DWORD error = GetLastError();
    if (error == 0) {
        return NULL;
    }
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   error_buffer, sizeof(error_buffer), NULL);
    return error_buffer;
}
