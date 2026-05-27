#include "UART.hpp"
/* Legacy C-style compatibility shim — new code uses UART.hpp directly */
/* Old enum values remapped for callers that haven't been migrated yet  */
#ifndef USART_H_
#define USART_H_

/* Hardware enum shim */
#define UART1 UART_HardWare::UART1
#define UART2 UART_HardWare::UART2
#define UART3 UART_HardWare::UART3

/* Mode shim */
#define UART_MODE_TX     UART_Mode::TX
#define UART_MODE_RX     UART_Mode::RX
#define UART_MODE_TX_RX  UART_Mode::TX_RX

/* Parity shim */
#define UART_PARITY_NONE  UART_Parity::NONE
#define UART_PARITY_EVEN  UART_Parity::EVEN
#define UART_PARITY_ODD   UART_Parity::ODD

/* StopBits shim */
#define UART_STOPBITS_1   UART_StopBits::BITS_1
#define UART_STOPBITS_0_5 UART_StopBits::BITS_0_5
#define UART_STOPBITS_2   UART_StopBits::BITS_2
#define UART_STOPBITS_1_5 UART_StopBits::BITS_1_5

/* WordLength shim */
#define UART_WORDLEN_8B  UART_WordLength::BITS_8
#define UART_WORDLEN_9B  UART_WordLength::BITS_9

/* Synch shim */
#define Polling   UART_Synch::Polling
#define Interrupt UART_Synch::Interrupt

/* Function shims */
#define UART_Init(hw,cfg,pclk)           UART::Init(hw, *(cfg), pclk)
#define UART_SendSyncBuffer(hw,buf,sz)   UART::SendSyncBuffer(hw, buf, sz)
#define UART_voidSendNumber(hw,n)        UART::SendNumber(hw, n)
#define UART1_CALLBACK(fn)               UART::SetCallback1(fn)
#define UART2_CALLBACK(fn)               UART::SetCallback2(fn)
#define UART3_CALLBACK(fn)               UART::SetCallback3(fn)

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


typedef enum {
    Polling = 0,
    Interrupt =32
} UART_Synch_t;
/* ---------------- Configuration Struct ---------------- */
typedef struct {
    uint32 BaudRate;
    UART_Mode_t Mode;
    UART_Parity_t Parity;
    UART_StopBits_t StopBits;
    UART_WordLength_t WordLength;
    UART_Synch_t Sync_Mode;
} UART_Config_t;
#define UART1_IQ_NUM  37
#define UART2_IQ_NUM  38
#define UART3_IQ_NUM  39

/* ---------------- API ---------------- */
void UART_Init(UART_HardWare_t base, const UART_Config_t *cfg, uint32 pclk);
void UART_SendSyncBuffer(UART_HardWare_t base, const uint8 *buf, uint8 size);
void UART_voidSendNumber(UART_HardWare_t HardWare_Unit,sint32 Copy_sint32Number);
void UART2_CALLBACK(void(*p2function)(uint8));
void UART3_CALLBACK(void(*p2function)(uint8));
void UART1_CALLBACK(void(*p2function)(uint8));

#endif
