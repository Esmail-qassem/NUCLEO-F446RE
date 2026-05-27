/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : UART                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "UART.hpp"

static uint8 rx_test;
static void (*uart1_funcPtr)(uint8) = nullptr;
static void (*uart2_funcPtr)(uint8) = nullptr;
static void (*uart3_funcPtr)(uint8) = nullptr;

USART_RegDef_t* UART::GetReg(UART_HardWare hw)
{
    switch (hw)
    {
        case UART_HardWare::UART1: return reinterpret_cast<USART_RegDef_t*>(USART1_BASE_ADDR);
        case UART_HardWare::UART2: return reinterpret_cast<USART_RegDef_t*>(USART2_BASE_ADDR);
        case UART_HardWare::UART3: return reinterpret_cast<USART_RegDef_t*>(USART3_BASE_ADDR);
        default: return nullptr;
    }
}

void UART::WriteNumber(UART_HardWare hw, sint32 num)
{
    uint8 buf[10];
    uint8 cnt = 0;
    while (num > 0)
    {
        buf[cnt++] = static_cast<uint8>((num % 10) + '0');
        num /= 10;
    }
    for (uint8 i = cnt; i > 0u; i--)
        UART::SendSyncBuffer(hw, buf + i - 1, 1u);
}

void UART::Init(UART_HardWare hw, const UART_Config_t &cfg, uint32 pclk)
{
    USART_RegDef_t *reg = GetReg(hw);
    if (!reg) return;
    CLEAR_BIT(reg->CR1, 13);
    uint32 usartdiv = (pclk + (cfg.BaudRate / 2U)) / cfg.BaudRate;
    reg->BRR = usartdiv;
    if (cfg.WordLength == UART_WordLength::BITS_9) SET_BIT(reg->CR1, 12);
    else CLEAR_BIT(reg->CR1, 12);
    if (cfg.Parity == UART_Parity::NONE) {
        CLEAR_BIT(reg->CR1, 10);
    } else {
        SET_BIT(reg->CR1, 10);
        if (cfg.Parity == UART_Parity::ODD) SET_BIT(reg->CR1, 9);
        else CLEAR_BIT(reg->CR1, 9);
    }
    reg->CR2 &= ~(0x3u << 12u);
    reg->CR2 |=  (static_cast<uint32>(cfg.StopBits) << 12u);
    reg->CR1 |= static_cast<uint32>(cfg.Sync_Mode);
    reg->CR1 |= static_cast<uint32>(cfg.Mode);
    SET_BIT(reg->CR1, 13);
}

void UART::SendSyncBuffer(UART_HardWare hw, const uint8 *buf, uint8 size)
{
    USART_RegDef_t *reg = GetReg(hw);
    if (!reg || !buf) return;
    for (uint8 i = 0u; i < size; i++)
    {
        while (!GET_BIT(reg->SR, 7)) {}
        reg->DR = buf[i];
    }
}

void UART::SendNumber(UART_HardWare hw, sint32 number)
{
    if (number < 0)
    {
        uint8 minus = '-';
        SendSyncBuffer(hw, &minus, 1u);
        number = -number;
    }
    if (number == 0)
    {
        uint8 zero = '0';
        SendSyncBuffer(hw, &zero, 1u);
        return;
    }
    WriteNumber(hw, number);
}

void UART::SetCallback1(void(*fn)(uint8)) { if (fn) uart1_funcPtr = fn; }
void UART::SetCallback2(void(*fn)(uint8)) { if (fn) uart2_funcPtr = fn; }
void UART::SetCallback3(void(*fn)(uint8)) { if (fn) uart3_funcPtr = fn; }

static void UART_HandleIRQ(UART_HardWare hw, void(*cb)(uint8))
{
    uint32 base;
    switch (hw)
    {
        case UART_HardWare::UART1: base = USART1_BASE_ADDR; break;
        case UART_HardWare::UART2: base = USART2_BASE_ADDR; break;
        default:                   base = USART3_BASE_ADDR; break;
    }
    volatile uint32 *SR_ptr = reinterpret_cast<volatile uint32*>(base + 0x00u);
    volatile uint32 *DR_ptr = reinterpret_cast<volatile uint32*>(base + 0x04u);
    uint32 status = *SR_ptr;
    if (status & (1u << 5u))
    {
        rx_test = static_cast<uint8>(*DR_ptr);
        if (cb) cb(rx_test);
    }
    if (status & (1u << 3u)) { volatile uint32 tmp = *SR_ptr; tmp = *DR_ptr; (void)tmp; }
    if (status & (1u << 2u)) { volatile uint32 tmp = *SR_ptr; (void)tmp; }
    if (status & (1u << 1u)) { volatile uint32 tmp = *SR_ptr; (void)tmp; }
}

extern "C" void USART1_IRQHandler(void) { UART_HandleIRQ(UART_HardWare::UART1, uart1_funcPtr); }
extern "C" void USART2_IRQHandler(void) { UART_HandleIRQ(UART_HardWare::UART2, uart2_funcPtr); }
extern "C" void USART3_IRQHandler(void) { UART_HandleIRQ(UART_HardWare::UART3, uart3_funcPtr); }
