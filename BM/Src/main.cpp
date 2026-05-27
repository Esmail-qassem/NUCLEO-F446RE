/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : BM main                                                */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RCC.hpp"
#include "GPIO.hpp"
#include "UART.hpp"

/* ── Configurations ───────────────────────────────────────────────── */
static RCC_Config_t RCC_Configuration =
{
    RCC_ClockSrc::HSI,
    {RCC_PLLSrc::HSI, 0U, 0U, 0U, 0U},
    RCC_AHBPre::DIV1,
    RCC_APBPre::DIV1,
    RCC_APBPre::DIV1
};

static UART_Config_t Uart_configuration =
{
    115200U,
    UART_Mode::TX,
    UART_Parity::NONE,
    UART_StopBits::BITS_1,
    UART_WordLength::BITS_8,
    UART_Synch::Polling
};

constexpr uint32 VTOR_ADDR = 0xE000ED08UL;
#define VTOR (*reinterpret_cast<volatile uint32*>(VTOR_ADDR))

static void (*Jump_toApplication)(void) = nullptr;
static void (*Jump_toBootLoader)(void)  = nullptr;

/* ── RCC CSR bits ─────────────────────────────────────────────────── */
constexpr uint8 SFT_RSTF  = 28U;
constexpr uint8 POR_RSTF  = 27U;
constexpr uint8 PIN_RSTF  = 26U;
constexpr uint8 IWDG_RSTF = 29U;

static volatile uint32 &RCC_CSR_REG = *reinterpret_cast<volatile uint32*>(0x40023874UL);

/* ── Prototypes ───────────────────────────────────────────────────── */
static void GPIO_PIN_CONFIG(void);
static void APP_init(void);

/* ── main ─────────────────────────────────────────────────────────── */
extern "C" int main(void)
{
    APP_init();
    GPIO_PIN_CONFIG();

    uint8 SwReset    = static_cast<uint8>(GET_BIT(RCC_CSR_REG, SFT_RSTF));
    uint8 PowerReset = static_cast<uint8>(GET_BIT(RCC_CSR_REG, POR_RSTF));
    uint8 PinReset   = static_cast<uint8>(GET_BIT(RCC_CSR_REG, PIN_RSTF));
    uint8 IwdgReset  = static_cast<uint8>(GET_BIT(RCC_CSR_REG, IWDG_RSTF));

    while (1)
    {
        UART::SendSyncBuffer(UART_HardWare::UART2,
                             reinterpret_cast<const uint8*>("\n BM \n"), 7U);

        if (PowerReset || SwReset || IwdgReset)
        {
            VTOR = 0x08008000UL;
            Jump_toApplication = reinterpret_cast<void(*)(void)>(
                *reinterpret_cast<volatile uint32*>(0x08008004UL));
            Jump_toApplication();
        }
        else if (PinReset)
        {
            VTOR = 0x08004000UL;
            Jump_toBootLoader = reinterpret_cast<void(*)(void)>(
                *reinterpret_cast<volatile uint32*>(0x08004004UL));
            Jump_toBootLoader();
        }
    }
}

/* ── APP_init ─────────────────────────────────────────────────────── */
static void APP_init(void)
{
    UART::Init(UART_HardWare::UART2, Uart_configuration, 16000000U);
}

/* ── GPIO_PIN_CONFIG ──────────────────────────────────────────────── */
static void GPIO_PIN_CONFIG(void)
{
    /* UART2 TX */
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P2,
                  GPIO_Mode::AF, GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::A, 2u, static_cast<uint8>(AF7_USART2));
}

/* ── SystemInit ───────────────────────────────────────────────────── */
extern "C" void SystemInit(void)
{
    RCC::Init(RCC_Configuration);
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::GPIOA);
    RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::USART2);
}
