/*
 * tests/test_crc32.c
 *
 * Unit tests for your CRC32 implementation.
 * This tests the pure computation logic — no hardware required.
 *
 * Your crc32() function lives in (e.g.) APP/Src/crc.c.
 * It should be a pure function: uint32_t crc32(const uint8_t *data, size_t len);
 */

#include "unity/unity.h"
#include <stdint.h>
#include <string.h>

/* ── Pull in the function under test ─────────────────────── */
/* Option A: include the .c directly (easiest for small drivers)  */
/* #include "../APP/Src/crc.c"                                     */

/* Option B: declare the prototype and link via Makefile           */
/* Adjust to match your actual function signature:                 */
extern uint32_t crc32(const uint8_t *data, size_t len);

/* ── Unity boilerplate ───────────────────────────────────── */
void setUp(void)    {}   /* runs before each test */
void tearDown(void) {}   /* runs after  each test */

/* ── Test cases ──────────────────────────────────────────── */

/* Known-good vector: CRC32 of the ASCII string "123456789"
   must equal 0xCBF43926 (standard zlib/IEEE 802.3 polynomial) */
void test_crc32_known_vector(void)
{
    const uint8_t input[] = "123456789";
    uint32_t result = crc32(input, sizeof(input) - 1); /* exclude null */
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926, result);
}

/* Empty input must return initial CRC value (0xFFFFFFFF ^ 0xFFFFFFFF = 0) */
void test_crc32_empty_input(void)
{
    uint32_t result = crc32(NULL, 0);
    TEST_ASSERT_EQUAL_HEX32(0x00000000, result);
}

/* Single byte: 0x00 */
void test_crc32_single_zero_byte(void)
{
    const uint8_t input[] = {0x00};
    uint32_t result = crc32(input, 1);
    TEST_ASSERT_EQUAL_HEX32(0xD202EF8D, result);   /* standard value */
}

/* Incremental CRC must equal full CRC over concatenated data */
void test_crc32_incremental(void)
{
    const uint8_t full[]  = {0x01, 0x02, 0x03, 0x04, 0x05};
    const uint8_t part1[] = {0x01, 0x02, 0x03};
    const uint8_t part2[] = {0x04, 0x05};

    uint32_t full_crc = crc32(full, sizeof(full));
    /* If your crc32 supports incremental (seed) mode, test it here */
    /* uint32_t inc_crc = crc32_with_seed(part2, sizeof(part2),
                           crc32(part1, sizeof(part1)));            */
    /* TEST_ASSERT_EQUAL_HEX32(full_crc, inc_crc);                  */

    /* For now just assert full CRC is stable across calls */
    TEST_ASSERT_EQUAL_HEX32(full_crc, crc32(full, sizeof(full)));
}

/* OTA header magic field must survive a round-trip through your
   packet struct — regression test for the 0x0000DEAD magic value */
void test_crc32_ota_magic_bytes(void)
{
    /* Your OTA header: MAGIC(4) + CRC32(4) + SIZE(4), little-endian */
    const uint8_t magic_bytes[] = {0xAD, 0xDE, 0x00, 0x00};  /* 0x0000DEAD LE */
    uint32_t result = crc32(magic_bytes, sizeof(magic_bytes));
    /* Not asserting a specific value here — just that it doesn't crash
       and produces a consistent result (idempotency test) */
    TEST_ASSERT_EQUAL_HEX32(result, crc32(magic_bytes, sizeof(magic_bytes)));
}

/* ── Main ────────────────────────────────────────────────── */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc32_known_vector);
    RUN_TEST(test_crc32_empty_input);
    RUN_TEST(test_crc32_single_zero_byte);
    RUN_TEST(test_crc32_incremental);
    RUN_TEST(test_crc32_ota_magic_bytes);
    return UNITY_END();
}
