#ifndef FLASH_H_
#define FLASH_H_
#include "STD_TYPES.h"


#define FLASH_BASE      0x40023C00UL
#define FLASH_ACR       (*(volatile uint32 *)(FLASH_BASE + 0x00))
#define FLASH_KEYR      (*(volatile uint32 *)(FLASH_BASE + 0x04))
#define FLASH_OPTKEYR   (*(volatile uint32 *)(FLASH_BASE + 0x08))
#define FLASH_SR        (*(volatile uint32 *)(FLASH_BASE + 0x0C))
#define FLASH_CR        (*(volatile uint32 *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR     (*(volatile uint32 *)(FLASH_BASE + 0x14))

/* FLASH_CR bits */
#define FLASH_CR_LOCK   (1U << 31)
#define FLASH_CR_PG     (1U << 0)
#define FLASH_CR_SER    (1U << 1)
#define FLASH_CR_MER    (1U << 2)
#define FLASH_CR_SNB_Pos 3
#define FLASH_CR_STRT   (1U << 16)
#define FLASH_CR_PSIZE_Pos 8

/* FLASH_SR bits */
#define FLASH_SR_BSY    (1U << 16)
#define FLASH_SR_EOP    (1U << 0)
#define FLASH_SR_PGSERR (1U << 7)
#define FLASH_SR_PGPERR (1U << 6)
#define FLASH_SR_PGAERR (1U << 5)
#define FLASH_SR_WRPERR (1U << 4)
#define FLASH_SR_OPERR  (1U << 1)
#define FLASH_SR_ERRORS_MASK (FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR | FLASH_SR_WRPERR | FLASH_SR_OPERR)

/* FLASH keys */
#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

/* Simple crude timeout counter for bootloader */
#define FLASH_DRV_TIMEOUT 5000000U


typedef enum {
    FLASH_DRV_OK = 0,
    FLASH_DRV_ERROR,
    FLASH_DRV_TIMEOUT_ERROR,
    FLASH_DRV_ALIGNMENT_ERROR,
} FlashDrv_Status_t;

/* Sector numbers for STM32F446RE */
uint32 FlashDrv_GetSector(uint32 Address);

/* Unlock/lock control */
void FlashDrv_Unlock(void);
void FlashDrv_Lock(void);

/* Erase one sector (by number) */
FlashDrv_Status_t FlashDrv_EraseSector(uint32 SectorNumber);

/* Erase a flash address range (erases all sectors covering range) */
FlashDrv_Status_t FlashDrv_EraseRange(uint32 startAddress, uint32 length);

/* Program primitives */
FlashDrv_Status_t FlashDrv_ProgramHalfWord(uint32 Address, uint16 Data);
FlashDrv_Status_t FlashDrv_ProgramWord(uint32 Address, uint32 Data);

/* Program a buffer (best if word-aligned) */
FlashDrv_Status_t FlashDrv_ProgramBuffer(uint32 Address, const uint8 *buffer, uint8 length);

/* Verification */
int FlashDrv_Verify(uint32 Address, const uint8 *buffer, uint8 length);

/* Safety functions */
void FlashDrv_PrepareForWrite(void);
void FlashDrv_FinishWrite(void);


#endif /* FLASH_DRV_H */
