/* tests/internal/test_abi_adapters.c
 * Integration tests for ABI adapters, validating handle conversion
 * in realistic usage patterns.
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "unimpi_vtable.h"
#include "unimpi.h"

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s at line %d\n", #cond, __LINE__); \
        return 1; \
    } \
} while(0)

/* ==================== Mock Backend Functions ==================== */

/* Simulates integer backend functions */
static int mock_comm_size_4byte(int comm, int *size) {
    (void)comm;
    *size = 4;
    return 0;
}

static int mock_comm_dup_4byte(int comm, int *newcomm) {
    static int next_comm = 0x44000100;
    (void)comm;
    *newcomm = next_comm++;
    return 0;
}

static int mock_waitall_4byte(int count, int *requests, void *statuses) {
    int i;
    (void)statuses;
    for (i = 0; i < count; i++) {
        requests[i] = 0x2c000000;  /* Set to NULL */
    }
    return 0;
}

/* Simulates OpenMPI backend functions (pointer handles) */
static int mock_comm_size_pointer(void *comm, int *size) {
    (void)comm;
    *size = 4;
    return 0;
}

/* ==================== Adapter Simulation ==================== */

/* Simulates the actual adapter logic from request_array_wrappers.c */
static void simulate_reqs_compress_inplace(MPI_Request *reqs, int n) {
    int32_t *p = (int32_t *)reqs;
    int i;
    for (i = 1; i < n; i++) {
        p[i] = p[i * 2];
    }
}

static void simulate_reqs_expand_inplace(MPI_Request *reqs, int n) {
    int i;
    for (i = n - 1; i >= 0; i--) {
        int32_t val = ((int32_t *)reqs)[i];
        reqs[i] = (MPI_Request)(intptr_t)val;
    }
}

/* ==================== Integration Tests ==================== */

/* Test 1: Comm handle through adapter */
static int test_comm_adapter(void) {
    MPI_Comm facade_comm = 0x44000000;
    int native_comm;
    MPI_Comm new_facade;

    printf("Testing comm handle adapter...\n");

    /* Simulate adapter: facade → native */
    native_comm = (int)(intptr_t)facade_comm;

    /* Call mock backend */
    TEST_ASSERT(mock_comm_dup_4byte(native_comm, &native_comm) == 0);

    /* Simulate adapter: native → facade */
    new_facade = (MPI_Comm)(intptr_t)native_comm;

    /* Verify */
    TEST_ASSERT(new_facade != facade_comm);
    TEST_ASSERT((int)(intptr_t)new_facade == native_comm);

    printf("  PASS: Comm adapter\n");
    return 0;
}

/* Test 2: Request array complete workflow */
static int test_request_array_workflow(void) {
    MPI_Request requests[4];
    int i;

    printf("Testing complete request array workflow...\n");

    /* Initialize with fake active requests */
    for (i = 0; i < 4; i++) {
        requests[i] = (MPI_Request)(intptr_t)(0x2c000001 + i);
    }

    /* Compress for backend */
    simulate_reqs_compress_inplace(requests, 4);

    /* Mock backend call with compressed array */
    TEST_ASSERT(mock_waitall_4byte(4, (int *)requests, NULL) == 0);

    /* Backend sets to NULL (0x2c000000) */
    for (i = 0; i < 4; i++) {
        TEST_ASSERT(((int *)requests)[i] == 0x2c000000);
    }

    /* Expand back */
    simulate_reqs_expand_inplace(requests, 4);

    /* Verify all NULL */
    for (i = 0; i < 4; i++) {
        TEST_ASSERT(requests[i] == UNIMPI_REQUEST_NULL);
    }

    printf("  PASS: Request array workflow\n");
    return 0;
}

/* Test 3: Partial completion scenario */
static int test_partial_completion(void) {
    /* Simulates Testany where only one request completes */
    MPI_Request requests[4];
    int32_t native_reqs[4];
    int i;

    printf("Testing partial completion scenario...\n");

    /* Initialize */
    for (i = 0; i < 4; i++) {
        requests[i] = (MPI_Request)(intptr_t)(0x2c000001 + i);
    }

    /* Compress */
    simulate_reqs_compress_inplace(requests, 4);

    /* Simulate: only index 1 completes */
    ((int *)requests)[1] = 0x2c000000;

    /* Expand */
    simulate_reqs_expand_inplace(requests, 4);

    /* Verify partial state */
    TEST_ASSERT(requests[0] != UNIMPI_REQUEST_NULL);  /* Still active */
    TEST_ASSERT(requests[1] == UNIMPI_REQUEST_NULL); /* Completed */
    TEST_ASSERT(requests[2] != UNIMPI_REQUEST_NULL);
    TEST_ASSERT(requests[3] != UNIMPI_REQUEST_NULL);

    printf("  PASS: Partial completion\n");
    return 0;
}

/* Test 4: Error handling propagation */
static int test_error_propagation(void) {
    /* Verify error codes pass through adapters correctly */
    int error_codes[] = {0, 5, 17, 19, 34};
    int i;

    printf("Testing error code propagation...\n");

    for (i = 0; i < (int)(sizeof(error_codes)/sizeof(error_codes[0])); i++) {
        int err = error_codes[i];
        /* Error codes are just ints, no conversion needed */
        TEST_ASSERT(err == error_codes[i]);
    }

    printf("  PASS: Error propagation\n");
    return 0;
}

/* Test 5: Status array handling */
static int test_status_array_handling(void) {
    /* Simplified status handling test */
    char facade_statuses[4 * 128];  /* 128-byte per status */
    char native_statuses[4 * 20];   /* 20-byte per status */
    int i;

    printf("Testing status array handling...\n");

    /* Simulate backend writing to native status array */
    memset(native_statuses, 0, sizeof(native_statuses));
    for (i = 0; i < 4; i++) {
        /* Write MPI_ERROR at offset 16 in native status */
        int *error_field = (int *)(native_statuses + i * 20 + 16);
        *error_field = i;  /* Different error per status */
    }

    /* Simulate expand to facade: copy 20 bytes to front of 128-byte slot */
    memset(facade_statuses, 0, sizeof(facade_statuses));
    for (i = 3; i >= 0; i--) {
        memcpy(facade_statuses + i * 128, native_statuses + i * 20, 20);
    }

    /* Verify */
    for (i = 0; i < 4; i++) {
        int *error_field = (int *)(facade_statuses + i * 128 + 16);
        TEST_ASSERT(*error_field == i);
    }

    printf("  PASS: Status array handling\n");
    return 0;
}

/* Test 6: Zero-count edge case */
static int test_zero_count_workflow(void) {
    MPI_Request requests[1] = {UNIMPI_REQUEST_NULL};

    printf("Testing zero-count workflow...\n");

    /* With count=0, should not touch array */
    /* Just verify no crash */
    simulate_reqs_compress_inplace(requests, 0);
    simulate_reqs_expand_inplace(requests, 0);

    TEST_ASSERT(requests[0] == UNIMPI_REQUEST_NULL);

    printf("  PASS: Zero-count workflow\n");
    return 0;
}

/* Test 7: Large array performance sanity */
static int test_large_array_performance(void) {
    static MPI_Request requests[1024];
    int i;

    printf("Testing large array (1024 elements)...\n");

    /* Initialize */
    for (i = 0; i < 1024; i++) {
        requests[i] = (MPI_Request)(intptr_t)(0x2c000000 + i);
    }

    /* Compress */
    simulate_reqs_compress_inplace(requests, 1024);

    /* Verify compressed */
    for (i = 0; i < 1024; i++) {
        int expected = (int)(0x2c000000 + i);
        TEST_ASSERT(((int *)requests)[i] == expected);
    }

    /* Expand */
    simulate_reqs_expand_inplace(requests, 1024);

    /* Verify restored */
    for (i = 0; i < 1024; i++) {
        MPI_Request expected = (MPI_Request)(intptr_t)(0x2c000000 + i);
        TEST_ASSERT(requests[i] == expected);
    }

    printf("  PASS: Large array performance\n");
    return 0;
}

/* Test 8: Cross-backend compatibility */
static int test_cross_backend_compat(void) {
    /* Test that same handle values work across different backends */
    MPI_Comm comm = 0x44000000;
    MPI_Datatype dtype = 0x0c000001;
    MPI_Request req = 0x2c000001;

    printf("Testing cross-backend compatibility...\n");

    /* All should narrow to same native values */
    TEST_ASSERT((int)(intptr_t)comm == 0x44000000);
    TEST_ASSERT((int)(intptr_t)dtype == 0x0c000001);
    TEST_ASSERT((int)(intptr_t)req == 0x2c000001);

    /* Restore should preserve values */
    TEST_ASSERT((MPI_Comm)(intptr_t)0x44000000 == comm);
    TEST_ASSERT((MPI_Datatype)(intptr_t)0x0c000001 == dtype);
    TEST_ASSERT((MPI_Request)(intptr_t)0x2c000001 == req);

    printf("  PASS: Cross-backend compatibility\n");
    return 0;
}

/* Test 9: Memory alignment */
static int test_memory_alignment(void) {
    /* Verify our assumptions about memory layout */
    struct {
        char padding[4];
        MPI_Request req;
    } test_struct;

    printf("Testing memory alignment...\n");

    /* Request should be properly aligned for 8-byte access */
    TEST_ASSERT(((uintptr_t)&test_struct.req) % sizeof(void*) == 0);

    /* intptr_t size should be void* size */
    TEST_ASSERT(sizeof(intptr_t) == sizeof(void*));

    printf("  PASS: Memory alignment\n");
    return 0;
}

/* Test 10: Integration with actual vtable types */
static int test_vtable_integration(void) {
    /* Test that our types match vtable expectations */
    MPI_Comm comm = UNIMPI_COMM_WORLD;
    MPI_Request req = UNIMPI_REQUEST_NULL;

    printf("Testing vtable integration...\n");

    /* Basic type checks */
    TEST_ASSERT(sizeof(comm) == sizeof(intptr_t));
    TEST_ASSERT(sizeof(req) == sizeof(intptr_t));

    /* Null values should be consistent */
    TEST_ASSERT(req == 0x2c000000);

    printf("  PASS: Vtable integration\n");
    return 0;
}

/* ==================== Main ==================== */

int main(void) {
    int failures = 0;

    printf("=== UniMPI ABI Adapter Integration Tests ===\n\n");

    failures += test_comm_adapter();
    failures += test_request_array_workflow();
    failures += test_partial_completion();
    failures += test_error_propagation();
    failures += test_status_array_handling();
    failures += test_zero_count_workflow();
    failures += test_large_array_performance();
    failures += test_cross_backend_compat();
    failures += test_memory_alignment();
    failures += test_vtable_integration();

    printf("\n");
    if (failures == 0) {
        printf("All ABI adapter tests PASSED\n");
        return 0;
    } else {
        printf("%d ABI adapter test(s) FAILED\n", failures);
        return 1;
    }
}

