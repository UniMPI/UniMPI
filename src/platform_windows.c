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

const char* unimpi_platform_load_advice(void) {
    return "Troubleshooting:\n"
           "1. Check if MS-MPI runtime is installed: C:\\Program Files\\Microsoft MPI\\Bin\n"
           "2. Verify msmpi.dll is in C:\\Windows\\System32\\\n"
           "3. Ensure the executable architecture matches the MPI library\n"
           "4. Reinstall Microsoft MPI Runtime if needed\n"
           "5. Use PowerShell to check: dumpbin /DEPENDENTS msmpi.dll\n";
}
