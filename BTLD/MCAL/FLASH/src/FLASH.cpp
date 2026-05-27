/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : FLASH                                                  */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "FLASH.hpp"

/* ── Private helpers ──────────────────────────────────────────────── */
uint8 FlashDrv::IsValidAddress(uint32 addr)
{
    return ((addr >= FLASH_MEM_BASE) && (addr < FLASH_MEM_END)) ? 1u : 0u;
}

FlashDrv_Status FlashDrv::WaitForLastOp(void)
{
    uint32 timeout = FLASH_DRV_TIMEOUT;
    while ((FLASH_REGS->SR & FLASH_SR_BSY) && (timeout > 0u)) timeout--;
    if (timeout == 0u)                          return FlashDrv_Status::TIMEOUT_ERROR;
    if (FLASH_REGS->SR & FLASH_SR_ERRORS_MASK) return FlashDrv_Status::ERROR;
    return FlashDrv_Status::OK;
}

/* ── Unlock / Lock ────────────────────────────────────────────────── */
void FlashDrv::Unlock(void)
{
    if (FLASH_REGS->CR & FLASH_CR_LOCK)
    {
        FLASH_REGS->KEYR = FLASH_KEY1;
        FLASH_REGS->KEYR = FLASH_KEY2;
    }
}
void FlashDrv::Lock(void)       { FLASH_REGS->CR |= FLASH_CR_LOCK; }
uint8 FlashDrv::IsLocked(void)  { return (FLASH_REGS->CR & FLASH_CR_LOCK) ? 1u : 0u; }
void FlashDrv::ClearErrors(void){ FLASH_REGS->SR |= FLASH_SR_EOP | FLASH_SR_ERRORS_MASK; }

/* ── Sector helpers ───────────────────────────────────────────────── */
uint32 FlashDrv::GetSector(uint32 addr)
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

uint32 FlashDrv::GetSectorBase(uint32 sec)
{
    static const uint32 bases[] = {
        SECTOR0_BASE, SECTOR1_BASE, SECTOR2_BASE, SECTOR3_BASE,
        SECTOR4_BASE, SECTOR5_BASE, SECTOR6_BASE, SECTOR7_BASE
    };
    return (sec < 8u) ? bases[sec] : 0u;
}

uint32 FlashDrv::GetSectorSize(uint32 sec)
{
    static const uint32 sizes[] = {
        SECTOR0_SIZE, SECTOR1_SIZE, SECTOR2_SIZE, SECTOR3_SIZE,
        SECTOR4_SIZE, SECTOR5_SIZE, SECTOR6_SIZE, SECTOR7_SIZE
    };
    return (sec < 8u) ? sizes[sec] : 0u;
}

/* ── EraseSector ──────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::EraseSector(uint32 sectorNum)
{
    if (sectorNum > 7u) return FlashDrv_Status::INVALID_ADDRESS;
    Unlock();
    ClearErrors();
    auto s = WaitForLastOp();
    if (s != FlashDrv_Status::OK) { Lock(); return s; }
    FLASH_REGS->CR &= ~FLASH_CR_SNB_Msk;
    FLASH_REGS->CR |= (sectorNum << FLASH_CR_SNB_Pos) | FLASH_CR_SER | FLASH_CR_STRT;
    s = WaitForLastOp();
    FLASH_REGS->CR &= ~FLASH_CR_SER;
    Lock();
    return s;
}

/* ── MassErase ────────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::MassErase(void)
{
    Unlock();
    ClearErrors();
    auto s = WaitForLastOp();
    if (s != FlashDrv_Status::OK) { Lock(); return s; }
    FLASH_REGS->CR |= FLASH_CR_MER | FLASH_CR_STRT;
    s = WaitForLastOp();
    FLASH_REGS->CR &= ~FLASH_CR_MER;
    Lock();
    return s;
}

/* ── ProgramWord ──────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::ProgramWord(uint32 addr, uint32 data)
{
    if (!IsValidAddress(addr))  return FlashDrv_Status::INVALID_ADDRESS;
    if (addr & 0x3u)            return FlashDrv_Status::ALIGNMENT_ERROR;
    Unlock();
    ClearErrors();
    auto s = WaitForLastOp();
    if (s != FlashDrv_Status::OK) { Lock(); return s; }
    FLASH_REGS->CR = (FLASH_REGS->CR & ~FLASH_CR_PSIZE_Msk) |
                     (static_cast<uint32>(FLASH_PSIZE_WORD) << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;
    *reinterpret_cast<volatile uint32*>(addr) = data;
    s = WaitForLastOp();
    FLASH_REGS->CR &= ~FLASH_CR_PG;
    Lock();
    return s;
}

/* ── ProgramByte ──────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::ProgramByte(uint32 addr, uint8 data)
{
    if (!IsValidAddress(addr)) return FlashDrv_Status::INVALID_ADDRESS;
    Unlock();
    ClearErrors();
    auto s = WaitForLastOp();
    if (s != FlashDrv_Status::OK) { Lock(); return s; }
    FLASH_REGS->CR = (FLASH_REGS->CR & ~FLASH_CR_PSIZE_Msk) |
                     (static_cast<uint32>(FLASH_PSIZE_BYTE) << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;
    *reinterpret_cast<volatile uint8*>(addr) = data;
    s = WaitForLastOp();
    FLASH_REGS->CR &= ~FLASH_CR_PG;
    Lock();
    return s;
}

/* ── ProgramHalfWord ──────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::ProgramHalfWord(uint32 addr, uint16 data)
{
    if (!IsValidAddress(addr)) return FlashDrv_Status::INVALID_ADDRESS;
    if (addr & 0x1u)           return FlashDrv_Status::ALIGNMENT_ERROR;
    Unlock();
    ClearErrors();
    auto s = WaitForLastOp();
    if (s != FlashDrv_Status::OK) { Lock(); return s; }
    FLASH_REGS->CR = (FLASH_REGS->CR & ~FLASH_CR_PSIZE_Msk) |
                     (static_cast<uint32>(FLASH_PSIZE_HALF_WORD) << FLASH_CR_PSIZE_Pos) | FLASH_CR_PG;
    *reinterpret_cast<volatile uint16*>(addr) = data;
    s = WaitForLastOp();
    FLASH_REGS->CR &= ~FLASH_CR_PG;
    Lock();
    return s;
}

/* ── IsErased ─────────────────────────────────────────────────────── */
uint8 FlashDrv::IsErased(uint32 addr, uint32 len)
{
    const uint8 *p = reinterpret_cast<const uint8*>(addr);
    for (uint32 i = 0u; i < len; i++)
        if (p[i] != 0xFFu) return 0u;
    return 1u;
}

/* ── ProgramBufferAligned ─────────────────────────────────────────── */
FlashDrv_Status FlashDrv::ProgramBufferAligned(uint32 addr, const uint8 *buf, uint32 len)
{
    if (!IsValidAddress(addr) || !IsValidAddress(addr + len - 1u))
        return FlashDrv_Status::INVALID_ADDRESS;
    if (addr & 0x3u) return FlashDrv_Status::ALIGNMENT_ERROR;
    uint32 i = 0u;
    while (i + 4u <= len)
    {
        uint32 word;
        __builtin_memcpy(&word, buf + i, 4u);
        auto s = ProgramWord(addr + i, word);
        if (s != FlashDrv_Status::OK) return s;
        i += 4u;
    }
    while (i < len)
    {
        auto s = ProgramByte(addr + i, buf[i]);
        if (s != FlashDrv_Status::OK) return s;
        i++;
    }
    return FlashDrv_Status::OK;
}

/* ── ProgramBuffer ────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::ProgramBuffer(uint32 addr, const uint8 *buf, uint32 len)
{
    if (!IsValidAddress(addr) || !IsValidAddress(addr + len - 1u))
        return FlashDrv_Status::INVALID_ADDRESS;
    for (uint32 i = 0u; i < len; i++)
    {
        auto s = ProgramByte(addr + i, buf[i]);
        if (s != FlashDrv_Status::OK) return s;
    }
    return FlashDrv_Status::OK;
}

/* ── EraseRange ───────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::EraseRange(uint32 start, uint32 length)
{
    if (!IsValidAddress(start) || !IsValidAddress(start + length - 1u))
        return FlashDrv_Status::INVALID_ADDRESS;
    uint32 cur = start;
    while (cur <= start + length - 1u)
    {
        uint32 sec = GetSector(cur);
        auto   s   = EraseSector(sec);
        if (s != FlashDrv_Status::OK) return s;
        cur = GetSectorBase(sec) + GetSectorSize(sec);
    }
    return FlashDrv_Status::OK;
}

/* ── Verify ───────────────────────────────────────────────────────── */
int FlashDrv::Verify(uint32 addr, const uint8 *buf, uint32 len)
{
    const uint8 *fp = reinterpret_cast<const uint8*>(addr);
    for (uint32 i = 0u; i < len; i++)
        if (fp[i] != buf[i]) return -1;
    return 0;
}

/* ── Read ─────────────────────────────────────────────────────────── */
FlashDrv_Status FlashDrv::Read(uint32 addr, uint8 *buf, uint32 len)
{
    if (!IsValidAddress(addr) || !IsValidAddress(addr + len - 1u))
        return FlashDrv_Status::INVALID_ADDRESS;
    const uint8 *fp = reinterpret_cast<const uint8*>(addr);
    for (uint32 i = 0u; i < len; i++) buf[i] = fp[i];
    return FlashDrv_Status::OK;
}

void FlashDrv::PrepareForWrite(void) { __asm volatile ("cpsid i"); }
void FlashDrv::FinishWrite(void)     { __asm volatile ("cpsie i"); }

FlashDrv_Status FlashDrv::ProgramWordSafe(uint32 addr, uint32 data)
{
    PrepareForWrite();
    auto s = ProgramWord(addr, data);
    FinishWrite();
    return s;
}
