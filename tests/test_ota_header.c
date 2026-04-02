/*
 * tests/test_ota_header.c
 *
 * Unit tests for OTA packet header parsing logic.
 *
 * Tests your header validation: MAGIC check, CRC32 verification,
 * SIZE field extraction — the exact logic that lives in BTLD.
 *
 * Your header struct is likely something like:
 *   typedef struct {
 *       uint32_t magic;   // 0x0000DEAD
 *       uint32_t crc32;
 *       uint32_t size;
 *   } OTA_Header_t;
 *
 * Adjust the include path and struct name to match your code.
 */

#include "unity/unity.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* ── Pull in the types/functions under test ──────────────── */
/* Adjust path: */
/* #include "../BTLD/Inc/ota.h" */

/* For now, define inline so this file is self-contained */
#define OTA_MAGIC  0x0000DEADu

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t crc32;
    uint32_t size;
} OTA_Header_t;

/* Stub crc32 — replace with your real one in the Makefile link */
static uint32_t crc32_stub(const uint8_t *data, size_t len)
{
    (void)data; (void)len;
    return 0xDEADBEEF;   /* deterministic stub */
}

/* Function under test — inline here; in your project this is in BTLD/Src/ota.c */
static bool OTA_ValidateHeader(const OTA_Header_t *hdr, uint32_t expected_crc)
{
    if (hdr == NULL)               return false;
    if (hdr->magic != OTA_MAGIC)   return false;
    if (hdr->size  == 0)           return false;
    if (hdr->size  > 0x70000u)     return false;  /* APP region ~448 KB */
    if (hdr->crc32 != expected_crc) return false;
    return true;
}

/* ── Unity boilerplate ───────────────────────────────────── */
void setUp(void)    {}
void tearDown(void) {}

/* ── Test cases ──────────────────────────────────────────── */

void test_valid_header_passes(void)
{
    OTA_Header_t hdr = {
        .magic = OTA_MAGIC,
        .crc32 = 0xDEADBEEF,
        .size  = 1024,
    };
    TEST_ASSERT_TRUE(OTA_ValidateHeader(&hdr, 0xDEADBEEF));
}

void test_wrong_magic_fails(void)
{
    OTA_Header_t hdr = {
        .magic = 0xCAFEBABE,   /* wrong */
        .crc32 = 0xDEADBEEF,
        .size  = 1024,
    };
    TEST_ASSERT_FALSE(OTA_ValidateHeader(&hdr, 0xDEADBEEF));
}

void test_zero_size_fails(void)
{
    OTA_Header_t hdr = {
        .magic = OTA_MAGIC,
        .crc32 = 0xDEADBEEF,
        .size  = 0,            /* invalid */
    };
    TEST_ASSERT_FALSE(OTA_ValidateHeader(&hdr, 0xDEADBEEF));
}

void test_oversized_payload_fails(void)
{
    OTA_Header_t hdr = {
        .magic = OTA_MAGIC,
        .crc32 = 0xDEADBEEF,
        .size  = 0x80000u,    /* 512 KB — too big for APP region */
    };
    TEST_ASSERT_FALSE(OTA_ValidateHeader(&hdr, 0xDEADBEEF));
}

void test_null_header_fails(void)
{
    TEST_ASSERT_FALSE(OTA_ValidateHeader(NULL, 0));
}

void test_bad_crc_fails(void)
{
    OTA_Header_t hdr = {
        .magic = OTA_MAGIC,
        .crc32 = 0x12345678,  /* computed CRC won't match */
        .size  = 512,
    };
    TEST_ASSERT_FALSE(OTA_ValidateHeader(&hdr, 0xDEADBEEF));
}

void test_header_is_12_bytes(void)
{
    /* Regression: packed struct must be exactly 12 bytes */
    TEST_ASSERT_EQUAL_UINT(12, sizeof(OTA_Header_t));
}

/* ── Main ────────────────────────────────────────────────── */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_header_passes);
    RUN_TEST(test_wrong_magic_fails);
    RUN_TEST(test_zero_size_fails);
    RUN_TEST(test_oversized_payload_fails);
    RUN_TEST(test_null_header_fails);
    RUN_TEST(test_bad_crc_fails);
    RUN_TEST(test_header_is_12_bytes);
    return UNITY_END();
}
