#include "UART.h"
#include "UART_private.h"

/*------------------------------------------------------------------
 *  RX callback pointers — one per hardware unit
 *------------------------------------------------------------------*/
static void (*uart1_funcPtr)(uint8) = NULL;
static void (*uart2_funcPtr)(uint8) = NULL;
static void (*uart3_funcPtr)(uint8) = NULL;

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static uint32 Get_BASE_ADD(UART_HardWare_t base);
static void   Write_UART_Number(UART_HardWare_t HardWare_Unit, sint32 Copy_sint32Number);
static void   UART_IrqCommon(uint32 Add, void (*callback)(uint8));

/*==================================================================
 *  Get_BASE_ADD — map logical UART id to peripheral base address
 *================================================================*/
static uint32 Get_BASE_ADD(UART_HardWare_t base)
{
    uint32 x = 0U;

    switch (base)
    {
        case UART1:  x = USART1_BASE;  break;
        case UART2:  x = USART2_BASE;  break;
        case UART3:  x = USART3_BASE;  break;
        default:     /* invalid id */  break;
    }
    return x;
}

/*==================================================================
 *  Write_UART_Number — emit a positive decimal number, MSB first
 *================================================================*/
static void Write_UART_Number(UART_HardWare_t HardWare_Unit, sint32 Copy_sint32Number)
{
    uint8 NUM[UART_NUM_BUF_LEN];
    uint8 Local_uint8Counter = 0U;
    uint8 i;

    while (Copy_sint32Number > 0)
    {
        NUM[Local_uint8Counter] = (uint8)((Copy_sint32Number % 10) + (sint32)'0');
        Local_uint8Counter++;
        Copy_sint32Number /= 10;
    }

    /* Digits were stored LSB-first; send in reverse order. */
    for (i = Local_uint8Counter; i > 0U; i--)
    {
        UART_SendSyncBuffer(HardWare_Unit, &NUM[i - 1U], 1U);
    }
}

/*==================================================================
 *  UART_Init
 *================================================================*/
void UART_Init(UART_HardWare_t base, const UART_Config_t *cfg, uint32 pclk)
{
    uint32 Add;
    uint32 usartdiv;
    uint32 cr1;
    uint32 cr2;

    if (cfg == NULL)
    {
        return;
    }

    Add = Get_BASE_ADD(base);

    /* Disable USART before reconfiguration. */
    USART_CR1(Add) &= ~USART_CR1_UE;

    /* ---- Baud rate (rounded integer divider) -------------------- */
    usartdiv       = (pclk + (cfg->BaudRate / 2U)) / cfg->BaudRate;
    USART_BRR(Add) = usartdiv;

    /* ---- CR1: word length, parity, mode, RX interrupt ----------- */
    cr1  = USART_CR1(Add);
    cr1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);

    if (cfg->WordLength == UART_WORDLEN_9B)
    {
        cr1 |= USART_CR1_M;
    }

    if (cfg->Parity != UART_PARITY_NONE)
    {
        cr1 |= USART_CR1_PCE;
        if (cfg->Parity == UART_PARITY_ODD)
        {
            cr1 |= USART_CR1_PS;
        }
    }

    cr1 |= (uint32)cfg->Sync_Mode;   /* RXNEIE when Interrupt mode */
    cr1 |= (uint32)cfg->Mode;        /* TE / RE                    */

    USART_CR1(Add) = cr1;

    /* ---- CR2: stop bits ----------------------------------------- */
    cr2  = USART_CR2(Add);
    cr2 &= ~USART_CR2_STOP_MSK;
    cr2 |= ((uint32)cfg->StopBits << USART_CR2_STOP_POS);
    USART_CR2(Add) = cr2;

    /* ---- Enable USART ------------------------------------------- */
    USART_CR1(Add) |= USART_CR1_UE;
}

/*==================================================================
 *  UART_SendSyncBuffer — blocking transmit of <size> bytes
 *================================================================*/
void UART_SendSyncBuffer(UART_HardWare_t base, const uint8 *buf, uint8 size)
{
    uint32 Add = Get_BASE_ADD(base);
    uint8  i;

    for (i = 0U; i < size; i++)
    {
        /* Wait until the transmit data register is empty. */
        while ((USART_SR(Add) & USART_SR_TXE) == 0U)
        {
            /* busy-wait */
        }
        USART_DR(Add) = buf[i];
    }
}

/*==================================================================
 *  UART_voidSendNumber — signed decimal print
 *================================================================*/
void UART_voidSendNumber(UART_HardWare_t HardWare_Unit, sint32 Copy_sint32Number)
{
    if (Copy_sint32Number < 0)
    {
        UART_SendSyncBuffer(HardWare_Unit, (const uint8 *)"-", 1U);
        Copy_sint32Number = -Copy_sint32Number;
    }

    if (Copy_sint32Number == 0)
    {
        UART_SendSyncBuffer(HardWare_Unit, (const uint8 *)"0", 1U);
    }
    else
    {
        Write_UART_Number(HardWare_Unit, Copy_sint32Number);
    }
}

/*==================================================================
 *  Callback registration
 *================================================================*/
void UART1_CALLBACK(void (*p2function)(uint8))
{
    if (p2function != NULL)
    {
        uart1_funcPtr = p2function;
    }
}

void UART2_CALLBACK(void (*p2function)(uint8))
{
    if (p2function != NULL)
    {
        uart2_funcPtr = p2function;
    }
}

void UART3_CALLBACK(void (*p2function)(uint8))
{
    if (p2function != NULL)
    {
        uart3_funcPtr = p2function;
    }
}

/*==================================================================
 *  UART_IrqCommon
 *  Shared body for all USART interrupt handlers: deliver received
 *  byte to the registered callback and clear error flags.
 *================================================================*/
static void UART_IrqCommon(uint32 Add, void (*callback)(uint8))
{
    uint32          status = USART_SR(Add);
    volatile uint32 dummy;

    /* RX data available — reading DR clears RXNE. */
    if ((status & USART_SR_RXNE) != 0U)
    {
        uint8 rx = (uint8)USART_DR(Add);
        if (callback != NULL)
        {
            callback(rx);
        }
    }

    /* Overrun — cleared by reading SR then DR. */
    if ((status & USART_SR_ORE) != 0U)
    {
        dummy = USART_SR(Add);
        dummy = USART_DR(Add);
        (void)dummy;
    }

    /* Framing error — cleared by reading SR. */
    if ((status & USART_SR_FE) != 0U)
    {
        dummy = USART_SR(Add);
        (void)dummy;
    }

    /* Noise error — cleared by reading SR. */
    if ((status & USART_SR_NE) != 0U)
    {
        dummy = USART_SR(Add);
        (void)dummy;
    }
}

/*==================================================================
 *  IRQ handlers — referenced from the vector table.
 *  Prototypes provided here to satisfy MISRA Rule 8.4.
 *================================================================*/
void USART1_IRQHandler(void);
void USART2_IRQHandler(void);
void USART3_IRQHandler(void);

void USART1_IRQHandler(void)
{
    UART_IrqCommon(Get_BASE_ADD(UART1), uart1_funcPtr);
}

void USART2_IRQHandler(void)
{
    UART_IrqCommon(Get_BASE_ADD(UART2), uart2_funcPtr);
}

void USART3_IRQHandler(void)
{
    UART_IrqCommon(Get_BASE_ADD(UART3), uart3_funcPtr);
}
