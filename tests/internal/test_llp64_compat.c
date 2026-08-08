/* tests/internal/test_llp64_compat.c
 * LLP64 (Windows x64) platform compatibility tests.
 *
 * LLP64 platforms have:
 *   - int: 4 bytes
 *   - long: 4 bytes
 *   - long long: 8 bytes
 *   - intptr_t: 8 bytes
 *   - void*: 8 bytes
 *
 * This differs from LP64 (Linux/macOS x86_64):
 *   - int: 4 bytes
 *   - long: 8 bytes
 *   - intptr_t: 8 bytes
 *   - void*: 8 bytes
 *
 * The key difference is sign extension behavior when casting
 * int32_t -> intptr_t on LLP64 vs LP64.
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "unimpi_vtable.h"
#include "unimpi.h"

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s at line %d\n", #cond, __LINE__); \
        return 1; \
    } \
} while(0)

/* Detect platform type */
#if defined(_WIN64) && !defined(_LP64)
#define PLATFORM_LLP64 1
#define PLATFORM_LP64 0
#define PLATFORM_NAME "LLP64 (Windows x64)"
#elif defined(__LP64__) || (defined(__x86_64__) && !defined(_WIN64))
#define PLATFORM_LLP64 0
#define PLATFORM_LP64 1
#define PLATFORM_NAME "LP64 (Linux/macOS x86_64)"
#else
#define PLATFORM_LLP64 0
#define PLATFORM_LP64 0
#define PLATFORM_NAME "ILP32 or unknown"
#endif

/* ==================== Platform Detection ==================== */

static int test_platform_detection(void) {
    printf("Testing platform detection...\n");
    printf("  Platform: %s\n", PLATFORM_NAME);
    printf("  sizeof(int) = %zu\n", sizeof(int));
    printf("  sizeof(long) = %zu\n", sizeof(long));
    printf("  sizeof(long long) = %zu\n", sizeof(long long));
    printf("  sizeof(void*) = %zu\n", sizeof(void*));
    printf("  sizeof(intptr_t) = %zu\n", sizeof(intptr_t));

    /* Verify 64-bit platform */
    TEST_ASSERT(sizeof(void*) == 8);
    TEST_ASSERT(sizeof(intptr_t) == 8);

    /* Verify int is 4 bytes */
    TEST_ASSERT(sizeof(int) == 4);

#if PLATFORM_LLP64
    /* LLP64: long is 4 bytes */
    TEST_ASSERT(sizeof(long) == 4);
#elif PLATFORM_LP64
    /* LP64: long is 8 bytes */
    TEST_ASSERT(sizeof(long) == 8);
#endif

    printf("  PASS: Platform detection\n");
    return 0;
}

/* ==================== Sign Extension Tests ==================== */

/* Critical test: sign extension of 32-bit values to 64-bit */
static int test_sign_extension_behavior(void) {
    int32_t native;
    intptr_t facade;

    printf("Testing sign extension behavior...\n");

    /* Test 1: Positive value (sign bit = 0) */
    native = 0x44000000;  /* MPI_COMM_WORLD on integer backends */
    facade = (intptr_t)native;

#if PLATFORM_LLP64
    /* On LLP64, (intptr_t)(int32_t) positive value should be positive */
    TEST_ASSERT(facade == 0x0000000044000000LL);
    TEST_ASSERT(facade > 0);
#elif PLATFORM_LP64
    /* On LP64, same behavior */
    TEST_ASSERT(facade == 0x0000000044000000LL);
    TEST_ASSERT(facade > 0);
#endif

    /* Test 2: Negative value (sign bit = 1) */
    native = (int32_t)0x84000000;  /* Sign bit set */
    facade = (intptr_t)native;

    /* Both platforms: negative 32-bit should sign-extend to negative 64-bit */
    TEST_ASSERT(facade < 0);
    TEST_ASSERT((facade & 0xFFFFFFFF00000000LL) == 0xFFFFFFFF00000000LL);

    /* Test 3: All bits set (int32_t -1) */
    native = (int32_t)-1;
    facade = (intptr_t)native;
    TEST_ASSERT(facade == (intptr_t)-1);
    TEST_ASSERT(facade == 0xFFFFFFFFFFFFFFFFLL);

    /* Test 4: Zero */
    native = 0;
    facade = (intptr_t)native;
    TEST_ASSERT(facade == 0);

    printf("  PASS: Sign extension behavior\n");
    return 0;
}

/* ==================== Handle Conversion Tests ==================== */

/* Test handle conversion on both platforms */
static int test_handle_conversion(void) {
    printf("Testing handle conversion on %s...\n", PLATFORM_NAME);

    /* Test cases that might differ between platforms */
    struct {
        int32_t native;
        int64_t expected_lp64;
        int64_t expected_llp64;
        const char *name;
    } test_cases[] = {
        {0x44000000, 0x0000000044000000LL, 0x0000000044000000LL, "MPI_COMM_WORLD"},
        {0x44000001, 0x0000000044000001LL, 0x0000000044000001LL, "MPI_COMM_SELF"},
        {0x0c000001, 0x000000000c000001LL, 0x000000000c000001LL, "MPI_INT"},
        {0x2c000001, 0x000000002c000001LL, 0x000000002c000001LL, "Active request"},
        {(int32_t)0x84000000, 0xFFFFFFFF84000000LL, 0xFFFFFFFF84000000LL, "Handle with high bit"},
        {(int32_t)-1, 0xFFFFFFFFFFFFFFFFLL, 0xFFFFFFFFFFFFFFFFLL, "All bits set"},
    };

    int i;
    for (i = 0; i < (int)(sizeof(test_cases)/sizeof(test_cases[0])); i++) {
        intptr_t result = (intptr_t)test_cases[i].native;

        /* Verify sign extension is consistent */
        if (test_cases[i].native >= 0) {
            /* Positive: should be positive and match lower 32 bits */
            TEST_ASSERT(result == (intptr_t)(uint64_t)(uint32_t)test_cases[i].native);
        } else {
            /* Negative: should sign extend */
            TEST_ASSERT(result < 0);
        }

        /* Platform-specific check */
#if PLATFORM_LP64
        TEST_ASSERT(result == test_cases[i].expected_lp64);
#elif PLATFORM_LLP64
        TEST_ASSERT(result == test_cases[i].expected_llp64);
#endif
    }

    printf("  PASS: Handle conversion\n");
    return 0;
}

/* ==================== Array Operations ==================== */

/* Test array compression/expansion on this platform */
static int test_array_operations(void) {
    MPI_Request requests[8];
    int32_t *compressed;
    int i;

    printf("Testing array operations on %s...\n", PLATFORM_NAME);

    /* Initialize with typical values */
    for (i = 0; i < 8; i++) {
        requests[i] = (MPI_Request)(intptr_t)(0x2c000001 + i);
    }

    /* Compress in-place */
    compressed = (int32_t *)requests;
    for (i = 1; i < 8; i++) {
        compressed[i] = compressed[i * 2];
    }

    /* Verify compression (lower 32 bits preserved) */
    for (i = 0; i < 8; i++) {
        TEST_ASSERT(compressed[i] == (int32_t)(0x2c000001 + i));
    }

    /* Expand in-place with sign extension */
    for (i = 7; i >= 0; i--) {
        int32_t val = compressed[i];
        requests[i] = (MPI_Request)(intptr_t)val;
    }

    /* Verify restoration */
    for (i = 0; i < 8; i++) {
        MPI_Request expected = (UNIMPI_REQUEST_NULL + 1 + i);
        TEST_ASSERT(requests[i] == expected);
    }

    printf("  PASS: Array operations\n");
    return 0;
}

/* ==================== Edge Cases ==================== */

static int test_edge_cases(void) {
    printf("Testing edge cases...\n");

    /* Edge case 1: Maximum positive int32 */
    {
        int32_t native = INT32_MAX;  /* 0x7FFFFFFF */
        intptr_t facade = (intptr_t)native;
        TEST_ASSERT(facade == 0x7FFFFFFF);
        TEST_ASSERT(facade > 0);
    }

    /* Edge case 2: Minimum negative int32 */
    {
        int32_t native = INT32_MIN;  /* 0x80000000 */
        intptr_t facade = (intptr_t)native;
        TEST_ASSERT(facade < 0);
        /* Sign extended */
        TEST_ASSERT((facade & 0x7FFFFFFF00000000LL) == 0x7FFFFFFF00000000LL);
    }

    /* Edge case 3: Handle with all lower bits set */
    {
        MPI_Comm comm = (MPI_Comm)(intptr_t)0x44FFFFFF;
        int native = (int)(intptr_t)comm;
        MPI_Comm restored = (MPI_Comm)(intptr_t)native;
        TEST_ASSERT(restored == comm);
    }

    printf("  PASS: Edge cases\n");
    return 0;
}

/* ==================== Compatibility Warnings ==================== */

static int test_compatibility_warnings(void) {
    printf("Testing compatibility assumptions...\n");

    /* Warn about potential issues */

    /* Issue 1: C-style cast truncation warnings */
    /* On LLP64: (int)intptr_t_value may truncate silently */
    {
        intptr_t large = 0x0000000100000000LL;  /* > 32-bit */
        int truncated = (int)large;  /* Silent truncation */
        TEST_ASSERT(truncated == 0);  /* Truncated to 0 */
        /* This is expected but should be documented */
    }

    /* Issue 2: Sign bit preservation */
    {
        uint32_t unsigned_val = 0x84000000;
        int32_t signed_val = (int32_t)unsigned_val;  /* Sign bit set */
        intptr_t extended = (intptr_t)signed_val;  /* Sign extended */
        TEST_ASSERT(extended < 0);  /* Negative */
    }

    printf("  PASS: Compatibility warnings documented\n");
    return 0;
}

/* ==================== Summary ==================== */

static int print_summary(void) {
    printf("\n=== LLP64/LP64 Compatibility Summary ===\n");
    printf("Platform: %s\n", PLATFORM_NAME);
    printf("\nKey observations:\n");

    if (PLATFORM_LLP64) {
        printf("- LLP64 detected: long is 4 bytes\n");
        printf("- Use intptr_t, not long, for 64-bit integers\n");
        printf("- Handle sign extension carefully for high-bit handles\n");
    } else if (PLATFORM_LP64) {
        printf("- LP64 detected: long is 8 bytes\n");
        printf("- intptr_t and long are equivalent\n");
    }

    printf("\nConversion rules:\n");
    printf("- int32_t -> intptr_t: Always sign-extends on both platforms\n");
    printf("- intptr_t -> int32_t: Truncates upper 32 bits (implementation-defined)\n");
    printf("- Handle values with high bit (0x80000000+) become negative when sign-extended\n");

    return 0;
}

/* ==================== Main ==================== */

int main(void) {
    int failures = 0;

    printf("=== UniMPI LLP64/LP64 Compatibility Tests ===\n\n");

    failures += test_platform_detection();
    failures += test_sign_extension_behavior();
    failures += test_handle_conversion();
    failures += test_array_operations();
    failures += test_edge_cases();
    failures += test_compatibility_warnings();

    print_summary();

    printf("\n");
    if (failures == 0) {
        printf("All LLP64/LP64 compatibility tests PASSED\n");
        return 0;
    } else {
        printf("%d LLP64/LP64 compatibility test(s) FAILED\n", failures);
        return 1;
    }
}

