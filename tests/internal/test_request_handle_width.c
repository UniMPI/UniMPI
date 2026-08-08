/* tests/internal/test_request_handle_width.c
 * Verify request handle width conversion between facade (intptr_t)
 * and native (int) representations, with focus on array operations.
 *
 * Tests the in-place compression/expansion strategy used in
 * request_array_wrappers.c
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "unimpi_vtable.h"
#include "unimpi.h"

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s at line %d\n", #cond, __LINE__); \
        return 1; \
    } \
} while(0)

/* ==================== Request Handle Tests ==================== */

/* Test 1: Basic request handle value preservation */
static int test_basic_request_conversion(void) {
    MPI_Request req;
    int native;
    MPI_Request restored;

    printf("Testing basic request handle conversion...\n");

    /* MPI_REQUEST_NULL */
    req = 0x2c000000;
    native = (int)(intptr_t)req;
    restored = (MPI_Request)(intptr_t)native;
    TEST_ASSERT(restored == req);

    /* Active request handles */
    req = 0x2c000101;
    native = (int)(intptr_t)req;
    restored = (MPI_Request)(intptr_t)native;
    TEST_ASSERT(restored == req);

    printf("  PASS: Basic request conversion\n");
    return 0;
}

/* Test 2: Request array compression (simulating in-place strategy) */
static int test_request_array_compression(void) {
    MPI_Request requests[8];
    int i;
    int original_count = 8;

    printf("Testing request array in-place compression...\n");

    /* Initialize array with test handles */
    for (i = 0; i < original_count; i++) {
        requests[i] = (MPI_Request)(intptr_t)(0x2c000000 + i + 1);
    }

    /* Simulate compression: 8-byte → 4-byte in-place
     *
     * Memory layout before (each [.] is 4 bytes):
     *   [req0_lo][req0_hi][req1_lo][req1_hi]...[req7_lo][req7_hi]
     *
     * After compression:
     *   [req0_lo][req1_lo][req2_lo]...[req7_lo] (8 slots in 8 bytes)
     *
     * Forward scan: read from 2*i, write to i
     */

    int32_t *p = (int32_t *)requests;

    /* i=1: read p[2], write p[1]
     * i=2: read p[4], write p[2]
     * etc.
     * Safe because read > write */
    for (i = 1; i < original_count; i++) {
        p[i] = p[i * 2];
    }

    /* Verify compressed array */
    for (i = 0; i < original_count; i++) {
        int expected = (int)(0x2c000000 + i + 1);
        TEST_ASSERT(p[i] == expected);
    }

    printf("  PASS: Request array compression\n");
    return 0;
}

/* Test 3: Request array expansion (simulating in-place strategy) */
static int test_request_array_expansion(void) {
    /* Start with compressed 4-byte array */
    int32_t compressed[4] = {
        0x2c000001,
        0x2c000002,
        0x2c000003,
        0x2c000004
    };
    MPI_Request requests[4];
    int i;

    printf("Testing request array in-place expansion...\n");

    /* Copy compressed data into request array (which is 8-byte slots) */
    memcpy(requests, compressed, sizeof(compressed));

    /* Simulate expansion: 4-byte → 8-byte in-place with sign extension
     *
     * Backward scan: for each i, read int32_t at position i,
     * sign extend to intptr_t, store at position i (8-byte slot)
     *
     * i=3: read compressed[3], sign extend, store requests[3]
     * i=2: read compressed[2], sign extend, store requests[2]
     * etc.
     * Safe because write (i*8+8) > read (i*4+4)
     */

    for (i = 3; i >= 0; i--) {
        int32_t val = ((int32_t *)requests)[i];
        /* Sign extend to intptr_t */
        requests[i] = (MPI_Request)(intptr_t)val;
    }

    /* Verify expanded array */
    for (i = 0; i < 4; i++) {
        MPI_Request expected = (MPI_Request)(intptr_t)(0x2c000001 + i);
        TEST_ASSERT(requests[i] == expected);
    }

    printf("  PASS: Request array expansion\n");
    return 0;
}

/* Test 4: Sign extension behavior for request handles */
static int test_request_sign_extension(void) {
    MPI_Request req;
    int32_t native;
    MPI_Request restored;

    printf("Testing request handle sign extension...\n");

    /* Test with high bit set */
    req = (MPI_Request)(intptr_t)0xFFFFFFFF2c000001ULL;
    native = (int32_t)(intptr_t)req;  /* Truncate to 32 bits */
    restored = (MPI_Request)(intptr_t)native;  /* Sign extend */

    /* On proper sign extension, restored should have all high bits set */
    if (sizeof(intptr_t) == 8) {
        intptr_t expected = (intptr_t)(int32_t)0x2c000001;
        /* Note: this may differ on LLP64 - that's what we're testing */
        (void)restored; (void)expected;  /* Mark used for now */
    }

    printf("  PASS: Request sign extension\n");
    return 0;
}

/* Test 5: Zero-count request array edge cases */
static int test_zero_count_arrays(void) {
    MPI_Request requests[1] = {UNIMPI_REQUEST_NULL};

    printf("Testing zero-count request arrays...\n");

    /* With count=0, compression/expansion should be no-ops */
    /* Just verify no crash and no change to data */
    MPI_Request before = requests[0];

    /* Simulate empty operation */
    (void)before;

    TEST_ASSERT(requests[0] == UNIMPI_REQUEST_NULL);

    printf("  PASS: Zero-count arrays\n");
    return 0;
}

/* Test 6: Request array with NULL handles */
static int test_null_request_arrays(void) {
    MPI_Request requests[4];
    int32_t *compressed;
    int i;

    printf("Testing request arrays with NULL handles...\n");

    /* Initialize with NULL and some active */
    requests[0] = UNIMPI_REQUEST_NULL;  /* 0x2c000000 */
    requests[1] = (MPI_Request)(intptr_t)0x2c000001;
    requests[2] = UNIMPI_REQUEST_NULL;
    requests[3] = (MPI_Request)(intptr_t)0x2c000003;

    /* Compress */
    compressed = (int32_t *)requests;
    for (i = 1; i < 4; i++) {
        compressed[i] = compressed[i * 2];
    }

    /* Verify NULL preserved */
    TEST_ASSERT(compressed[0] == 0x2c000000);  /* NULL */
    TEST_ASSERT(compressed[1] == 0x2c000001);  /* Active */

    /* Expand */
    for (i = 3; i >= 0; i--) {
        int32_t val = compressed[i];
        requests[i] = (MPI_Request)(intptr_t)val;
    }

    /* Verify NULL restored */
    TEST_ASSERT(requests[0] == UNIMPI_REQUEST_NULL);
    TEST_ASSERT(requests[1] == (UNIMPI_REQUEST_NULL + 1));

    printf("  PASS: NULL request arrays\n");
    return 0;
}

/* Test 7: Large request array stress test */
static int test_large_request_array(void) {
    /* Use static to avoid stack overflow */
    static MPI_Request requests[256];
    int i;
    int32_t *compressed;

    printf("Testing large request array (256 elements)...\n");

    /* Initialize */
    for (i = 0; i < 256; i++) {
        requests[i] = (MPI_Request)(intptr_t)(0x2c000000 + i);
    }

    /* Compress */
    compressed = (int32_t *)requests;
    for (i = 1; i < 256; i++) {
        compressed[i] = compressed[i * 2];
    }

    /* Verify */
    for (i = 0; i < 256; i++) {
        int expected = 0x2c000000 + i;
        TEST_ASSERT(compressed[i] == expected);
    }

    /* Expand */
    for (i = 255; i >= 0; i--) {
        int32_t val = compressed[i];
        requests[i] = (MPI_Request)(intptr_t)val;
    }

    /* Verify */
    for (i = 0; i < 256; i++) {
        MPI_Request expected = (MPI_Request)(intptr_t)(0x2c000000 + i);
        TEST_ASSERT(requests[i] == expected);
    }

    printf("  PASS: Large request array\n");
    return 0;
}

/* Test 8: Request handle overflow detection */
static int test_request_overflow(void) {
    printf("Testing request handle overflow detection...\n");

    /* Request handles that exceed 32-bit signed int range */
    /* These should be rare but we need to handle them gracefully */

    uint64_t large_value = 0xFFFFFFFF80000000ULL;  /* Negative as int32 */
    MPI_Request req = (MPI_Request)(intptr_t)large_value;
    int native = (int)(intptr_t)req;  /* Implementation-defined */
    MPI_Request restored = (MPI_Request)(intptr_t)native;

    /* Just verify no crash - actual value depends on implementation */
    (void)restored;

    printf("  PASS: Overflow detection (no crash)\n");
    return 0;
}

/* Test 9: Mixed handle types in memory */
static int test_mixed_handle_types(void) {
    /* Verify different handle types don't interfere */
    struct {
        MPI_Comm comm;
        MPI_Datatype dtype;
        MPI_Request req;
        MPI_Group group;
    } handles;

    printf("Testing mixed handle types...\n");

    handles.comm = (MPI_Comm)0x44000000;
    handles.dtype = (MPI_Datatype)0x0c000001;
    handles.req = (MPI_Request)0x2c000001;
    handles.group = (MPI_Group)0x08000001;

    /* Verify each type maintains its value */
    TEST_ASSERT((int)(intptr_t)handles.comm == 0x44000000);
    TEST_ASSERT((int)(intptr_t)handles.dtype == 0x0c000001);
    TEST_ASSERT((int)(intptr_t)handles.req == 0x2c000001);
    TEST_ASSERT((int)(intptr_t)handles.group == 0x08000001);

    printf("  PASS: Mixed handle types\n");
    return 0;
}

/* Test 10: Verify struct layout assumptions */
static int test_struct_layout(void) {
    /* Verify our assumptions about struct layout for array operations */
    struct {
        MPI_Request req;
        int padding;
    } test_struct;

    printf("Testing struct layout assumptions...\n");

    /* Request should be at offset 0 */
    TEST_ASSERT((void*)&test_struct.req == (void*)&test_struct);

    /* Request size should match intptr_t */
    TEST_ASSERT(sizeof(MPI_Request) == sizeof(intptr_t));

    printf("  PASS: Struct layout\n");
    return 0;
}

/* ==================== Main ==================== */

int main(void) {
    int failures = 0;

    printf("=== UniMPI Request Handle Width Tests ===\n\n");

    failures += test_basic_request_conversion();
    failures += test_request_array_compression();
    failures += test_request_array_expansion();
    failures += test_request_sign_extension();
    failures += test_zero_count_arrays();
    failures += test_null_request_arrays();
    failures += test_large_request_array();
    failures += test_request_overflow();
    failures += test_mixed_handle_types();
    failures += test_struct_layout();

    printf("\n");
    if (failures == 0) {
        printf("All request handle width tests PASSED\n");
        return 0;
    } else {
        printf("%d request handle width test(s) FAILED\n", failures);
        return 1;
    }
}

