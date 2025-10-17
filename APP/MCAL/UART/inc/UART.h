#ifndef USART_H_
#define USART_H_

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ---------------- Base Addresses ---------------- */
#define USART1_BASE   0x40011000UL
#define USART2_BASE   0x40004400UL
#define USART3_BASE   0x40004800UL

/* ---------------- Register Definitions ---------------- */
#define USART_SR(base)     (*(volatile uint32*)((base) + 0x00))
#define USART_DR(base)     (*(volatile uint32*)((base) + 0x04))
#define USART_BRR(base)    (*(volatile uint32*)((base) + 0x08))
#define USART_CR1(base)    (*(volatile uint32*)((base) + 0x0C))
#define USART_CR2(base)    (*(volatile uint32*)((base) + 0x10))
#define USART_CR3(base)    (*(volatile uint32*)((base) + 0x14))

/* ---------------- UART Modes ---------------- */
typedef enum {
    UART_MODE_TX       = 0x08,
    UART_MODE_RX       = 0x04,
    UART_MODE_TX_RX    = 0x0C
} UART_Mode_t;


typedef enum {
	UART1,
	UART2,
	UART3,
} UART_HardWare_t;
/* Alternate Function mappings (AF numbers) */
#define AF7_USART1   7
#define AF7_USART2   7
#define AF7_USART3   7
#define AF8_UART4    8
#define AF8_UART5    8
#define AF8_USART6   8


/* ---------------- Parity ---------------- */
typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} UART_Parity_t;

/* ---------------- Stop Bits ---------------- */
typedef enum {
    UART_STOPBITS_1 = 0x00,
    UART_STOPBITS_0_5 = 0x01,
    UART_STOPBITS_2 = 0x02,
    UART_STOPBITS_1_5 = 0x03
} UART_StopBits_t;

/* ---------------- Word Length ---------------- */
typedef enum {
    UART_WORDLEN_8B = 0,
    UART_WORDLEN_9B
} UART_WordLength_t;

/* ---------------- Configuration Struct ---------------- */
typedef struct {
    uint32 BaudRate;
    UART_Mode_t Mode;
    UART_Parity_t Parity;
    UART_StopBits_t StopBits;
    UART_WordLength_t WordLength;
} UART_Config_t;

/* ---------------- API ---------------- */
void UART_Init(UART_HardWare_t base, const UART_Config_t *cfg, uint32 pclk);
void UART_SendByte(UART_HardWare_t base, uint8 data);
void UART_SendString(UART_HardWare_t base, const char *str);
uint8 UART_ReceiveByte(UART_HardWare_t base);
uint8 UART_ReceiveByte_Timeout(UART_HardWare_t base, uint32 timeout);

#endif
