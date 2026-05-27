/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : GP_TIMER                                               */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Base addresses ───────────────────────────────────────────────── */
constexpr uint32 TIMER2_BASE_ADDR = 0x40000000UL;
constexpr uint32 TIMER3_BASE_ADDR = 0x40000400UL;
constexpr uint32 TIMER4_BASE_ADDR = 0x40000800UL;
constexpr uint32 TIMER5_BASE_ADDR = 0x40000C00UL;

/* ── Register accessors ───────────────────────────────────────────── */
#define TIM_CR1(base)   (*reinterpret_cast<volatile uint16*>((base) + 0x00u))
#define TIM_CR2(base)   (*reinterpret_cast<volatile uint16*>((base) + 0x04u))
#define TIM_SMCR(base)  (*reinterpret_cast<volatile uint16*>((base) + 0x08u))
#define TIM_DIER(base)  (*reinterpret_cast<volatile uint16*>((base) + 0x0Cu))
#define TIM_SR(base)    (*reinterpret_cast<volatile uint16*>((base) + 0x10u))
#define TIM_CNT(base)   (*reinterpret_cast<volatile uint16*>((base) + 0x24u))
#define TIM_PSC(base)   (*reinterpret_cast<volatile uint16*>((base) + 0x28u))
#define TIM_ARR(base)   (*reinterpret_cast<volatile uint16*>((base) + 0x2Cu))
#define TIM_CCMR1(base) (*reinterpret_cast<volatile uint32*>((base) + 0x18u))
#define TIM_CCER(base)  (*reinterpret_cast<volatile uint32*>((base) + 0x20u))
#define TIM_CCR1(base)  (*reinterpret_cast<volatile uint32*>((base) + 0x34u))
#define TIM_CCR2(base)  (*reinterpret_cast<volatile uint32*>((base) + 0x38u))
#define TIM_CCMR2(base) (*reinterpret_cast<volatile uint32*>((base) + 0x1Cu))
#define TIM_CCR3(base)  (*reinterpret_cast<volatile uint32*>((base) + 0x3Cu))
#define TIM_CCR4(base)  (*reinterpret_cast<volatile uint32*>((base) + 0x40u))

/* ── Bit definitions ──────────────────────────────────────────────── */
constexpr uint16 TIM_DIER_UIE = (1U << 0U);
constexpr uint16 TIM_CR1_CEN  = (1U << 0U);
constexpr uint16 TIM_SR_UIF   = (1U << 0U);
constexpr uint16 TIM_SR_TIF   = (1U << 6U);

/* ── IRQ numbers ──────────────────────────────────────────────────── */
constexpr uint8 TIM2_IRQn = 28U;
constexpr uint8 TIM3_IRQn = 29U;
constexpr uint8 TIM4_IRQn = 30U;
constexpr uint8 TIM5_IRQn = 50U;

/* ── Default prescaler / reload values ───────────────────────────── */
constexpr uint16 TIM2_PRESCALER_VAL = 8U - 1U;
constexpr uint16 TIM2_RELOAD_VAL    = 2000U - 1U;
constexpr uint16 TIM3_PRESCALER_VAL = 8U - 1U;
constexpr uint16 TIM3_RELOAD_VAL    = 2000U - 1U;
constexpr uint16 TIM4_PRESCALER_VAL = 8U - 1U;
constexpr uint16 TIM4_RELOAD_VAL    = 2000U - 1U;
constexpr uint16 TIM5_PRESCALER_VAL = 8U - 1U;
constexpr uint16 TIM5_RELOAD_VAL    = 2000U - 1U;

/* ── Timer selection enum ─────────────────────────────────────────── */
enum class Timer_t : uint8
{
    TIMER2 = 2,
    TIMER3 = 3,
    TIMER4 = 4,
    TIMER5 = 5
};

/* ── Callback type ────────────────────────────────────────────────── */
using TimerCallback_t = void(*)(void);

/* ── IRQ handlers ─────────────────────────────────────────────────── */
extern "C" {
    void TIM2_IRQHandler(void);
    void TIM3_IRQHandler(void);
    void TIM4_IRQHandler(void);
    void TIM5_IRQHandler(void);
}

/* ── GPTimer Driver Class ─────────────────────────────────────────── */
class GPTimer
{
public:
    static void Init        (Timer_t timer);
    static void Start       (Timer_t timer);
    static void Stop        (Timer_t timer);
    static void SetCallback (Timer_t timer, TimerCallback_t cb);
    static void PWM_Init    (Timer_t timer);
    static void PWM_SetDuty (Timer_t timer, uint8 channel, uint8 duty);

private:
    static uint32 GetBase(Timer_t timer);
    GPTimer() = delete;
};
