#include "UART.h"
uint8 rx_test;
void (*funcPtr)(uint8)=NULL;

uint32 Get_BASE_ADD(uint8 base)
{
    uint32 x=0;
    switch(base)
    {
        case UART1 :   x= USART1_BASE; break;
        case UART2 :   x= USART2_BASE; break;
        case UART3 :   x= USART3_BASE; break;
        default :      break; 
    }
    return x;
}
void Write_UART_Number(UART_HardWare_t HardWare_Unit, sint32 Copy_sint32Number)
{
    uint8 NUM[10];
    uint8 Local_uint8Counter=0;
    while(Copy_sint32Number>0)
    {
    	NUM[Local_uint8Counter++]=(Copy_sint32Number%10)+'0';
    	Copy_sint32Number/=10;
    }
    /*reverse*/
    for(uint8 i=Local_uint8Counter;i>0;i--)
    {
        UART_SendSyncBuffer(HardWare_Unit,(uint8*)(NUM + i - 1),1u);
    }
}
/* ---------------- Initialization ---------------- */
void UART_Init(UART_HardWare_t base, const UART_Config_t *cfg, uint32 pclk)
{
    if (!cfg) return;
    uint32 Add =Get_BASE_ADD(base);

    /* Disable USART before config */
    CLEAR_BIT(USART_CR1(Add), 13);

    /* ---------------- Baud Rate Setup ---------------- */
    uint32 usartdiv = (pclk + (cfg->BaudRate / 2U)) / cfg->BaudRate;
    USART_BRR(Add) = usartdiv;

    /* ---------------- Word Length ---------------- */
    if (cfg->WordLength == UART_WORDLEN_9B)
        SET_BIT(USART_CR1(Add), 12);
    else
        CLEAR_BIT(USART_CR1(Add), 12);

    /* ---------------- Parity ---------------- */
    if (cfg->Parity == UART_PARITY_NONE) {
        CLEAR_BIT(USART_CR1(Add), 10); // Parity control disable
    } else {
        SET_BIT(USART_CR1(Add), 10); // Enable parity
        if (cfg->Parity == UART_PARITY_ODD)
            SET_BIT(USART_CR1(Add), 9);
        else
            CLEAR_BIT(USART_CR1(Add), 9);
    }

    /* ---------------- Stop Bits ---------------- */
    USART_CR2(Add) &= ~(0x3 << 12);
    USART_CR2(Add) |= (cfg->StopBits << 12);

    /* ---------------- INTERRUPT (Tx/Rx) ---------------- */
    USART_CR1(Add) |= cfg->Sync_Mode;

    /* ---------------- Mode (Tx/Rx) ---------------- */
    USART_CR1(Add) |= cfg->Mode;

    /* ---------------- Enable USART ---------------- */
    SET_BIT(USART_CR1(Add), 13);
}

/* ---------------- Transmit Single Byte ---------------- */
void UART_SendSyncBuffer(UART_HardWare_t base, const uint8 *buf, uint8 size)

{
    uint32 Add = Get_BASE_ADD(base);

    for (uint8 i = 0; i < size; i++)
    {
        while (!GET_BIT(USART_SR(Add), 7)); // Wait until TXE = 1
        USART_DR(Add) = *(buf + i);
    }

    }
void UART_voidSendNumber(UART_HardWare_t HardWare_Unit,sint32 Copy_sint32Number)
{
	if(Copy_sint32Number<0)
	{
		UART_SendSyncBuffer(HardWare_Unit,(uint8*)"-",1u);
		Copy_sint32Number= -Copy_sint32Number;
	}
	if(Copy_sint32Number==0)
	{
        UART_SendSyncBuffer(HardWare_Unit,(uint8*)"0",1u);
		return;
	}
    Write_UART_Number(HardWare_Unit,Copy_sint32Number);
}

void UART2_CALLBACK(void(*p2function)(uint8))
{
    if(p2function!= NULL)
    {
         funcPtr = p2function;
    }
}

void USART2_IRQHandler(void)
{   
    uint32 Add = Get_BASE_ADD(UART2);
    uint32 status = USART_SR(Add);
    
    /* RX Data Available */
    if (status & (1 << 5))  // RXNE
    {
        rx_test= (uint8)USART_DR(Add);  // Reading DR clears RXNE
        funcPtr(rx_test);
    }
    
    /* Overrun Error */
    if (status & (1 << 3))  // ORE
    {
        // Clear by reading SR then DR
        volatile uint32 temp = USART_SR(Add);
        temp = USART_DR(Add);
        (void)temp;
    }
    
    /* Framing Error */
    if (status & (1 << 2))  // FE
    {
        // Clear by reading SR
        volatile uint32 temp = USART_SR(Add);
        (void)temp;
    }
    
    /* Noise Error */
    if (status & (1 << 1))  // NE  
    {
        // Clear by reading SR
        volatile uint32 temp = USART_SR(Add);
        (void)temp;
    }
}