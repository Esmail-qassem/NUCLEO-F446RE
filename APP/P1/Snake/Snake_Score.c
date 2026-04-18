/***********************************************************************
 * File        : Snake_Score.c
 * Module      : Snake / Score manager
 *
 * See Snake_Score.h for the storage-layout rationale.
 *
 * Strategy in one line:
 *   append 8-byte records in sector 7 until it fills, then erase once
 *   (with in-loop IWDG refresh) and start over.
 ***********************************************************************/
#include "Snake_Score.h"
#include "FLASH.h"
#include "IWDG.h"

/*-----------------------------------------------------------------*/
/*  Storage location                                               */
/*-----------------------------------------------------------------*/
#define SNAKE_STORE_SECTOR     (7U)                 /* 128 KB sector */
#define SNAKE_STORE_ADDR       (0x08060000UL)
#define SNAKE_STORE_SIZE       (128UL * 1024UL)     /* 128 KB total  */
#define SNAKE_RECORD_SIZE      (8UL)                /* 2 x 32-bit    */
#define SNAKE_STORE_MAGIC      (0xA5D5F00DUL)
#define SNAKE_BLANK_WORD       (0xFFFFFFFFUL)

/*-----------------------------------------------------------------*/
/*  Module state                                                   */
/*-----------------------------------------------------------------*/
static uint16 s_score        = 0U;
static uint16 s_high_score   = 0U;
static uint16 s_high_flushed = 0U;

/* Byte offset inside sector 7 of the next free (all-0xFF) slot.
   Equal to SNAKE_STORE_SIZE when the sector is full.               */
static uint32 s_next_offset  = 0U;

/*-----------------------------------------------------------------*/
/*  Flash helpers (private)                                        */
/*-----------------------------------------------------------------*/

/* Read one record slot; returns 1 if it holds a valid record. */
static uint8 Slot_Read(uint32 offset, uint16 *out_val)
{
    const volatile uint32 *p_magic = (const volatile uint32 *)(SNAKE_STORE_ADDR + offset);
    const volatile uint16 *p_val   = (const volatile uint16 *)(SNAKE_STORE_ADDR + offset + 4UL);
    const volatile uint16 *p_inv   = (const volatile uint16 *)(SNAKE_STORE_ADDR + offset + 6UL);

    uint8 valid = 0U;

    if (*p_magic == SNAKE_STORE_MAGIC)
    {
        uint16 val = *p_val;
        uint16 inv = *p_inv;

        if ((uint16)(~val) == inv)
        {
            *out_val = val;
            valid = 1U;
        }
    }

    return valid;
}

/* Returns 1 if both words of the slot are fully erased (0xFFFFFFFF). */
static uint8 Slot_IsBlank(uint32 offset)
{
    const volatile uint32 *w0 = (const volatile uint32 *)(SNAKE_STORE_ADDR + offset);
    const volatile uint32 *w1 = (const volatile uint32 *)(SNAKE_STORE_ADDR + offset + 4UL);

    return ((*w0 == SNAKE_BLANK_WORD) && (*w1 == SNAKE_BLANK_WORD)) ? 1U : 0U;
}

/* Erase the whole storage sector while keeping the IWDG happy.
 *
 * This mirrors FlashDrv_EraseSector() but performs the BSY polling
 * in place, with an *inlined* IWDG refresh (direct IWDG_KR write)
 * inside each iteration. Two reasons for the inline:
 *   1. Reliability: during a flash erase the CPU stalls on every
 *      flash read, so only code already present in the ART cache
 *      keeps executing. Keeping the wait body tiny (a handful of
 *      Thumb instructions) ensures it fits easily in one cache
 *      line and never needs a new fetch from the busy flash.
 *   2. Timing: the watchdog gets fed on every poll iteration, so
 *      no matter how long the erase takes (up to ~3 s) it cannot
 *      fire.
 *
 * This function is only entered when the storage sector fills up
 * (after thousands of new-high-score events), so the long blocking
 * erase is effectively never hit in practice.
 */
static void Store_EraseSectorSafe(void)
{
    /* Unlock FLASH_CR. */
    if ((FLASH_CR & FLASH_CR_LOCK) != 0U)
    {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }

    /* Clear any previous status flags. */
    FLASH_SR |= (uint32)(FLASH_SR_EOP | FLASH_SR_ERRORS_MASK);

    /* Wait for any pending operation to finish (inline watchdog pet). */
    while ((FLASH_SR & FLASH_SR_BSY) != 0U)
    {
        IWDG_KR = IWDG_KEY_RELOAD;
    }

    /* Program size = 32-bit word (matches 3.3 V VDD on Nucleo). */
    FLASH_CR &= ~(uint32)FLASH_CR_PSIZE_Msk;
    FLASH_CR |=  ((uint32)FLASH_PSIZE_WORD << FLASH_CR_PSIZE_Pos);

    /* Select sector 7 and enable sector-erase. */
    FLASH_CR &= ~(uint32)FLASH_CR_SNB_Msk;
    FLASH_CR |=  ((uint32)SNAKE_STORE_SECTOR << FLASH_CR_SNB_Pos);
    FLASH_CR |=  (uint32)FLASH_CR_SER;

    /* Kick off. */
    FLASH_CR |= (uint32)FLASH_CR_STRT;

    /* Main erase wait -- the long one (up to ~3 s on a 128 KB sector).
       Body is deliberately a handful of instructions so it stays in the
       ART cache for the entire duration, independent of flash reads.   */
    while ((FLASH_SR & FLASH_SR_BSY) != 0U)
    {
        IWDG_KR = IWDG_KEY_RELOAD;
    }

    /* Cleanup. */
    FLASH_CR &= ~(uint32)FLASH_CR_SER;
    FLASH_CR |=  (uint32)FLASH_CR_LOCK;
}

/* Scan sector 7 at startup: find the newest valid record and the
   first free slot. The sector is filled monotonically from offset 0,
   so the first blank slot marks "next free" and everything after it
   is also blank -- we can stop scanning there.                       */
static void Store_Scan(void)
{
    uint16 latest_val = 0U;
    uint8  any_valid  = 0U;
    uint32 offset     = 0U;

    s_next_offset = SNAKE_STORE_SIZE;  /* assume full until proven otherwise */

    while (offset < SNAKE_STORE_SIZE)
    {
        if (Slot_IsBlank(offset) == 1U)
        {
            s_next_offset = offset;
            break;
        }

        {
            uint16 val = 0U;
            if (Slot_Read(offset, &val) == 1U)
            {
                latest_val = val;
                any_valid  = 1U;
            }
            /* else: slot is non-blank but corrupted -> skip it. */
        }

        offset += SNAKE_RECORD_SIZE;
    }

    if (any_valid == 1U)
    {
        s_high_score   = latest_val;
        s_high_flushed = latest_val;
    }
}

/* Append one record at s_next_offset. Erases the sector first if it
   is full (rare path, IWDG-safe).                                    */
static void Store_Append(uint16 value)
{
    uint32 addr;
    uint32 word0;
    uint32 word1;

    /* If the sector is full, perform the (rare) IWDG-safe erase. */
    if ((s_next_offset + SNAKE_RECORD_SIZE) > SNAKE_STORE_SIZE)
    {
        Store_EraseSectorSafe();
        s_next_offset = 0U;
    }

    addr = SNAKE_STORE_ADDR + s_next_offset;

    /* word0 = magic, word1 = [value][~value]. */
    word0 = SNAKE_STORE_MAGIC;
    word1 = ((uint32)value) | (((uint32)((uint16)~value)) << 16U);

    (void)FlashDrv_ProgramWord(addr,         word0);
    (void)FlashDrv_ProgramWord(addr + 4UL,   word1);

    s_next_offset += SNAKE_RECORD_SIZE;
}

/*-----------------------------------------------------------------*/
/*  Public API                                                     */
/*-----------------------------------------------------------------*/
void SnakeScore_Init(void)
{
    s_score        = 0U;
    s_high_score   = 0U;
    s_high_flushed = 0U;
    s_next_offset  = 0U;

    Store_Scan();
}

void SnakeScore_Reset(void)
{
    s_score = 0U;
}

void SnakeScore_Increment(void)
{
    s_score++;

    if (s_score > s_high_score)
    {
        s_high_score = s_score;
    }
}

void SnakeScore_Persist(void)
{
    /* Only write when the high score actually changed. A program
       operation writes microseconds worth of work -- negligible
       against the IWDG.                                             */
    if (s_high_score != s_high_flushed)
    {
        Store_Append(s_high_score);
        s_high_flushed = s_high_score;
    }
}

uint16 SnakeScore_GetCurrent(void)
{
    return s_score;
}

uint16 SnakeScore_GetHigh(void)
{
    return s_high_score;
}
