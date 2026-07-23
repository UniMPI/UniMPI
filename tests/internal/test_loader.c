/* Loader decision-table and failure-path tests. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_loader.h"

#ifdef _WIN32
#include <io.h>
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

static void set_env_value(const char *name, const char *value) {
#ifdef _WIN32
    assert(_putenv_s(name, value) == 0);
#else
    assert(setenv(name, value, 1) == 0);
#endif
}

static void unset_env_value(const char *name) {
#ifdef _WIN32
    assert(_putenv_s(name, "") == 0);
#else
    assert(unsetenv(name) == 0);
#endif
}

static void clear_loader_environment(void) {
    unset_env_value("UNIMPI_BACKEND");
    unset_env_value("UNIMPI_LIBRARY");
}

static const char* expected_auto_library(void) {
#ifdef _WIN32
    return "msmpi.dll";
#else
    return "libmpi.so.40";
#endif
}

static int expected_platform_support(unimpi_backend_type_t backend) {
#ifdef _WIN32
    return backend == UNIMPI_BACKEND_MSMPI;
#elif defined(__APPLE__)
    return backend == UNIMPI_BACKEND_OPENMPI ||
           backend == UNIMPI_BACKEND_MPICH;
#elif defined(__linux__)
    return backend == UNIMPI_BACKEND_OPENMPI ||
           backend == UNIMPI_BACKEND_MPICH ||
           backend == UNIMPI_BACKEND_INTELMPI;
#else
    (void)backend;
    return 0;
#endif
}

static void test_backend_detection_decision_table(void) {
    const char *detected_path = NULL;

    clear_loader_environment();
    assert(unimpi_loader_get_env_backend() == NULL);
    assert(unimpi_loader_detect_backend(&detected_path) == UNIMPI_OK);
    assert(strcmp(detected_path, expected_auto_library()) == 0);

    set_env_value("UNIMPI_BACKEND", "openmpi");
    assert(strcmp(unimpi_loader_get_env_backend(), "openmpi") == 0);
    assert(unimpi_loader_detect_backend(&detected_path) == UNIMPI_OK);
    assert(strcmp(detected_path, unimpi_backends[0].lib_name) == 0);

    set_env_value("UNIMPI_LIBRARY", "/explicit/fake/libmpi-for-test");
    assert(unimpi_loader_detect_backend(&detected_path) == UNIMPI_OK);
    assert(strcmp(detected_path, "/explicit/fake/libmpi-for-test") == 0);

    unset_env_value("UNIMPI_LIBRARY");
    set_env_value("UNIMPI_BACKEND", "custom-mpi-runtime");
    assert(unimpi_loader_detect_backend(&detected_path) == UNIMPI_OK);
    assert(strcmp(detected_path, "custom-mpi-runtime") == 0);

    set_env_value("UNIMPI_BACKEND", "");
    set_env_value("UNIMPI_LIBRARY", "");
    assert(unimpi_loader_get_env_backend() == NULL);
    assert(unimpi_loader_detect_backend(&detected_path) == UNIMPI_OK);
    assert(strcmp(detected_path, expected_auto_library()) == 0);

    clear_loader_environment();
}

static void test_backend_info(void) {
    static const char *const expected_names[] = {
        "openmpi", "mpich", "intelmpi", "msmpi"
    };
    static const unimpi_backend_type_t expected_types[] = {
        UNIMPI_BACKEND_OPENMPI,
        UNIMPI_BACKEND_MPICH,
        UNIMPI_BACKEND_INTELMPI,
        UNIMPI_BACKEND_MSMPI
    };

    assert(UNIMPI_MAX_BACKENDS == 4);
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        assert(unimpi_backends[i].type == expected_types[i]);
        assert(strcmp(unimpi_backends[i].name, expected_names[i]) == 0);
        assert(unimpi_backends[i].lib_name != NULL);
        assert(unimpi_backends[i].lib_name[0] != '\0');
    }
}

static void test_output_and_abi_validation(void) {
    int sentinel;
    unimpi_lib_handle_t handle = (unimpi_lib_handle_t)&sentinel;

    assert(unimpi_loader_detect_backend(NULL) == UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_loader_load("unused-library-name", NULL) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_loader_load(NULL, &handle) == UNIMPI_ERR_NO_BACKEND);
    assert(handle == NULL);

    handle = (unimpi_lib_handle_t)&sentinel;
    assert(unimpi_loader_load("/tmp/libmpi_abi.so", &handle) ==
           UNIMPI_ERR_ABI_MISMATCH);
    assert(handle == NULL);
    handle = (unimpi_lib_handle_t)&sentinel;
    assert(unimpi_loader_load("/tmp/LIBMPI-ABI.DYLIB", &handle) ==
           UNIMPI_ERR_ABI_MISMATCH);
    assert(handle == NULL);
}

static void test_platform_matrix(void) {
    static const unimpi_backend_type_t known_backends[] = {
        UNIMPI_BACKEND_OPENMPI,
        UNIMPI_BACKEND_MPICH,
        UNIMPI_BACKEND_INTELMPI,
        UNIMPI_BACKEND_MSMPI
    };

    assert(unimpi_loader_check_platform_support(UNIMPI_BACKEND_UNKNOWN,
                                                "native-mpi") ==
           UNIMPI_ERR_BACKEND_NOT_SUPPORTED);
    assert(unimpi_loader_check_platform_support((unimpi_backend_type_t)-1,
                                                "native-mpi") ==
           UNIMPI_ERR_BACKEND_NOT_SUPPORTED);
    assert(unimpi_loader_check_platform_support((unimpi_backend_type_t)99,
                                                "native-mpi") ==
           UNIMPI_ERR_BACKEND_NOT_SUPPORTED);

    for (size_t i = 0; i < sizeof(known_backends) / sizeof(known_backends[0]);
         i++) {
        unimpi_backend_type_t backend = known_backends[i];
        int result = unimpi_loader_check_platform_support(backend, "native-mpi");

        if (expected_platform_support(backend)) {
            assert(result == UNIMPI_OK);
            if (backend == UNIMPI_BACKEND_MPICH ||
                backend == UNIMPI_BACKEND_INTELMPI) {
                assert(unimpi_loader_check_platform_support(
                           backend, "/opt/libmpi_abi.so") ==
                       UNIMPI_ERR_ABI_MISMATCH);
            }
        } else {
            assert(result == UNIMPI_ERR_BACKEND_NOT_SUPPORTED);
        }
    }
}

static void test_load_failure_diagnostics(void) {
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
}

int main(void) {
    test_backend_detection_decision_table();
    test_backend_info();
    test_output_and_abi_validation();
    test_platform_matrix();
    test_load_failure_diagnostics();

    printf("Loader decision-table tests passed\n");
    return 0;
}
