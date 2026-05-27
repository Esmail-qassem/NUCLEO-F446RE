/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : UART                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 USART1_BASE_ADDR = 0x40011000UL;
constexpr uint32 USART2_BASE_ADDR = 0x40004400UL;
constexpr uint32 USART3_BASE_ADDR = 0x40004800UL;

/* ── Register Layout ──────────────────────────────────────────────── */
struct USART_RegDef_t
{
    volatile uint32 SR;
    volatile uint32 DR;
    volatile uint32 BRR;
    volatile uint32 CR1;
    volatile uint32 CR2;
    volatile uint32 CR3;
};

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class UART_Mode : uint8
{
    RX    = 0x04,
    TX    = 0x08,
    TX_RX = 0x0C
};

enum class UART_Parity : uint8
{
    NONE = 0,
    EVEN,
    ODD
};

enum class UART_StopBits : uint8
{
    BITS_1   = 0x00,
    BITS_0_5 = 0x01,
    BITS_2   = 0x02,
    BITS_1_5 = 0x03
};

enum class UART_WordLength : uint8
{
    BITS_8 = 0,
    BITS_9
};

enum class UART_Synch : uint8
{
    Polling   = 0,
    Interrupt = 32
};

enum class UART_HardWare : uint8
{
    UART1 = 0,
    UART2,
    UART3
};

/* ── AF constants ─────────────────────────────────────────────────── */
constexpr uint8 AF7_USART1 = 7u;
constexpr uint8 AF7_USART2 = 7u;
constexpr uint8 AF7_USART3 = 7u;

/* ── IRQ numbers ──────────────────────────────────────────────────── */
constexpr uint8 UART1_IQ_NUM = 37u;
constexpr uint8 UART2_IQ_NUM = 38u;
constexpr uint8 UART3_IQ_NUM = 39u;

/* ── Configuration struct ─────────────────────────────────────────── */
struct UART_Config_t
{
    uint32          BaudRate;
    UART_Mode       Mode;
    UART_Parity     Parity;
    UART_StopBits   StopBits;
    UART_WordLength WordLength;
    UART_Synch      Sync_Mode;
};

/* ── ISR declarations ─────────────────────────────────────────────── */
extern "C" {
    void USART1_IRQHandler(void);
    void USART2_IRQHandler(void);
    void USART3_IRQHandler(void);
}

/* ── UART Driver Class ────────────────────────────────────────────── */
class UART
{
public:
    static void Init           (UART_HardWare hw, const UART_Config_t &cfg, uint32 pclk);
    static void SendSyncBuffer (UART_HardWare hw, const uint8 *buf, uint8 size);
    static void SendNumber     (UART_HardWare hw, sint32 number);
    static void SetCallback1   (void(*fn)(uint8));
    static void SetCallback2   (void(*fn)(uint8));
    static void SetCallback3   (void(*fn)(uint8));

    static USART_RegDef_t* GetReg(UART_HardWare hw);

private:
    static void WriteNumber(UART_HardWare hw, sint32 number);
    UART() = delete;
};
