/* tests/unit/test_lifecycle_macros.c - Lifecycle macro tests */
#define UNIMPI_USE_STD_NAMES
#include <assert.h>
#include <string.h>
#include "unimpi.h"

#ifdef _WIN32
#define EXPECTED_FAKE_LIBRARY "Microsoft MPI fake backend"
#else
#define EXPECTED_FAKE_LIBRARY "Open MPI fake backend"
#endif

static void assert_before_init(void) {
    int flag = -1;

    assert(MPI_Initialized(&flag) == MPI_SUCCESS);
    assert(flag == 0);
    assert(MPI_Finalized(&flag) == MPI_SUCCESS);
    assert(flag == 0);
}

static void assert_after_init(void) {
    int flag = -1;

    assert(MPI_Initialized(&flag) == MPI_SUCCESS);
    assert(flag == 1);
    assert(MPI_Finalized(&flag) == MPI_SUCCESS);
    assert(flag == 0);
}

static void assert_after_finalize(void) {
    int flag = -1;

    assert(MPI_Initialized(&flag) == MPI_SUCCESS);
    assert(flag == 1);
    assert(MPI_Finalized(&flag) == MPI_SUCCESS);
    assert(flag == 1);
}

int main(int argc, char **argv) {
    int provided = -1;

    assert(argc == 2);
    assert_before_init();

    if (strcmp(argv[1], "init") == 0) {
        assert(MPI_Init(NULL, NULL) == MPI_SUCCESS);
    } else {
        assert(strcmp(argv[1], "thread") == 0);
        assert(MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided) ==
               MPI_SUCCESS);
        assert(provided == MPI_THREAD_MULTIPLE);
    }

    assert_after_init();
    assert(MPI_Finalize() == MPI_SUCCESS);
    assert_after_finalize();
    return 0;
}
