#ifndef FLASH_H_
#define FLASH_H_

#include "STD_TYPES.h"

/* Flash Memory Base Address */
#define FLASH_BASE                  0x40023C00UL

/* Flash Register Definitions */
#define FLASH_ACR                   (*(volatile uint32 *)(FLASH_BASE + 0x00))
#define FLASH_KEYR                  (*(volatile uint32 *)(FLASH_BASE + 0x04))
#define FLASH_OPTKEYR               (*(volatile uint32 *)(FLASH_BASE + 0x08))
#define FLASH_SR                    (*(volatile uint32 *)(FLASH_BASE + 0x0C))
#define FLASH_CR                    (*(volatile uint32 *)(FLASH_BASE + 0x10))
#define FLASH_OPTCR                 (*(volatile uint32 *)(FLASH_BASE + 0x14))

/* FLASH_CR bits */
#define FLASH_CR_LOCK               (1U << 31)
#define FLASH_CR_PG                 (1U << 0)
#define FLASH_CR_SER                (1U << 1)
#define FLASH_CR_MER                (1U << 2)
#define FLASH_CR_SNB_Pos            3
#define FLASH_CR_SNB_Msk            (0xFUL << FLASH_CR_SNB_Pos)
#define FLASH_CR_PSIZE_Pos          8
#define FLASH_CR_PSIZE_Msk          (3UL << FLASH_CR_PSIZE_Pos)
#define FLASH_CR_STRT               (1U << 16)

/* FLASH_SR bits */
#define FLASH_SR_EOP                (1U << 0)
#define FLASH_SR_OPERR              (1U << 1)
#define FLASH_SR_WRPERR             (1U << 4)
#define FLASH_SR_PGAERR             (1U << 5)
#define FLASH_SR_PGPERR             (1U << 6)
#define FLASH_SR_PGSERR             (1U << 7)
#define FLASH_SR_BSY                (1U << 16)

/* Error mask for all flash errors */
#define FLASH_SR_ERRORS_MASK        (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | \
                                    FLASH_SR_PGPERR | FLASH_SR_PGSERR)

/* Flash Keys */
#define FLASH_KEY1                  0x45670123U
#define FLASH_KEY2                  0xCDEF89ABU

/* Flash Memory Organization for STM32F446RE */
#define FLASH_BASE_ADDR             0x08000000U
#define FLASH_SIZE                  (512U * 1024U)  /* 512 KB */
#define FLASH_END_ADDR              (FLASH_BASE_ADDR + FLASH_SIZE)

/* Sector addresses for STM32F446RE */
#define SECTOR0_BASE                0x08000000U
#define SECTOR0_SIZE                16*1024
#define SECTOR1_BASE                0x08004000U
#define SECTOR1_SIZE                16*1024
#define SECTOR2_BASE                0x08008000U
#define SECTOR2_SIZE                16*1024
#define SECTOR3_BASE                0x0800C000U
#define SECTOR3_SIZE                16*1024
#define SECTOR4_BASE                0x08010000U
#define SECTOR4_SIZE                64*1024
#define SECTOR5_BASE                0x08020000U
#define SECTOR5_SIZE                128*1024
#define SECTOR6_BASE                0x08040000U
#define SECTOR6_SIZE                128*1024
#define SECTOR7_BASE                0x08060000U
#define SECTOR7_SIZE                128*1024

/* Programming size definitions */
#define FLASH_PSIZE_BYTE            0U
#define FLASH_PSIZE_HALF_WORD       1U
#define FLASH_PSIZE_WORD            2U
#define FLASH_PSIZE_DOUBLE_WORD     3U

/* Timeout definitions */
#define FLASH_DRV_TIMEOUT           5000000U

/* Driver Status */
typedef enum {
    FLASH_DRV_OK = 0,
    FLASH_DRV_ERROR,
    FLASH_DRV_TIMEOUT_ERROR,
    FLASH_DRV_ALIGNMENT_ERROR,
    FLASH_DRV_INVALID_ADDRESS,
    FLASH_DRV_LOCKED,
    FLASH_DRV_NOT_ERASED
} FlashDrv_Status_t;

/* Function Prototypes */
/* Initialization and control */
void FlashDrv_Unlock(void);
void FlashDrv_Lock(void);
uint8 FlashDrv_IsLocked(void);
void FlashDrv_ClearErrors(void);

/* Sector management */
uint32 FlashDrv_GetSector(uint32 Address);
uint32 FlashDrv_GetSectorBase(uint32 SectorNumber);
uint32 FlashDrv_GetSectorSize(uint32 SectorNumber);

/* Erase operations */
FlashDrv_Status_t FlashDrv_EraseSector(uint32 SectorNumber);
FlashDrv_Status_t FlashDrv_EraseRange(uint32 startAddress, uint32 length);
FlashDrv_Status_t FlashDrv_MassErase(void);

/* Program operations */
FlashDrv_Status_t FlashDrv_ProgramByte(uint32 Address, uint8 Data);
FlashDrv_Status_t FlashDrv_ProgramHalfWord(uint32 Address, uint16 Data);
FlashDrv_Status_t FlashDrv_ProgramWord(uint32 Address, uint32 Data);
FlashDrv_Status_t FlashDrv_ProgramBuffer(uint32 Address, const uint8 *buffer, uint32 length);
FlashDrv_Status_t FlashDrv_ProgramBufferAligned(uint32 Address, const uint8 *buffer, uint32 length);

/* Verification and utilities */
int FlashDrv_Verify(uint32 Address, const uint8 *buffer, uint32 length);
uint8 FlashDrv_IsErased(uint32 Address, uint32 length);
FlashDrv_Status_t FlashDrv_Read(uint32 Address, uint8 *buffer, uint32 length);

/* Safety functions */
void FlashDrv_PrepareForWrite(void);
void FlashDrv_FinishWrite(void);
FlashDrv_Status_t FlashDrv_ProgramWordSafe(uint32 Address, uint32 Data);

#endif /* FLASH_H_ */