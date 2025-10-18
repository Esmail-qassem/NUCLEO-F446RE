#include"FLASH.h"


void FlashDrv_Unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

void FlashDrv_Lock(void)
{
    FLASH_CR |= FLASH_CR_LOCK;
}

void FlashDrv_ClearErrors(void)
{
    FLASH_SR = FLASH_SR_EOP | FLASH_SR_ERRORS_MASK;
}

static FlashDrv_Status_t FlashDrv_WaitForLastOperation(void)
{
    uint32 timeout = FLASH_DRV_TIMEOUT;
    while ((FLASH_SR & FLASH_SR_BSY) && timeout--) ;
    if (timeout == 0) return FLASH_DRV_TIMEOUT;
    if (FLASH_SR & FLASH_SR_ERRORS_MASK) return FLASH_DRV_ERROR;
    return FLASH_DRV_OK;
}

/* -------------------------
   Sector mapping
   ------------------------- */
uint32 FlashDrv_GetSector(uint32 Address)
{
    if (Address < 0x08004000U) return 0;
    if (Address < 0x08008000U) return 1;
    if (Address < 0x0800C000U) return 2;
    if (Address < 0x08010000U) return 3;
    if (Address < 0x08020000U) return 4;
    if (Address < 0x08040000U) return 5;
    if (Address < 0x08060000U) return 6;
    return 7;
}

/* -------------------------
   Erase
   ------------------------- */
FlashDrv_Status_t FlashDrv_EraseSector(uint32 SectorNumber)
{
    FlashDrv_Status_t st;

    FlashDrv_Unlock();
    FlashDrv_ClearErrors();

    /* configure sector erase */
    FLASH_CR &= ~(0xFU << FLASH_CR_SNB_Pos);
    FLASH_CR |= FLASH_CR_SER | (SectorNumber << FLASH_CR_SNB_Pos);

    /* start erase */
    FLASH_CR |= FLASH_CR_STRT;

    st = FlashDrv_WaitForLastOperation();

    FLASH_CR &= ~FLASH_CR_SER;
    FlashDrv_Lock();
    return st;
}

FlashDrv_Status_t FlashDrv_EraseRange(uint32 startAddress, uint32 length)
{
    uint32 end = startAddress + length;
    uint32 addr = startAddress;
    FlashDrv_Status_t st = FLASH_DRV_OK;

    while (addr < end) {
        uint32 sector = FlashDrv_GetSector(addr);
        st = FlashDrv_EraseSector(sector);
        if (st != FLASH_DRV_OK) return st;
        /* advance to next sector */
        switch (sector) {
            case 0: addr = 0x08004000U; break;
            case 1: addr = 0x08008000U; break;
            case 2: addr = 0x0800C000U; break;
            case 3: addr = 0x08010000U; break;
            case 4: addr = 0x08020000U; break;
            case 5: addr = 0x08040000U; break;
            case 6: addr = 0x08060000U; break;
            default: addr = 0x08080000U; break;
        }
    }
    return FLASH_DRV_OK;
}

/* -------------------------
   Program
   ------------------------- */
FlashDrv_Status_t FlashDrv_ProgramHalfWord(uint32 Address, uint16 Data)
{
    if (Address & 0x1U) return FLASH_DRV_ALIGNMENT_ERROR;

    FlashDrv_Unlock();
    FlashDrv_ClearErrors();

    FLASH_CR &= ~(3U << FLASH_CR_PSIZE_Pos); /* half-word = 00 */
    FLASH_CR |= FLASH_CR_PG;

    *((volatile uint16*)Address) = Data;

    FlashDrv_Status_t st = FlashDrv_WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_PG;
    FlashDrv_Lock();
    return st;
}

FlashDrv_Status_t FlashDrv_ProgramWord(uint32 Address, uint32 Data)
{
    if (Address & 0x3U) return FLASH_DRV_ALIGNMENT_ERROR;

    FlashDrv_Unlock();
    FlashDrv_ClearErrors();

    FLASH_CR &= ~(3U << FLASH_CR_PSIZE_Pos);
    FLASH_CR |= (1U << FLASH_CR_PSIZE_Pos); /* word = 01 */
    FLASH_CR |= FLASH_CR_PG;

    *((volatile uint32*)Address) = Data;

    FlashDrv_Status_t st = FlashDrv_WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_PG;
    FlashDrv_Lock();
    return st;
}

FlashDrv_Status_t FlashDrv_ProgramBuffer(uint32 Address, const uint8 *buffer, uint8 length)
{
    uint8 i = 0;
    FlashDrv_Status_t st;

    if (Address & 0x1U) return FLASH_DRV_ALIGNMENT_ERROR;

    while (i + 4 <= length) {
        uint32 val = buffer[i] | (buffer[i+1]<<8) | (buffer[i+2]<<16) | (buffer[i+3]<<24);
        st = FlashDrv_ProgramWord(Address + i, val);
        if (st != FLASH_DRV_OK) return st;
        i += 4;
    }

    if (i + 2 <= length) {
        uint16 val = buffer[i] | (buffer[i+1]<<8);
        st = FlashDrv_ProgramHalfWord(Address + i, val);
        if (st != FLASH_DRV_OK) return st;
        i += 2;
    }

    if (i < length) return FLASH_DRV_ALIGNMENT_ERROR;

    return FLASH_DRV_OK;
}

int FlashDrv_Verify(uint32 Address, const uint8 *buffer, uint8 length)
{
    const uint8 *p = (const uint8*)Address;
    for (uint8 i=0; i<length; i++) {
        if (p[i] != buffer[i]) return -1;
    }
    return 0;
}

/* -------------------------
   Safety
   ------------------------- */
void FlashDrv_PrepareForWrite(void)
{
    __asm volatile ("cpsid i"); /* disable interrupts */
}

void FlashDrv_FinishWrite(void)
{
    __asm volatile ("cpsie i"); /* enable interrupts */
}
