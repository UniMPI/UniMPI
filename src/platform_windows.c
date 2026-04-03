#include "tftk_mpi_platform.h"
#include <stdlib.h>

static char error_buffer[256];

tftk_mpi_lib_handle_t tftk_mpi_platform_dlopen(const char *path) {
    return LoadLibraryA(path);
}

void tftk_mpi_platform_dlclose(tftk_mpi_lib_handle_t handle) {
    if (handle) {
        FreeLibrary(handle);
    }
}

void* tftk_mpi_platform_dlsym(tftk_mpi_lib_handle_t handle, const char *symbol) {
    return (void*)GetProcAddress(handle, symbol);
}

const char* tftk_mpi_platform_dlerror(void) {
    DWORD error = GetLastError();
    if (error == 0) {
        return NULL;
    }
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   error_buffer, sizeof(error_buffer), NULL);
    return error_buffer;
}
