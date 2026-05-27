/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : FLASH                                                  */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

constexpr uint32 FLASH_REG_BASE = 0x40023C00UL;

struct Flash_RegDef_t
{
    volatile uint32 ACR;    /* 0x00 */
    volatile uint32 KEYR;   /* 0x04 */
    volatile uint32 OPTKEYR;/* 0x08 */
    volatile uint32 SR;     /* 0x0C */
    volatile uint32 CR;     /* 0x10 */
    volatile uint32 OPTCR;  /* 0x14 */
};

#define FLASH_REGS (reinterpret_cast<Flash_RegDef_t*>(FLASH_REG_BASE))

constexpr uint32 FLASH_CR_LOCK   = (1u << 31u);
constexpr uint32 FLASH_CR_PG     = (1u <<  0u);
constexpr uint32 FLASH_CR_SER    = (1u <<  1u);
constexpr uint32 FLASH_CR_MER    = (1u <<  2u);
constexpr uint8  FLASH_CR_SNB_Pos = 3u;
constexpr uint32 FLASH_CR_SNB_Msk = (0xFu << 3u);
constexpr uint8  FLASH_CR_PSIZE_Pos = 8u;
constexpr uint32 FLASH_CR_PSIZE_Msk = (3u << 8u);
constexpr uint32 FLASH_CR_STRT   = (1u << 16u);

constexpr uint32 FLASH_SR_BSY    = (1u << 16u);
constexpr uint32 FLASH_SR_EOP    = (1u <<  0u);
constexpr uint32 FLASH_SR_ERRORS_MASK = (0x1Fu << 1u);

constexpr uint32 FLASH_KEY1 = 0x45670123UL;
constexpr uint32 FLASH_KEY2 = 0xCDEF89ABuL;

constexpr uint32 FLASH_MEM_BASE  = 0x08000000UL;
constexpr uint32 FLASH_MEM_SIZE  = 512u * 1024u;
constexpr uint32 FLASH_MEM_END   = FLASH_MEM_BASE + FLASH_MEM_SIZE;

constexpr uint32 FLASH_DRV_TIMEOUT = 5000000u;

constexpr uint8 FLASH_PSIZE_BYTE       = 0u;
constexpr uint8 FLASH_PSIZE_HALF_WORD  = 1u;
constexpr uint8 FLASH_PSIZE_WORD       = 2u;
constexpr uint8 FLASH_PSIZE_DOUBLE_WORD= 3u;

/* Sector bases */
constexpr uint32 SECTOR0_BASE = 0x08000000u; constexpr uint32 SECTOR0_SIZE = 16u*1024u;
constexpr uint32 SECTOR1_BASE = 0x08004000u; constexpr uint32 SECTOR1_SIZE = 16u*1024u;
constexpr uint32 SECTOR2_BASE = 0x08008000u; constexpr uint32 SECTOR2_SIZE = 16u*1024u;
constexpr uint32 SECTOR3_BASE = 0x0800C000u; constexpr uint32 SECTOR3_SIZE = 16u*1024u;
constexpr uint32 SECTOR4_BASE = 0x08010000u; constexpr uint32 SECTOR4_SIZE = 64u*1024u;
constexpr uint32 SECTOR5_BASE = 0x08020000u; constexpr uint32 SECTOR5_SIZE = 128u*1024u;
constexpr uint32 SECTOR6_BASE = 0x08040000u; constexpr uint32 SECTOR6_SIZE = 128u*1024u;
constexpr uint32 SECTOR7_BASE = 0x08060000u; constexpr uint32 SECTOR7_SIZE = 128u*1024u;

enum class FlashDrv_Status : uint8
{
    OK = 0,
    ERROR,
    TIMEOUT_ERROR,
    ALIGNMENT_ERROR,
    INVALID_ADDRESS,
    LOCKED,
    NOT_ERASED
};

class FlashDrv
{
public:
    static void            Unlock          (void);
    static void            Lock            (void);
    static uint8           IsLocked        (void);
    static void            ClearErrors     (void);
    static uint32          GetSector       (uint32 address);
    static uint32          GetSectorBase   (uint32 sectorNum);
    static uint32          GetSectorSize   (uint32 sectorNum);
    static FlashDrv_Status EraseSector     (uint32 sectorNum);
    static FlashDrv_Status EraseRange      (uint32 start, uint32 length);
    static FlashDrv_Status MassErase       (void);
    static FlashDrv_Status ProgramByte     (uint32 addr, uint8  data);
    static FlashDrv_Status ProgramHalfWord (uint32 addr, uint16 data);
    static FlashDrv_Status ProgramWord     (uint32 addr, uint32 data);
    static FlashDrv_Status ProgramBuffer   (uint32 addr, const uint8 *buf, uint32 len);
    static FlashDrv_Status ProgramBufferAligned(uint32 addr, const uint8 *buf, uint32 len);
    static int             Verify          (uint32 addr, const uint8 *buf, uint32 len);
    static uint8           IsErased        (uint32 addr, uint32 len);
    static FlashDrv_Status Read            (uint32 addr, uint8 *buf, uint32 len);
    static void            PrepareForWrite (void);
    static void            FinishWrite     (void);
    static FlashDrv_Status ProgramWordSafe (uint32 addr, uint32 data);
private:
    static FlashDrv_Status WaitForLastOp  (void);
    static uint8           IsValidAddress (uint32 addr);
    FlashDrv() = delete;
};
