/* Test MS-MPI Comm_spawn with valid executable */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--child") == 0) {
        /* Child process - run with fewer processes */
        int val = 0;
        MPI_Status status;
        MPI_Init(&argc, &argv);
        MPI_Comm parent;
        int ret = MPI_Comm_get_parent(&parent);
        if (ret == 0 && parent != (MPI_Comm)0) {
            MPI_Recv(&val, 1, MPI_INT, 0, 0, parent, &status);
            val += 1;
            MPI_Send(&val, 1, MPI_INT, 0, 0, parent);
            MPI_Comm_disconnect(&parent);
        }
        MPI_Finalize();
        return 0;
    }

    int ret, rank;
    MPI_Comm intercomm;
    MPI_Status status;
    int errcodes[1];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("Testing Comm_spawn with identity executable...\n");
        char *args[] = {(char*)"--child", NULL};
        ret = MPI_Comm_spawn("./build/tests/test_spawn_test.exe", args, 1,
                             MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes);
        printf("  MPI_Comm_spawn returned: %d\n", ret);
        if (ret == MPI_SUCCESS) {
            printf("  Sending data to child...\n");
            int val = 42;
            MPI_Send(&val, 1, MPI_INT, 0, 0, intercomm);
            int child_result;
            MPI_Recv(&child_result, 1, MPI_INT, 0, 0, intercomm, &status);
            printf("  Received from child: %d (expected 43)\n", child_result);
            MPI_Comm_disconnect(&intercomm);
        } else {
            printf("  MS-MPI: Comm_spawn not supported (expected)\n");
        }
    }

    MPI_Finalize();
    return 0;
}
