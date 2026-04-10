#include "CRC.h"
#include "RCC.h"

/*===========================================================================
  STM32F446RE Hardware CRC-32 Driver
  Polynomial : 0x04C11DB7  (same as Ethernet / ZIP / PNG)
  Initial val: 0xFFFFFFFF  (set by hardware on reset)
  Input order : word fed MSB first (hardware handles internally)

  NOTE:
    The hardware CRC unit feeds 32-bit WORDS.
    For byte buffers: pack 4 bytes per word, big-endian (byte[0] in MSByte).
    If buffer length is not a multiple of 4, the last partial word is
    zero-padded on the RIGHT (LSByte side).

    The ESP and server must use the same byte→word packing when computing
    the CRC to append to the firmware image.
===========================================================================*/

/* ── Init ───────────────────────────────────────────────────────────────── */
void CRC_Init(void)
{
    RCC_EnableClock(RCC_AHB1, AHB1_CRC);
    CRC_Reset();
}

/* ── Reset accumulator ──────────────────────────────────────────────────── */
void CRC_Reset(void)
{
    CRC_CR = (1U << CRC_CR_RESET_BIT);
    /* Reset is synchronous — one AHB cycle, no poll needed */
}

/* ── Feed one 32-bit word ───────────────────────────────────────────────── */
uint32 CRC_AccumulateWord(uint32 word)
{
    CRC_DR = word;
    return CRC_DR;   /* reading DR returns running CRC */
}

/* ── Full buffer calculation ────────────────────────────────────────────── */
uint32 CRC_Calculate(const uint8* buf, uint32 len)
{
    uint32 i;
    uint32 word;
    uint32 remaining = len % 4U;
    uint32 full_words = len / 4U;

    CRC_Reset();

    /* Feed complete 32-bit words — pack bytes MSB first */
    for (i = 0U; i < full_words; i++)
    {
        word = ((uint32)buf[i*4U + 0U] << 24U)
             | ((uint32)buf[i*4U + 1U] << 16U)
             | ((uint32)buf[i*4U + 2U] <<  8U)
             | ((uint32)buf[i*4U + 3U]);
        CRC_DR = word;
    }

    /* Handle trailing bytes (pad with 0x00 on right) */
    if (remaining > 0U)
    {
        word = 0U;
        uint32 base = full_words * 4U;
        switch (remaining)
        {
            case 3U: word |= ((uint32)buf[base + 2U] <<  8U); /* fall-through */
            case 2U: word |= ((uint32)buf[base + 1U] << 16U); /* fall-through */
            case 1U: word |= ((uint32)buf[base + 0U] << 24U); break;
            default: break;
        }
        CRC_DR = word;
    }

    return CRC_DR;
}

/* ── Verify buffer with appended 4-byte CRC ────────────────────────────── */
uint8 CRC_Verify(const uint8* buf, uint32 total_len)
{
    uint32 expected_crc;
    uint32 computed_crc;
    uint32 payload_len;

    if (total_len < 5U)
    {
        return CRC_ERR;   /* impossible to have payload + 4-byte CRC */
    }

    payload_len = total_len - 4U;

    /* Read the appended CRC (big-endian, last 4 bytes) */
    expected_crc = ((uint32)buf[payload_len + 0U] << 24U)
                 | ((uint32)buf[payload_len + 1U] << 16U)
                 | ((uint32)buf[payload_len + 2U] <<  8U)
                 | ((uint32)buf[payload_len + 3U]);

    /* Compute CRC over the payload only */
    computed_crc = CRC_Calculate(buf, payload_len);

    return (computed_crc == expected_crc) ? CRC_OK : CRC_ERR;
}

/* ── Read current accumulator ───────────────────────────────────────────── */
uint32 CRC_GetResult(void)
{
    return CRC_DR;
}
