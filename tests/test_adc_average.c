/*
 * tests/test_adc_average.c
 *
 * Unit tests for the ADC multi-sample averaging logic.
 *
 * Your bare-metal ADC driver does 16-sample averaging with
 * 480-cycle sampling. The averaging math is pure logic —
 * no hardware needed to test it.
 *
 * Extract the averaging function from your driver:
 *   uint32_t ADC_Average(const uint32_t *samples, uint8_t count);
 * or test it inline as shown here.
 */

#include "unity/unity.h"
#include <stdint.h>
#include <string.h>

/* ── Function under test ─────────────────────────────────── */
/* Include your actual driver if it separates logic cleanly:  */
/* #include "../APP/Src/adc_driver.c"                         */

/* For demo, the averaging function is re-implemented here.   */
/* Replace with a call to your real function once you refactor. */

#define ADC_NUM_SAMPLES  16u
#define ADC_MAX_VALUE    4095u   /* 12-bit ADC */

static uint32_t ADC_Average(const uint32_t *samples, uint8_t count)
{
    if (samples == NULL || count == 0) return 0;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += samples[i];
    }
    return sum / count;
}

/* LDR raw-to-percentage: 0 = dark, 100 = bright (example) */
static uint8_t LDR_ToPercent(uint32_t raw)
{
    if (raw > ADC_MAX_VALUE) raw = ADC_MAX_VALUE;
    return (uint8_t)((raw * 100u) / ADC_MAX_VALUE);
}

/* ── Unity boilerplate ───────────────────────────────────── */
void setUp(void)    {}
void tearDown(void) {}

/* ── Test cases ──────────────────────────────────────────── */

void test_average_uniform_samples(void)
{
    uint32_t samples[ADC_NUM_SAMPLES];
    for (int i = 0; i < ADC_NUM_SAMPLES; i++) samples[i] = 2048;
    TEST_ASSERT_EQUAL_UINT32(2048, ADC_Average(samples, ADC_NUM_SAMPLES));
}

void test_average_zero_samples(void)
{
    uint32_t samples[ADC_NUM_SAMPLES];
    memset(samples, 0, sizeof(samples));
    TEST_ASSERT_EQUAL_UINT32(0, ADC_Average(samples, ADC_NUM_SAMPLES));
}

void test_average_max_samples(void)
{
    uint32_t samples[ADC_NUM_SAMPLES];
    for (int i = 0; i < ADC_NUM_SAMPLES; i++) samples[i] = ADC_MAX_VALUE;
    TEST_ASSERT_EQUAL_UINT32(ADC_MAX_VALUE, ADC_Average(samples, ADC_NUM_SAMPLES));
}

void test_average_mixed_samples(void)
{
    /* 8 samples at 0, 8 samples at 4095 → average should be ~2047 */
    uint32_t samples[ADC_NUM_SAMPLES];
    for (int i = 0; i < 8;  i++) samples[i]     = 0;
    for (int i = 8; i < 16; i++) samples[i]     = ADC_MAX_VALUE;
    uint32_t avg = ADC_Average(samples, ADC_NUM_SAMPLES);
    TEST_ASSERT_UINT32_WITHIN(2, 2047, avg);   /* allow ±2 for integer division */
}

void test_average_null_returns_zero(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, ADC_Average(NULL, ADC_NUM_SAMPLES));
}

void test_average_count_zero_returns_zero(void)
{
    uint32_t samples[4] = {100, 200, 300, 400};
    TEST_ASSERT_EQUAL_UINT32(0, ADC_Average(samples, 0));
}

/* ── LDR percentage conversion tests ─────────────────────── */

void test_ldr_full_dark(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, LDR_ToPercent(0));
}

void test_ldr_full_bright(void)
{
    TEST_ASSERT_EQUAL_UINT8(100, LDR_ToPercent(ADC_MAX_VALUE));
}

void test_ldr_midpoint(void)
{
    /* ~2047 raw → ~50% */
    uint8_t pct = LDR_ToPercent(2047);
    TEST_ASSERT_UINT8_WITHIN(2, 50, pct);
}

void test_ldr_clamps_overflow(void)
{
    /* Values above 4095 must be clamped, not wrap around */
    TEST_ASSERT_EQUAL_UINT8(100, LDR_ToPercent(9999));
}

/* ── Main ────────────────────────────────────────────────── */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_average_uniform_samples);
    RUN_TEST(test_average_zero_samples);
    RUN_TEST(test_average_max_samples);
    RUN_TEST(test_average_mixed_samples);
    RUN_TEST(test_average_null_returns_zero);
    RUN_TEST(test_average_count_zero_returns_zero);
    RUN_TEST(test_ldr_full_dark);
    RUN_TEST(test_ldr_full_bright);
    RUN_TEST(test_ldr_midpoint);
    RUN_TEST(test_ldr_clamps_overflow);
    return UNITY_END();
}
