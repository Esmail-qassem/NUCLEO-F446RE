/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : Basic Timer (TIM6 / TIM7)                             */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 TIMER6_BASE_ADDR = 0x40001000UL;
constexpr uint32 TIMER7_BASE_ADDR = 0x40001400UL;

/* ── Register layout (basic timer subset) ────────────────────────── */
struct BasicTimer_RegDef_t
{
    volatile uint16 CR1;  volatile uint16 _r0; /* 0x00 */
    volatile uint16 CR2;  volatile uint16 _r1; /* 0x04 */
    uint32 _r2[2];                              /* 0x08, 0x0C gap */
    volatile uint16 DIER; volatile uint16 _r3; /* 0x0C */
    volatile uint16 SR;   volatile uint16 _r4; /* 0x10 */
    uint32 _r5[4];                              /* 0x14–0x20 */
    volatile uint16 CNT;  volatile uint16 _r6; /* 0x24 */
    volatile uint16 PSC;  volatile uint16 _r7; /* 0x28 */
    volatile uint16 ARR;  volatile uint16 _r8; /* 0x2C */
};

/* ── Bit masks ────────────────────────────────────────────────────── */
constexpr uint16 BTIM_DIER_UIE = (1u << 0u);
constexpr uint16 BTIM_CR1_CEN  = (1u << 0u);
constexpr uint16 BTIM_SR_UIF   = (1u << 0u);

/* ── IRQ numbers ──────────────────────────────────────────────────── */
constexpr uint8 TIM6_IRQn = 54u;
constexpr uint8 TIM7_IRQn = 55u;

/* ── Default prescaler/reload ─────────────────────────────────────── */
constexpr uint16 TIM6_PRESCALER = 8u - 1u;
constexpr uint16 TIM6_RELOAD    = 2000u - 1u;
constexpr uint16 TIM7_PRESCALER = 8u - 1u;
constexpr uint16 TIM7_RELOAD    = 2000u - 1u;

/* ── Timer selection ──────────────────────────────────────────────── */
enum class BasicTimer_ID : uint8
{
    TIM6 = 6u,
    TIM7 = 7u
};

using TimerCallback_t = void(*)(void);

/* ── ISR declarations ─────────────────────────────────────────────── */
extern "C" {
    void TIM6_DAC_IRQHandler(void);
    void TIM7_IRQHandler(void);
}

/* ── BasicTimer Driver Class ──────────────────────────────────────── */
class BasicTimer
{
public:
    static void Init        (BasicTimer_ID timer);
    static void Start       (BasicTimer_ID timer);
    static void Stop        (BasicTimer_ID timer);
    static void SetCallback (BasicTimer_ID timer, TimerCallback_t cb);

private:
    static BasicTimer_RegDef_t* GetReg(BasicTimer_ID timer);
    BasicTimer() = delete;
};
