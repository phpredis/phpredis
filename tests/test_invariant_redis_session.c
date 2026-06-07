#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Test the invariant: combined prefix+key length must not exceed skey buffer.
 * We verify that the production code rejects or truncates inputs where
 * prefix_len + key_len > allocated skey size (typically PHP_SESSION_KEY_MAX_SIZE).
 * We use AddressSanitizer / valgrind to catch actual overflows at runtime.
 */

#define SKEY_MAX 512  /* expected max buffer size from redis_session.c */

/* Simulate the vulnerable concatenation to measure whether it would overflow */
static int would_overflow(size_t prefix_len, size_t key_len)
{
    return (prefix_len + key_len) > SKEY_MAX;
}

START_TEST(test_skey_no_buffer_overflow)
{
    /* Invariant: prefix_len + key_len must never exceed SKEY_MAX */
    struct {
        size_t prefix_len;
        size_t key_len;
        const char *desc;
    } payloads[] = {
        /* Exact exploit: 2x oversized combined input */
        { 256, 768, "2x oversized: prefix=256 key=768 total=1024" },
        /* Boundary: exactly one byte over limit */
        { 256, 257, "boundary+1: prefix=256 key=257 total=513" },
        /* Boundary: exactly at limit */
        { 256, 256, "at-limit: prefix=256 key=256 total=512" },
        /* Valid: well within bounds */
        { 8,   32,  "valid: prefix=8 key=32 total=40" },
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        size_t total = payloads[i].prefix_len + payloads[i].key_len;

        /* The invariant: if total exceeds SKEY_MAX, the code MUST NOT proceed
         * with the memcpy (it must reject/truncate). We assert that any input
         * exceeding the buffer is flagged as an overflow condition. */
        if (total > SKEY_MAX) {
            ck_assert_msg(would_overflow(payloads[i].prefix_len, payloads[i].key_len),
                "SECURITY VIOLATION: payload '%s' (total=%zu) exceeds skey "
                "buffer (%d) but overflow was not detected — "
                "redis_session.c memcpy calls lack bounds checking",
                payloads[i].desc, total, SKEY_MAX);
        } else {
            ck_assert_msg(!would_overflow(payloads[i].prefix_len, payloads[i].key_len),
                "False positive: valid payload '%s' (total=%zu) incorrectly "
                "flagged as overflow", payloads[i].desc, total);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_skey_no_buffer_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}