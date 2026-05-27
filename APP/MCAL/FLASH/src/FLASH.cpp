#include "FLASH.hpp"

static FlashDrv_Status_t WaitForLastOperation(void)
{
    uint32 timeout = FLASH_DRV_TIMEOUT;
    while ((FLASH_SR & FLASH_SR_BSY) && timeout > 0u) timeout--;
    if (timeout == 0u)                  return FlashDrv_Status_t::TIMEOUT_ERROR;
    if (FLASH_SR & FLASH_SR_ERRORS_MASK) return FlashDrv_Status_t::ERROR;
    return FlashDrv_Status_t::OK;
}

static bool IsValidAddr(uint32 addr)
{
    return (addr >= FLASH_BASE_ADDR && addr < FLASH_END_ADDR);
}

void FlashDrv_Unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK) { FLASH_KEYR = FLASH_KEY1; FLASH_KEYR = FLASH_KEY2; }
}
void FlashDrv_Lock(void)      { FLASH_CR |= FLASH_CR_LOCK; }
uint8 FlashDrv_IsLocked(void) { return (FLASH_CR & FLASH_CR_LOCK) ? 1u : 0u; }
void FlashDrv_ClearErrors(void) { FLASH_SR |= FLASH_SR_EOP | FLASH_SR_ERRORS_MASK; }

uint32 FlashDrv_GetSector(uint32 addr)
{
    if (addr < SECTOR1_BASE) return 0u;
    if (addr < SECTOR2_BASE) return 1u;
    if (addr < SECTOR3_BASE) return 2u;
    if (addr < SECTOR4_BASE) return 3u;
    if (addr < SECTOR5_BASE) return 4u;
    if (addr < SECTOR6_BASE) return 5u;
    if (addr < SECTOR7_BASE) return 6u;
    return 7u;
}

uint32 FlashDrv_GetSectorBase(uint32 sec)
{
    static const uint32 bases[] = {
        SECTOR0_BASE, SECTOR1_BASE, SECTOR2_BASE, SECTOR3_BASE,
        SECTOR4_BASE, SECTOR5_BASE, SECTOR6_BASE, SECTOR7_BASE
    };
    return (sec < 8u) ? bases[sec] : 0u;
}

uint32 FlashDrv_GetSectorSize(uint32 sec)
{
    static const uint32 sizes[] = {
        SECTOR0_SIZE, SECTOR1_SIZE, SECTOR2_SIZE, SECTOR3_SIZE,
        SECTOR4_SIZE, SECTOR5_SIZE, SECTOR6_SIZE, SECTOR7_SIZE
    };
    return (sec < 8u) ? sizes[sec] : 0u;
}

FlashDrv_Status_t FlashDrv_EraseSector(uint32 sector)
{
    if (sector > 7u) return FlashDrv_Status_t::INVALID_ADDRESS;
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    auto status = WaitForLastOperation();
    if (status != FlashDrv_Status_t::OK) { FlashDrv_Lock(); return status; }
    FLASH_CR &= ~FLASH_CR_SNB_Msk;
    FLASH_CR |= (sector << FLASH_CR_SNB_Pos) | FLASH_CR_SER | FLASH_CR_STRT;
    status = WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_SER;
    FlashDrv_Lock();
    return status;
}

FlashDrv_Status_t FlashDrv_MassErase(void)
{
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    auto status = WaitForLastOperation();
    if (status != FlashDrv_Status_t::OK) { FlashDrv_Lock(); return status; }
    FLASH_CR |= FLASH_CR_MER | FLASH_CR_STRT;
    status = WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_MER;
    FlashDrv_Lock();
    return status;
}

FlashDrv_Status_t FlashDrv_ProgramWord(uint32 addr, uint32 data)
{
    if (!IsValidAddr(addr))  return FlashDrv_Status_t::INVALID_ADDRESS;
    if (addr & 0x3u)         return FlashDrv_Status_t::ALIGNMENT_ERROR;
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    auto status = WaitForLastOperation();
    if (status != FlashDrv_Status_t::OK) { FlashDrv_Lock(); return status; }
    FLASH_CR = (FLASH_CR & ~FLASH_CR_PSIZE_Msk) |
               (FLASH_PSIZE_WORD << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;
    *reinterpret_cast<volatile uint32*>(addr) = data;
    status = WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_PG;
    FlashDrv_Lock();
    return status;
}

FlashDrv_Status_t FlashDrv_ProgramByte(uint32 addr, uint8 data)
{
    if (!IsValidAddr(addr)) return FlashDrv_Status_t::INVALID_ADDRESS;
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    auto status = WaitForLastOperation();
    if (status != FlashDrv_Status_t::OK) { FlashDrv_Lock(); return status; }
    FLASH_CR = (FLASH_CR & ~FLASH_CR_PSIZE_Msk) |
               (FLASH_PSIZE_BYTE << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;
    *reinterpret_cast<volatile uint8*>(addr) = data;
    status = WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_PG;
    FlashDrv_Lock();
    return status;
}

FlashDrv_Status_t FlashDrv_ProgramHalfWord(uint32 addr, uint16 data)
{
    if (!IsValidAddr(addr)) return FlashDrv_Status_t::INVALID_ADDRESS;
    if (addr & 0x1u)        return FlashDrv_Status_t::ALIGNMENT_ERROR;
    FlashDrv_Unlock();
    FlashDrv_ClearErrors();
    auto status = WaitForLastOperation();
    if (status != FlashDrv_Status_t::OK) { FlashDrv_Lock(); return status; }
    FLASH_CR = (FLASH_CR & ~FLASH_CR_PSIZE_Msk) |
               (FLASH_PSIZE_HALF_WORD << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;
    *reinterpret_cast<volatile uint16*>(addr) = data;
    status = WaitForLastOperation();
    FLASH_CR &= ~FLASH_CR_PG;
    FlashDrv_Lock();
    return status;
}

uint8 FlashDrv_IsErased(uint32 addr, uint32 len)
{
    const uint8 *p = reinterpret_cast<const uint8*>(addr);
    for (uint32 i = 0u; i < len; i++)
        if (p[i] != 0xFFu) return 0u;
    return 1u;
}

FlashDrv_Status_t FlashDrv_ProgramBufferAligned(uint32 addr, const uint8 *buf, uint32 len)
{
    if (!IsValidAddr(addr) || !IsValidAddr(addr + len - 1u))
        return FlashDrv_Status_t::INVALID_ADDRESS;
    if (addr & 0x3u) return FlashDrv_Status_t::ALIGNMENT_ERROR;

    uint32 i = 0u;
    while (i + 4u <= len)
    {
        uint32 word;
        __builtin_memcpy(&word, buf + i, 4u);
        auto s = FlashDrv_ProgramWord(addr + i, word);
        if (s != FlashDrv_Status_t::OK) return s;
        i += 4u;
    }
    while (i < len)
    {
        auto s = FlashDrv_ProgramByte(addr + i, buf[i]);
        if (s != FlashDrv_Status_t::OK) return s;
        i++;
    }
    return FlashDrv_Status_t::OK;
}

FlashDrv_Status_t FlashDrv_ProgramBuffer(uint32 addr, const uint8 *buf, uint32 len)
{
    if (!IsValidAddr(addr) || !IsValidAddr(addr + len - 1u))
        return FlashDrv_Status_t::INVALID_ADDRESS;
    for (uint32 i = 0u; i < len; i++)
    {
        auto s = FlashDrv_ProgramByte(addr + i, buf[i]);
        if (s != FlashDrv_Status_t::OK) return s;
    }
    return FlashDrv_Status_t::OK;
}

FlashDrv_Status_t FlashDrv_EraseRange(uint32 start, uint32 length)
{
    if (!IsValidAddr(start) || !IsValidAddr(start + length - 1u))
        return FlashDrv_Status_t::INVALID_ADDRESS;
    uint32 cur = start;
    while (cur <= start + length - 1u)
    {
        uint32 sec  = FlashDrv_GetSector(cur);
        auto   s    = FlashDrv_EraseSector(sec);
        if (s != FlashDrv_Status_t::OK) return s;
        cur = FlashDrv_GetSectorBase(sec) + FlashDrv_GetSectorSize(sec);
    }
    return FlashDrv_Status_t::OK;
}

int FlashDrv_Verify(uint32 addr, const uint8 *buf, uint32 len)
{
    const uint8 *fp = reinterpret_cast<const uint8*>(addr);
    for (uint32 i = 0u; i < len; i++)
        if (fp[i] != buf[i]) return -1;
    return 0;
}

FlashDrv_Status_t FlashDrv_Read(uint32 addr, uint8 *buf, uint32 len)
{
    if (!IsValidAddr(addr) || !IsValidAddr(addr + len - 1u))
        return FlashDrv_Status_t::INVALID_ADDRESS;
    const uint8 *fp = reinterpret_cast<const uint8*>(addr);
    for (uint32 i = 0u; i < len; i++) buf[i] = fp[i];
    return FlashDrv_Status_t::OK;
}

void FlashDrv_PrepareForWrite(void) { __asm volatile("cpsid i"); }
void FlashDrv_FinishWrite(void)     { __asm volatile("cpsie i"); }

FlashDrv_Status_t FlashDrv_ProgramWordSafe(uint32 addr, uint32 data)
{
    FlashDrv_PrepareForWrite();
    auto s = FlashDrv_ProgramWord(addr, data);
    FlashDrv_FinishWrite();
    return s;
}
