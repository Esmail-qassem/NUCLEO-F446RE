/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : BTLD main                                              */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "STD_TYPES.h"
#include "SysTick.hpp"
#include "RCC.hpp"
#include "GPIO.hpp"
#include "NVIC.hpp"
#include "UART.hpp"
#include "Parse.hpp"

constexpr uint32 VTOR_ADDR = 0xE000ED08UL;
#define VTOR (*reinterpret_cast<volatile uint32*>(VTOR_ADDR))

static void (*Pt2Func_App)(void) = nullptr;

/* ── RCC/UART configs ─────────────────────────────────────────────── */
static RCC_Config_t RCC_Configuration =
{
    RCC_ClockSrc::HSI,
    {RCC_PLLSrc::HSI, 0U, 0U, 0U, 0U},
    RCC_AHBPre::DIV1,
    RCC_APBPre::DIV1,
    RCC_APBPre::DIV1
};

static UART_Config_t Uart1_configuration =
{
    115200U,
    UART_Mode::TX_RX,
    UART_Parity::NONE,
    UART_StopBits::BITS_1,
    UART_WordLength::BITS_8,
    UART_Synch::Interrupt
};

static UART_Config_t Uart2_configuration =
{
    115200U,
    UART_Mode::TX_RX,
    UART_Parity::NONE,
    UART_StopBits::BITS_1,
    UART_WordLength::BITS_8,
    UART_Synch::Interrupt
};

/* ── ms_ticks (extern used by Parse.cpp) ─────────────────────────── */
uint32 ms_ticks = 0U;

/* ── Prototypes ───────────────────────────────────────────────────── */
static void GPIO_PIN_CONFIG(void);
static void APP_init(void);
static void CallBackFunctions(void);

/* ── SysTick callback ─────────────────────────────────────────────── */
static void systick_handler(void)
{
    ms_ticks++;
    if (ms_ticks % 1000U == 0U)
        UART::SendSyncBuffer(UART_HardWare::UART2,
                             reinterpret_cast<const uint8*>("."), 1U);
}

/* ── main ─────────────────────────────────────────────────────────── */
extern "C" int main(void)
{
    GPIO_PIN_CONFIG();
    APP_init();

    UART::SendSyncBuffer(UART_HardWare::UART2,
                         reinterpret_cast<const uint8*>("\nBTLD Session\n"), 14U);
    UART::SendSyncBuffer(UART_HardWare::UART2,
                         reinterpret_cast<const uint8*>("BTLD will jump to application in 20 seconds if no data is received\n"), 67U);
    UART::SendSyncBuffer(UART_HardWare::UART2,
                         reinterpret_cast<const uint8*>("Waiting for data"), 16U);

    SysTick::Init();
    NVIC::EnableInterrupt(UART2_IQ_NUM);
    SysTick::SetIntervalPeriodic(TICKS_PER_MS, &systick_handler);
    CRC32_Init();
    CallBackFunctions();

    while (1)
    {
        BootLoader_MainFunction();
    }
}

/* ── APP_init ─────────────────────────────────────────────────────── */
static void APP_init(void)
{
    UART::Init(UART_HardWare::UART2, Uart2_configuration, 16000000U);
    UART::Init(UART_HardWare::UART1, Uart1_configuration, 16000000U);
}

/* ── GPIO_PIN_CONFIG ──────────────────────────────────────────────── */
static void GPIO_PIN_CONFIG(void)
{
    /* UART2 TX (PA2) / RX (PA3) */
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P2, GPIO_Mode::AF, GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::A, 2u, static_cast<uint8>(AF7_USART2));
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P3, GPIO_Mode::AF, GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::A, 3u, static_cast<uint8>(AF7_USART2));

    /* UART1 TX (PA9) / RX (PA10) */
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P9,  GPIO_Mode::AF, GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P10, GPIO_Mode::AF, GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::A, 9u,  static_cast<uint8>(AF7_USART1));
    GPIO::SetAF(GPIO_Port::A, 10u, static_cast<uint8>(AF7_USART1));
}

/* ── SystemInit ───────────────────────────────────────────────────── */
extern "C" void SystemInit(void)
{
    RCC::Init(RCC_Configuration);
    /* Clear all reset flags */
    volatile uint32 &rcc_csr = *reinterpret_cast<volatile uint32*>(0x40023874UL);
    rcc_csr |= (1U << 24U);
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::GPIOA);
    RCC::EnableClock(RCC_Bus::APB2, RCC_Peripheral::USART1);
    RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::USART2);
}

/* ── CallBackFunctions ────────────────────────────────────────────── */
static void CallBackFunctions(void)
{
    UART::SetCallback2(BootLoader_Handler);
}
