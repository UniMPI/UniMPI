/* tests/internal/test_opaque_handle_width.c
 * Verify opaque handle width conversion between facade (intptr_t)
 * and native (int) representations.
 *
 * Critical for LLP64 (Windows x64) compatibility where intptr_t is 8 bytes
 * but native MPI handles are 4 bytes.
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "unimpi_vtable.h"
#include "unimpi.h"

/* Test configuration: verify handle conversion invariants */
#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s at line %d\n", #cond, __LINE__); \
        return 1; \
    } \
} while(0)

/* ==================== Handle Width Tests ==================== */

/* Test 1: Basic handle value preservation through narrowing/widening */
static int test_basic_handle_conversion(void) {
    /* Common MPI handle values */
    intptr_t test_values[] = {
        0x44000000,  /* MPI_COMM_WORLD (MPICH/Intel) */
        0x44000001,  /* MPI_COMM_SELF */
        0x04000000,  /* MPI_COMM_NULL */
        0x0c000000,  /* MPI_DATATYPE_NULL */
        0x0c000001,  /* MPI_INT */
        0x08000000,  /* MPI_GROUP_NULL */
        0x20000000,  /* MPI_WIN_NULL */
        0x18000000,  /* MPI_OP_NULL */
        0x1c000000,  /* MPI_INFO_NULL */
        0,           /* NULL handle */
    };
    int i;

    printf("Testing basic handle conversion...\n");

    for (i = 0; i < (int)(sizeof(test_values)/sizeof(test_values[0])); i++) {
        intptr_t facade = test_values[i];

        /* Narrow to native int */
        int native = (int)(intptr_t)facade;

        /* Widen back to intptr_t */
        intptr_t restored = (intptr_t)native;

        /* Verify preservation (lower 32 bits must match) */
        TEST_ASSERT((restored & 0xFFFFFFFFLL) == (facade & 0xFFFFFFFFLL));

        /* Sign extension check: on LP64, negative values should extend */
        /* On LLP64, behavior may differ - this is the key test */
    }

    printf("  PASS: Basic handle conversion\n");
    return 0;
}

/* Test 2: Comm handle specific tests */
static int test_comm_handle(void) {
    MPI_Comm comm;
    int native;
    MPI_Comm restored;

    printf("Testing Comm handle...\n");

    /* Test predefined values */
    comm = 0x44000000;
    native = (int)(intptr_t)comm;
    restored = (MPI_Comm)(intptr_t)native;
    TEST_ASSERT(restored == comm);

    /* Test negative values (sign extension behavior) */
    comm = (MPI_Comm)(intptr_t)-1;  /* All bits set */
    native = (int)(intptr_t)comm;
    restored = (MPI_Comm)(intptr_t)native;

    /* Sign extension should preserve -1 */
    TEST_ASSERT(native == -1);
    TEST_ASSERT(restored == (MPI_Comm)(intptr_t)-1);

    printf("  PASS: Comm handle\n");
    return 0;
}

/* Test 3: Datatype handle tests */
static int test_datatype_handle(void) {
    MPI_Datatype dtype;
    int native;
    MPI_Datatype restored;

    printf("Testing Datatype handle...\n");

    /* Standard datatypes */
    dtype = 0x0c000001;  /* MPI_INT */
    native = (int)(intptr_t)dtype;
    restored = (MPI_Datatype)(intptr_t)native;
    TEST_ASSERT(restored == dtype);

    /* Large handle values */
    dtype = 0x7FFFFFFF;  /* Maximum positive int */
    native = (int)(intptr_t)dtype;
    restored = (MPI_Datatype)(intptr_t)native;
    TEST_ASSERT(restored == dtype);

    printf("  PASS: Datatype handle\n");
    return 0;
}

/* Test 4: Group handle tests */
static int test_group_handle(void) {
    MPI_Group group;
    int native;
    MPI_Group restored;

    printf("Testing Group handle...\n");

    group = 0x08000001;  /* Group value */
    native = (int)(intptr_t)group;
    restored = (MPI_Group)(intptr_t)native;
    TEST_ASSERT(restored == group);

    printf("  PASS: Group handle\n");
    return 0;
}

/* Test 5: Window handle tests */
static int test_win_handle(void) {
    MPI_Win win;
    int native;
    MPI_Win restored;

    printf("Testing Window handle...\n");

    win = 0x20000001;  /* Window handle */
    native = (int)(intptr_t)win;
    restored = (MPI_Win)(intptr_t)native;
    TEST_ASSERT(restored == win);

    printf("  PASS: Window handle\n");
    return 0;
}

/* Test 6: Operation handle tests */
static int test_op_handle(void) {
    MPI_Op op;
    int native;
    MPI_Op restored;

    printf("Testing Op handle...\n");

    op = 0x18000001;  /* Operation handle */
    native = (int)(intptr_t)op;
    restored = (MPI_Op)(intptr_t)native;
    TEST_ASSERT(restored == op);

    printf("  PASS: Op handle\n");
    return 0;
}

/* Test 7: Info handle tests */
static int test_info_handle(void) {
    MPI_Info info;
    int native;
    MPI_Info restored;

    printf("Testing Info handle...\n");

    info = 0x1c000001;  /* Info handle */
    native = (int)(intptr_t)info;
    restored = (MPI_Info)(intptr_t)native;
    TEST_ASSERT(restored == info);

    printf("  PASS: Info handle\n");
    return 0;
}

/* Test 8: Sign extension edge cases (critical for LLP64) */
static int test_sign_extension(void) {
    /* Test values where sign extension matters */
    struct {
        intptr_t input;
        int expected_native;
        intptr_t expected_restored_lp64;  /* Expected on LP64 */
    } test_cases[] = {
        /* Positive values - should be same on both */
        {0x00000000, 0, 0},
        {0x00000001, 1, 1},
        {0x7FFFFFFF, INT_MAX, INT_MAX},
        {0x44000000, 0x44000000, 0x44000000},

        /* Negative values - sign extension behavior critical */
        {0xFFFFFFFF, -1, -1},            /* All 1s */
        {0x80000000, INT_MIN, INT_MIN},    /* Sign bit set */
        {0xAAAAAAAA, -1431655766, -1431655766}, /* Pattern */
    };
    int i;

    printf("Testing sign extension (LP64 platform)...\n");

    for (i = 0; i < (int)(sizeof(test_cases)/sizeof(test_cases[0])); i++) {
        intptr_t input = test_cases[i].input;
        int native = (int)(intptr_t)input;
        intptr_t restored = (intptr_t)native;

        TEST_ASSERT(native == test_cases[i].expected_native);

        /* On LP64, restored should match expected */
        /* On LLP64, this may differ - that's what we're testing */
        if (sizeof(void*) == 8) {
            /* LP64 assumption: long is 8 bytes, sign extension works */
            TEST_ASSERT(restored == test_cases[i].expected_restored_lp64);
        }
    }

    printf("  PASS: Sign extension\n");
    return 0;
}

/* Test 9: NULL handle special cases */
static int test_null_handles(void) {
    printf("Testing NULL handle values...\n");

    /* All predefined NULL values should convert correctly */
    TEST_ASSERT((int)(intptr_t)(MPI_Comm)0x04000000 == 0x04000000);
    TEST_ASSERT((int)(intptr_t)(MPI_Datatype)0x0c000000 == 0x0c000000);
    TEST_ASSERT((int)(intptr_t)(MPI_Group)0x08000000 == 0x08000000);
    TEST_ASSERT((int)(intptr_t)(MPI_Win)0x20000000 == 0x20000000);
    TEST_ASSERT((int)(intptr_t)(MPI_Op)0x18000000 == 0x18000000);
    TEST_ASSERT((int)(intptr_t)(MPI_Info)0x1c000000 == 0x1c000000);

    printf("  PASS: NULL handles\n");
    return 0;
}

/* Test 10: Platform detection and assumptions */
static int test_platform_assumptions(void) {
    printf("Testing platform assumptions...\n");

    /* Verify critical size assumptions */
    TEST_ASSERT(sizeof(intptr_t) == sizeof(void*));
    TEST_ASSERT(sizeof(MPI_Comm) == sizeof(intptr_t));
    TEST_ASSERT(sizeof(MPI_Datatype) == sizeof(intptr_t));
    TEST_ASSERT(sizeof(MPI_Request) == sizeof(intptr_t));

    /* Log platform info */
    printf("  Platform: ");
    if (sizeof(void*) == 8) {
        if ((long)-1 == 0xFFFFFFFFFFFFFFFFL) {
            printf("LP64 (long=8 bytes)\n");
        } else {
            printf("LLP64 (long=4 bytes, intptr_t=8 bytes)\n");
        }
    } else {
        printf("ILP32 (32-bit)\n");
    }

    printf("  sizeof(int) = %zu\n", sizeof(int));
    printf("  sizeof(long) = %zu\n", sizeof(long));
    printf("  sizeof(intptr_t) = %zu\n", sizeof(intptr_t));
    printf("  sizeof(void*) = %zu\n", sizeof(void*));
    printf("  sizeof(MPI_Comm) = %zu\n", sizeof(MPI_Comm));

    printf("  PASS: Platform assumptions\n");
    return 0;
}

/* ==================== Main ==================== */

int main(void) {
    int failures = 0;

    printf("=== UniMPI Opaque Handle Width Tests ===\n\n");

    failures += test_platform_assumptions();
    failures += test_basic_handle_conversion();
    failures += test_comm_handle();
    failures += test_datatype_handle();
    failures += test_group_handle();
    failures += test_win_handle();
    failures += test_op_handle();
    failures += test_info_handle();
    failures += test_sign_extension();
    failures += test_null_handles();

    printf("\n");
    if (failures == 0) {
        printf("All opaque handle width tests PASSED\n");
        return 0;
    } else {
        printf("%d opaque handle width test(s) FAILED\n", failures);
        return 1;
    }
}

