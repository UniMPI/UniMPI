/* Lifecycle state-machine tests.
 *
 * Every scenario is registered as a separate CTest process so that the
 * process-global MPI lifecycle starts in UNIMPI_STATE_NEVER each time.  The
 * tests always select a fake MPI DSO explicitly and never depend on a system
 * MPI installation.
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"

#ifdef _WIN32
#define EXPECTED_FAKE_BACKEND "msmpi"
#else
#define EXPECTED_FAKE_BACKEND "openmpi"
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

static void configure_fake_backend(const char *library_path) {
    unset_env_value("UNIMPI_BACKEND");
    unset_env_value("UNIMPI_FAKE_INIT_RESULT");
    unset_env_value("UNIMPI_FAKE_INIT_THREAD_RESULT");
    unset_env_value("UNIMPI_FAKE_PROVIDED_LEVEL");
    unset_env_value("UNIMPI_FAKE_FINALIZE_RESULT");
    set_env_value("UNIMPI_LIBRARY", library_path);
}

static void assert_identity_cleared(void) {
    assert(unimpi_is_initialized() == 0);
    assert(unimpi_get_backend_name() == NULL);
    assert(unimpi_get_library_path() == NULL);
}

static void assert_vtable_cleared(void) {
    assert(unimpi.init == NULL);
    assert(unimpi.finalize == NULL);
    assert(unimpi.comm_size == NULL);
    assert(unimpi.comm_rank == NULL);
    assert(unimpi.get_library_version == NULL);
    assert(UNIMPI_COMM_WORLD == 0);
    assert(UNIMPI_COMM_SELF == 0);
}

static void assert_active_identity(const char *library_path) {
    char library_version[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    int result_length = -1;

    assert(unimpi_is_initialized() == 1);
    assert(unimpi_get_backend_name() != NULL);
    assert(strcmp(unimpi_get_backend_name(), EXPECTED_FAKE_BACKEND) == 0);
    assert(unimpi_get_library_path() != NULL);
    assert(strcmp(unimpi_get_library_path(), library_path) == 0);
    assert(unimpi.get_library_version != NULL);
    assert(unimpi.get_library_version(library_version, &result_length) == 0);
    assert(result_length > 0);
    assert(strstr(library_version, "fake backend") != NULL);
}

static void scenario_preinit(void) {
    char version[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    int flag = -1;
    int result_length = -1;
    int version_major = -1;
    int version_minor = -1;

    assert_identity_cleared();
    assert_vtable_cleared();
    assert(unimpi_finalize() == UNIMPI_ERR_NOT_INITIALIZED);
    assert(unimpi_mpi_initialized(&flag) == UNIMPI_OK);
    assert(flag == 0);
    assert(unimpi_mpi_finalized(&flag) == UNIMPI_OK);
    assert(flag == 0);
    assert(unimpi_mpi_get_version(&version_major, &version_minor) == UNIMPI_OK);
    assert(version_major == UNIMPI_MPI_VERSION);
    assert(version_minor == UNIMPI_MPI_SUBVERSION);
    assert(unimpi_mpi_get_library_version(version, &result_length) == UNIMPI_OK);
    assert(result_length > 0);
    assert(strstr(version, "no active MPI backend") != NULL);
}

static void scenario_success(const char *library_path) {
    int argc_value = 1;
    char program_name[] = "test_lifecycle";
    char *argv_values[] = {program_name, NULL};
    char **argv_pointer = argv_values;
    int flag = -1;

    assert(unimpi_init(&argc_value, &argv_pointer) == UNIMPI_OK);
    assert_active_identity(library_path);
    assert(unimpi_mpi_initialized(&flag) == UNIMPI_OK);
    assert(flag == 1);
    assert(unimpi_mpi_finalized(&flag) == UNIMPI_OK);
    assert(flag == 0);
    assert(unimpi_finalize() == UNIMPI_OK);
    assert_identity_cleared();
    assert_vtable_cleared();
}

static void scenario_double_init(const char *library_path) {
    int provided = -1;

    assert(unimpi_init(NULL, NULL) == UNIMPI_OK);
    assert(unimpi_init(NULL, NULL) == UNIMPI_ERR_ALREADY_INITIALIZED);
    assert(unimpi_init_thread(NULL, NULL, UNIMPI_THREAD_SINGLE, &provided) ==
           UNIMPI_ERR_ALREADY_INITIALIZED);
    assert(unimpi_init_with(library_path) == UNIMPI_ERR_ALREADY_INITIALIZED);
    assert_active_identity(library_path);
    assert(unimpi_finalize() == UNIMPI_OK);
}

static void scenario_init_failure_retry(const char *library_path) {
    set_env_value("UNIMPI_FAKE_INIT_RESULT", "17");
    assert(unimpi_init(NULL, NULL) == UNIMPI_ERR_BACKEND_LOAD);
    assert_identity_cleared();
    assert_vtable_cleared();

    unset_env_value("UNIMPI_FAKE_INIT_RESULT");
    assert(unimpi_init(NULL, NULL) == UNIMPI_OK);
    assert_active_identity(library_path);
    assert(unimpi_finalize() == UNIMPI_OK);
    assert_identity_cleared();
    assert_vtable_cleared();
}

static void scenario_thread_level(const char *library_path, int required) {
    int provided = -1;

    assert(unimpi_init_thread(NULL, NULL, required, &provided) == UNIMPI_OK);
    assert(provided == required);
    assert_active_identity(library_path);
    assert(unimpi_finalize() == UNIMPI_OK);
}

static void scenario_thread_downgrade(const char *library_path) {
    int provided = -1;

    set_env_value("UNIMPI_FAKE_PROVIDED_LEVEL", "1");
    assert(unimpi_init_thread(NULL, NULL, UNIMPI_THREAD_MULTIPLE, &provided) ==
           UNIMPI_OK);
    assert(provided == UNIMPI_THREAD_FUNNELED);
    assert_active_identity(library_path);
    assert(unimpi_finalize() == UNIMPI_OK);
}

static void scenario_thread_fallback(const char *library_path) {
    int provided = -1;

    assert(unimpi_init_thread(NULL, NULL, UNIMPI_THREAD_MULTIPLE, &provided) ==
           UNIMPI_OK);
    assert(provided == UNIMPI_THREAD_SINGLE);
    assert(unimpi.init_thread == NULL);
    assert_active_identity(library_path);
    assert(unimpi_finalize() == UNIMPI_OK);
}

static void scenario_finalize_failure(const char *library_path) {
    int flag = -1;

    assert(unimpi_init(NULL, NULL) == UNIMPI_OK);
    assert_active_identity(library_path);
    set_env_value("UNIMPI_FAKE_FINALIZE_RESULT", "73");
    assert(unimpi_finalize() == 73);

    assert_identity_cleared();
    assert_vtable_cleared();
    assert(unimpi_mpi_initialized(&flag) == UNIMPI_OK);
    assert(flag == 1);
    assert(unimpi_mpi_finalized(&flag) == UNIMPI_OK);
    assert(flag == 0);
    assert(unimpi_finalize() == UNIMPI_ERR_INVALID_STATE);
    assert(unimpi_init(NULL, NULL) == UNIMPI_ERR_INVALID_STATE);
}

static void scenario_postfinal(const char *library_path) {
    int flag = -1;
    int provided = -1;

    assert(unimpi_init(NULL, NULL) == UNIMPI_OK);
    assert(unimpi_finalize() == UNIMPI_OK);
    assert_identity_cleared();
    assert_vtable_cleared();
    assert(unimpi_mpi_initialized(&flag) == UNIMPI_OK);
    assert(flag == 1);
    assert(unimpi_mpi_finalized(&flag) == UNIMPI_OK);
    assert(flag == 1);
    assert(unimpi_finalize() == UNIMPI_ERR_FINALIZED);
    assert(unimpi_init(NULL, NULL) == UNIMPI_ERR_FINALIZED);
    assert(unimpi_init_thread(NULL, NULL, UNIMPI_THREAD_SINGLE, &provided) ==
           UNIMPI_ERR_FINALIZED);
    assert(unimpi_init_with(library_path) == UNIMPI_ERR_FINALIZED);
}

static void scenario_identity_cleanup(const char *library_path) {
    char backend_copy[32];
    char path_copy[1024];

    assert(unimpi_init_with(library_path) == UNIMPI_OK);
    assert_active_identity(library_path);
    assert(strlen(unimpi_get_backend_name()) < sizeof(backend_copy));
    assert(strlen(unimpi_get_library_path()) < sizeof(path_copy));
    strcpy(backend_copy, unimpi_get_backend_name());
    strcpy(path_copy, unimpi_get_library_path());
    assert(unimpi_finalize() == UNIMPI_OK);
    assert(strcmp(backend_copy, EXPECTED_FAKE_BACKEND) == 0);
    assert(strcmp(path_copy, library_path) == 0);
    assert_identity_cleared();
    assert_vtable_cleared();
}

static int parse_thread_level(const char *value) {
    char *end = NULL;
    long parsed = strtol(value, &end, 10);

    assert(end != value);
    assert(*end == '\0');
    assert(parsed >= UNIMPI_THREAD_SINGLE);
    assert(parsed <= UNIMPI_THREAD_MULTIPLE);
    return (int)parsed;
}

int main(int argc, char **argv) {
    const char *scenario;
    const char *library_path;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <scenario> <fake-mpi-library> [thread-level]\n",
                argv[0]);
        return 2;
    }

    scenario = argv[1];
    library_path = argv[2];
    configure_fake_backend(library_path);

    if (strcmp(scenario, "preinit") == 0) {
        scenario_preinit();
    } else if (strcmp(scenario, "success") == 0) {
        scenario_success(library_path);
    } else if (strcmp(scenario, "double-init") == 0) {
        scenario_double_init(library_path);
    } else if (strcmp(scenario, "init-failure-retry") == 0) {
        scenario_init_failure_retry(library_path);
    } else if (strcmp(scenario, "thread-level") == 0) {
        assert(argc == 4);
        scenario_thread_level(library_path, parse_thread_level(argv[3]));
    } else if (strcmp(scenario, "thread-downgrade") == 0) {
        scenario_thread_downgrade(library_path);
    } else if (strcmp(scenario, "thread-fallback") == 0) {
        scenario_thread_fallback(library_path);
    } else if (strcmp(scenario, "finalize-failure") == 0) {
        scenario_finalize_failure(library_path);
    } else if (strcmp(scenario, "postfinal") == 0) {
        scenario_postfinal(library_path);
    } else if (strcmp(scenario, "identity-cleanup") == 0) {
        scenario_identity_cleanup(library_path);
    } else {
        fprintf(stderr, "Unknown lifecycle scenario: %s\n", scenario);
        return 2;
    }

    printf("Lifecycle scenario '%s' passed\n", scenario);
    return 0;
}
