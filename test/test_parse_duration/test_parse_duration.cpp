#include <unity.h>
#include <cstdlib>

// Standalone copy of parseDurationMs for native testing (no Arduino dependency)
unsigned long parseDurationMs(const char* str) {
    if (!str || *str == '\0') return 0;

    int parts[3] = {0, 0, 0};
    int count = 0;
    const char* p = str;

    while (*p && count < 3) {
        parts[count] = atoi(p);
        if (parts[count] < 0) return 0;
        count++;
        while (*p && *p != ':') p++;
        if (*p == ':') p++;
    }

    if (count == 0) return 0;

    unsigned long hours = 0, minutes = 0, seconds = 0;
    if (count == 1) {
        minutes = parts[0];
    } else if (count == 2) {
        hours = parts[0];
        minutes = parts[1];
    } else {
        hours = parts[0];
        minutes = parts[1];
        seconds = parts[2];
    }

    return (hours * 3600UL + minutes * 60UL + seconds) * 1000UL;
}

// --- Single value (minutes) ---

void test_single_minutes(void) {
    TEST_ASSERT_EQUAL_UINT32(30 * 60 * 1000UL, parseDurationMs("30"));
}

void test_single_one_minute(void) {
    TEST_ASSERT_EQUAL_UINT32(1 * 60 * 1000UL, parseDurationMs("1"));
}

void test_single_zero(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs("0"));
}

void test_single_large_minutes(void) {
    TEST_ASSERT_EQUAL_UINT32(120 * 60 * 1000UL, parseDurationMs("120"));
}

// --- Two parts (hh:mm) ---

void test_hh_mm_basic(void) {
    TEST_ASSERT_EQUAL_UINT32((1 * 3600UL + 30 * 60UL) * 1000UL, parseDurationMs("1:30"));
}

void test_hh_mm_zero_hours(void) {
    TEST_ASSERT_EQUAL_UINT32(5 * 60 * 1000UL, parseDurationMs("0:05"));
}

void test_hh_mm_30_hours(void) {
    // "30:00" = 30 hours, 0 minutes
    TEST_ASSERT_EQUAL_UINT32(30 * 3600UL * 1000UL, parseDurationMs("30:00"));
}

void test_hh_mm_zero_zero(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs("0:00"));
}

void test_hh_mm_24_hours(void) {
    TEST_ASSERT_EQUAL_UINT32(24 * 3600UL * 1000UL, parseDurationMs("24:00"));
}

// --- Three parts (hh:mm:ss) ---

void test_hh_mm_ss_basic(void) {
    TEST_ASSERT_EQUAL_UINT32((1 * 3600UL + 30 * 60UL + 0) * 1000UL, parseDurationMs("1:30:00"));
}

void test_hh_mm_ss_with_seconds(void) {
    TEST_ASSERT_EQUAL_UINT32((0 * 3600UL + 5 * 60UL + 30) * 1000UL, parseDurationMs("0:05:30"));
}

void test_hh_mm_ss_seconds_only(void) {
    TEST_ASSERT_EQUAL_UINT32(45 * 1000UL, parseDurationMs("0:00:45"));
}

void test_hh_mm_ss_all_zero(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs("0:00:00"));
}

void test_hh_mm_ss_large(void) {
    TEST_ASSERT_EQUAL_UINT32((10 * 3600UL + 30 * 60UL + 15) * 1000UL, parseDurationMs("10:30:15"));
}

// --- Invalid input ---

void test_null(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs(NULL));
}

void test_empty_string(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs(""));
}

void test_non_numeric(void) {
    // atoi("abc") returns 0, so "abc" → 0 minutes → 0
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs("abc"));
}

void test_negative_value(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs("-5"));
}

void test_negative_in_hh_mm(void) {
    TEST_ASSERT_EQUAL_UINT32(0, parseDurationMs("-1:30"));
}

int main(void) {
    UNITY_BEGIN();

    // Single value (minutes)
    RUN_TEST(test_single_minutes);
    RUN_TEST(test_single_one_minute);
    RUN_TEST(test_single_zero);
    RUN_TEST(test_single_large_minutes);

    // Two parts (hh:mm)
    RUN_TEST(test_hh_mm_basic);
    RUN_TEST(test_hh_mm_zero_hours);
    RUN_TEST(test_hh_mm_30_hours);
    RUN_TEST(test_hh_mm_zero_zero);
    RUN_TEST(test_hh_mm_24_hours);

    // Three parts (hh:mm:ss)
    RUN_TEST(test_hh_mm_ss_basic);
    RUN_TEST(test_hh_mm_ss_with_seconds);
    RUN_TEST(test_hh_mm_ss_seconds_only);
    RUN_TEST(test_hh_mm_ss_all_zero);
    RUN_TEST(test_hh_mm_ss_large);

    // Invalid input
    RUN_TEST(test_null);
    RUN_TEST(test_empty_string);
    RUN_TEST(test_non_numeric);
    RUN_TEST(test_negative_value);
    RUN_TEST(test_negative_in_hh_mm);

    return UNITY_END();
}
