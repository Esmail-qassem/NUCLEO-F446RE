/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : NVIC                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "NVIC.hpp"

/* priority grouping pre-build config (same as old config.h default) */
static constexpr uint8 PRIORITY_CONFIG = static_cast<uint8>(NVIC_PriorityGroup::G4_S4);

/* ── Register helpers ─────────────────────────────────────────────── */
static inline volatile uint32& NVIC_ISER(uint8 bank)
{
    return *reinterpret_cast<volatile uint32*>(NVIC_BASE_ADDR + static_cast<uint32>(bank) * 4u);
}
static inline volatile uint32& NVIC_ICER(uint8 bank)
{
    return *reinterpret_cast<volatile uint32*>(NVIC_BASE_ADDR + 0x80u + static_cast<uint32>(bank) * 4u);
}
static inline volatile uint32& NVIC_ISPR(uint8 bank)
{
    return *reinterpret_cast<volatile uint32*>(NVIC_BASE_ADDR + 0x100u + static_cast<uint32>(bank) * 4u);
}
static inline volatile uint32& NVIC_ICPR(uint8 bank)
{
    return *reinterpret_cast<volatile uint32*>(NVIC_BASE_ADDR + 0x180u + static_cast<uint32>(bank) * 4u);
}
static inline volatile uint32& NVIC_IABR(uint8 bank)
{
    return *reinterpret_cast<volatile uint32*>(NVIC_BASE_ADDR + 0x200u + static_cast<uint32>(bank) * 4u);
}
static inline volatile uint8& NVIC_IPR(uint8 irq)
{
    return *reinterpret_cast<volatile uint8*>(NVIC_BASE_ADDR + 0x300u + static_cast<uint32>(irq));
}

/* ── EnableInterrupt ──────────────────────────────────────────────── */
NVIC_Status NVIC::EnableInterrupt(uint8 irqNum)
{
    if (irqNum < 32u)
    {
        NVIC_ISER(0) = (1u << irqNum);
        return NVIC_Status::OK;
    }
    else if (irqNum < 64u)
    {
        NVIC_ISER(1) = (1u << (irqNum - 32u));
        return NVIC_Status::OK;
    }
    return NVIC_Status::NOK;
}

/* ── DisableInterrupt ─────────────────────────────────────────────── */
NVIC_Status NVIC::DisableInterrupt(uint8 irqNum)
{
    if (irqNum < 32u)
    {
        NVIC_ICER(0) = (1u << irqNum);
        return NVIC_Status::OK;
    }
    else if (irqNum < 64u)
    {
        NVIC_ICER(1) = (1u << (irqNum - 32u));
        return NVIC_Status::OK;
    }
    return NVIC_Status::NOK;
}

/* ── SetPendingFlag ───────────────────────────────────────────────── */
NVIC_Status NVIC::SetPendingFlag(uint8 irqNum)
{
    if (irqNum < 32u)
    {
        NVIC_ISPR(0) = (1u << irqNum);
        return NVIC_Status::OK;
    }
    else if (irqNum < 64u)
    {
        NVIC_ISPR(1) = (1u << (irqNum - 32u));
        return NVIC_Status::OK;
    }
    return NVIC_Status::NOK;
}

/* ── ClearPendingFlag ─────────────────────────────────────────────── */
NVIC_Status NVIC::ClearPendingFlag(uint8 irqNum)
{
    if (irqNum < 32u)
    {
        NVIC_ICPR(0) = (1u << irqNum);
        return NVIC_Status::OK;
    }
    else if (irqNum < 64u)
    {
        NVIC_ICPR(1) = (1u << (irqNum - 32u));
        return NVIC_Status::OK;
    }
    return NVIC_Status::NOK;
}

/* ── GetActiveFlag ────────────────────────────────────────────────── */
NVIC_Status NVIC::GetActiveFlag(uint8 irqNum, uint8 &out)
{
    if (irqNum < 32u)
    {
        out = static_cast<uint8>(GET_BIT(NVIC_IABR(0), irqNum));
        return NVIC_Status::OK;
    }
    else if (irqNum < 64u)
    {
        out = static_cast<uint8>(GET_BIT(NVIC_IABR(1), irqNum - 32u));
        return NVIC_Status::OK;
    }
    return NVIC_Status::NOK;
}

/* ── SetPriority ──────────────────────────────────────────────────── */
void NVIC::SetPriority(sint8 irqId, uint8 group, uint8 sub)
{
    /* Write priority group to AIRCR */
    volatile uint32 &aircr = *reinterpret_cast<volatile uint32*>(SCB_AIRCR_ADDR);
    aircr = 0x05FA0000UL | (static_cast<uint32>(PRIORITY_CONFIG) << 8u);

    uint8 localVar = PRIORITY_CONFIG - 3u;

    if (irqId >= 0)
    {
        NVIC_IPR(static_cast<uint8>(irqId)) =
            static_cast<uint8>((sub | (group << localVar)) << 4u);
    }
}
