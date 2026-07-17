/* tests/unit/test_datatype.c - Datatype tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"

void test_basic_types(void) {
    int rank;
    int buf_int = 42;
    double buf_double = 3.14159;
    char buf_char = 'X';

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Just verify the types can be used in communication */
    if (rank == 0) {
        MPI_Status status;
        /* Send to self to test basic types */
        unimpi.send(&buf_int, 1, MPI_INT, 0, 100, MPI_COMM_WORLD);
        unimpi.recv(&buf_int, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, &status);
        assert(buf_int == 42);

        unimpi.send(&buf_double, 1, MPI_DOUBLE, 0, 101, MPI_COMM_WORLD);
        unimpi.recv(&buf_double, 1, MPI_DOUBLE, 0, 101, MPI_COMM_WORLD, &status);
        assert(buf_double > 3.14 && buf_double < 3.15);

        unimpi.send(&buf_char, 1, MPI_CHAR, 0, 102, MPI_COMM_WORLD);
        unimpi.recv(&buf_char, 1, MPI_CHAR, 0, 102, MPI_COMM_WORLD, &status);
        assert(buf_char == 'X');
    }

    unimpi.barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("  Basic types test passed\n");
    }
}

void test_type_size(void) {
    int rank;
    int size_int, size_double;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Get type sizes */
    unimpi.type_size(MPI_INT, &size_int);
    unimpi.type_size(MPI_DOUBLE, &size_double);

    if (rank == 0) {
        assert(size_int == sizeof(int));
        assert(size_double == sizeof(double));
        printf("  Type size test passed (int=%d, double=%d)\n", size_int, size_double);
    }
}

void test_contiguous_type(void) {
    int rank;
    MPI_Datatype newtype;
    int data[4] = {1, 2, 3, 4};
    int recv[4] = {0, 0, 0, 0};

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Create contiguous type of 4 ints */
    unimpi.type_contiguous(4, MPI_INT, &newtype);
    unimpi.type_commit(&newtype);

    if (rank == 0) {
        MPI_Status status;
        /* Send to self using new type */
        unimpi.send(data, 1, newtype, 0, 103, MPI_COMM_WORLD);
        unimpi.recv(recv, 1, newtype, 0, 103, MPI_COMM_WORLD, &status);

        assert(recv[0] == 1);
        assert(recv[1] == 2);
        assert(recv[2] == 3);
        assert(recv[3] == 4);
    }

    unimpi.type_free(&newtype);
    unimpi.barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("  Contiguous type test passed\n");
    }
}

void test_vector_type(void) {
    int rank;
    MPI_Datatype newtype;
    /* Matrix: 4x4, send every other row (stride = 8 elements, 2 per row) */
    double data[16];
    double recv[8] = {0};

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Initialize data */
    for (int i = 0; i < 16; i++) {
        data[i] = (double)i;
    }

    /* Create vector type: 2 blocks of 2 doubles, stride 4 */
    unimpi.type_vector(2, 2, 4, MPI_DOUBLE, &newtype);
    unimpi.type_commit(&newtype);

    if (rank == 0) {
        MPI_Status status;
        unimpi.send(data, 1, newtype, 0, 104, MPI_COMM_WORLD);
        unimpi.recv(recv, 4, MPI_DOUBLE, 0, 104, MPI_COMM_WORLD, &status);

        /* Should receive elements 0,1,4,5 */
        assert(recv[0] == 0.0);
        assert(recv[1] == 1.0);
        assert(recv[2] == 4.0);
        assert(recv[3] == 5.0);
    }

    unimpi.type_free(&newtype);
    unimpi.barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("  Vector type test passed\n");
    }
}

void test_dup_type(void) {
    int rank;
    MPI_Datatype dup_type;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Duplicate MPI_INT */
    unimpi.type_dup(MPI_INT, &dup_type);

    int send = 123, recv = 0;
    if (rank == 0) {
        MPI_Status status;
        unimpi.send(&send, 1, dup_type, 0, 105, MPI_COMM_WORLD);
        unimpi.recv(&recv, 1, dup_type, 0, 105, MPI_COMM_WORLD, &status);
        assert(recv == 123);
    }

    unimpi.type_free(&dup_type);
    unimpi.barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("  Dup type test passed\n");
    }
}

int main(int argc, char **argv) {
    int ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to initialize TFTK-MPI: %s\n", unimpi_error_string(ret));
        return 1;
    }

    printf("Running datatype tests...\n");
    printf("Using backend: %s\n", unimpi_get_backend_name());

    test_basic_types();
    test_type_size();
    test_contiguous_type();
    test_vector_type();
    test_dup_type();

    printf("All datatype tests passed!\n");

    unimpi_finalize();
    return 0;
}
