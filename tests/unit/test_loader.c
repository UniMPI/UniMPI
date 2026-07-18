/* tests/unit/test_loader.c - Loader tests */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"
#include "unimpi_loader.h"

#ifdef _WIN32
    #include <io.h>
    #define setenv(name, value, overwrite) _putenv(name "=" value)
    #define unsetenv(name) _putenv(name "=")
    #define MISSING_LIBRARY "Z:\\unimpi-missing\\backend-does-not-exist.dll"
    #define TEST_CLOSE _close
    #define TEST_DUP _dup
    #define TEST_DUP2 _dup2
    #define TEST_FILENO _fileno
#else
    #include <unistd.h>
    #define MISSING_LIBRARY "/unimpi-missing/backend-does-not-exist.so"
    #define TEST_CLOSE close
    #define TEST_DUP dup
    #define TEST_DUP2 dup2
    #define TEST_FILENO fileno
#endif

void test_backend_detection_env(void) {
    /* Test with no environment variable */
    unsetenv("UNIMPI_BACKEND");
    const char *backend = unimpi_loader_get_env_backend();
    assert(backend == NULL);

    /* Test with environment variable set */
    setenv("UNIMPI_BACKEND", "openmpi", 1);
    backend = unimpi_loader_get_env_backend();
    assert(backend != NULL);
    assert(strcmp(backend, "openmpi") == 0);

    /* Cleanup */
    unsetenv("UNIMPI_BACKEND");

    printf("  Backend detection tests passed\n");
}

void test_backend_info(void) {
    /* Test backend info array */
    assert(UNIMPI_MAX_BACKENDS == 4);

    /* OpenMPI should be first */
    assert(unimpi_backends[0].type == UNIMPI_BACKEND_OPENMPI);
    assert(strcmp(unimpi_backends[0].name, "openmpi") == 0);

    /* MPICH should be present */
    int found_mpich = 0;
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        if (unimpi_backends[i].type == UNIMPI_BACKEND_MPICH) {
            found_mpich = 1;
            break;
        }
    }
    assert(found_mpich);

    printf("  Backend info tests passed\n");
}

void test_output_validation(void) {
    int sentinel;
    unimpi_lib_handle_t handle = (unimpi_lib_handle_t)&sentinel;

    assert(unimpi_loader_detect_backend(NULL) == UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_loader_load("unused-library-name", NULL) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_loader_load(NULL, &handle) == UNIMPI_ERR_NO_BACKEND);
    assert(handle == NULL);

    printf("  Loader output validation passed\n");
}

void test_load_failure_diagnostics(void) {
    int sentinel;
    int load_result;
    int saved_stderr;
    unimpi_lib_handle_t handle;
    FILE *capture;
    size_t output_length;
    const char *load_error;
    const char *advice;
    char expected_error[1024];
    char output[8192];

    handle = unimpi_platform_dlopen(MISSING_LIBRARY);
    assert(handle == NULL);
    load_error = unimpi_platform_dlerror();
    assert(load_error != NULL);
    assert(load_error[0] != '\0');
    assert(strlen(load_error) < sizeof(expected_error));
    strcpy(expected_error, load_error);

    capture = tmpfile();
    assert(capture != NULL);
    fflush(stderr);
    saved_stderr = TEST_DUP(TEST_FILENO(stderr));
    assert(saved_stderr >= 0);
    assert(TEST_DUP2(TEST_FILENO(capture), TEST_FILENO(stderr)) >= 0);
    handle = (unimpi_lib_handle_t)&sentinel;
    load_result = unimpi_loader_load(MISSING_LIBRARY, &handle);
    fflush(stderr);
    assert(TEST_DUP2(saved_stderr, TEST_FILENO(stderr)) >= 0);
    assert(TEST_CLOSE(saved_stderr) == 0);

    rewind(capture);
    output_length = fread(output, 1, sizeof(output) - 1, capture);
    output[output_length] = '\0';
    assert(fclose(capture) == 0);

    assert(load_result == UNIMPI_ERR_BACKEND_LOAD);
    assert(handle == NULL);
    assert(strstr(output, MISSING_LIBRARY) != NULL);
    assert(strstr(output, expected_error) != NULL);

    advice = unimpi_platform_load_advice();
    assert(advice != NULL);
#ifdef _WIN32
    assert(strstr(advice, "PowerShell") != NULL);
    assert(strstr(advice, "MS-MPI runtime") != NULL);
    assert(strstr(advice, "dumpbin /DEPENDENTS") != NULL);
    assert(strstr(advice, "`ls -la`") == NULL);
    assert(strstr(advice, "`ldd`") == NULL);
#elif defined(__APPLE__)
    assert(strstr(advice, "otool -L") != NULL);
    assert(strstr(advice, "DYLD_LIBRARY_PATH") != NULL);
    assert(strstr(advice, "DYLD_FALLBACK_LIBRARY_PATH") != NULL);
    assert(strstr(advice, "`ldd`") == NULL);
    assert(strstr(advice, "Ensure LD_LIBRARY_PATH") == NULL);
#elif defined(__linux__)
    assert(strstr(advice, "ldd") != NULL);
    assert(strstr(advice, "ldconfig -p") != NULL);
    assert(strstr(advice, "LD_LIBRARY_PATH") != NULL);
    assert(strstr(advice, "otool -L") == NULL);
    assert(strstr(advice, "PowerShell") == NULL);
#endif

    printf("  Loader failure diagnostics passed\n");
}

int main(void) {
    printf("Running loader tests...\n");

    test_backend_detection_env();
    test_backend_info();
    test_output_validation();
    test_load_failure_diagnostics();

    printf("All loader tests passed!\n");
    return 0;
}
