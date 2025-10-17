#include "UART.h"

uint32 Get_BASE_ADD(uint8 base)
{
    uint32 x=0;
    switch(base)
    {
        case UART1 :   
                x= USART1_BASE;
            break;
        case UART2 :   
            x= USART2_BASE;
            break;
        case UART3 :  
            x   = USART3_BASE;
            break;
        default : 
            break; 
    }
    return x;

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

    /* ---------------- Mode (Tx/Rx) ---------------- */
    USART_CR1(Add) |= cfg->Mode;

    /* ---------------- Enable USART ---------------- */
    SET_BIT(USART_CR1(Add), 13);
}

/* ---------------- Transmit Single Byte ---------------- */
void UART_SendByte(UART_HardWare_t base, uint8 data)
{
        uint32 Add =Get_BASE_ADD(base);
    while (!GET_BIT(USART_SR(Add), 7)); // TXE
    USART_DR(Add) = data;
}

/* ---------------- Transmit String ---------------- */
void UART_SendString(UART_HardWare_t base, const char *str)
{
    while (*str) {
        UART_SendByte(base, *str++);
    }
}

/* ---------------- Receive Byte (Blocking) ---------------- */
uint8 UART_ReceiveByte(UART_HardWare_t base)
{
    uint32 Add =Get_BASE_ADD(base);
    while (!GET_BIT(USART_SR(Add), 5)); // RXNE
    return (uint8)USART_DR(Add);
}

/* ---------------- Receive Byte with Timeout ---------------- */
uint8 UART_ReceiveByte_Timeout(UART_HardWare_t base, uint32 timeout)
{
        uint32 Add =Get_BASE_ADD(base);

    while ((!GET_BIT(USART_SR(Add), 5)) && timeout--) ;
    if (timeout == 0) return 0xFF; // Timeout
    return (uint8)USART_DR(Add);
}
void UART_voidSendNumber(UART_HardWare_t HardWare_Unit,sint32 Copy_sint32Number)
{
	if(Copy_sint32Number<0)
	{
		UART_SendByte(HardWare_Unit,'-');
		Copy_sint32Number= -Copy_sint32Number;
	}
	if(Copy_sint32Number==0)
	{
 UART_SendByte(HardWare_Unit,'0');
		return;
	}
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
    UART_SendByte(HardWare_Unit,(NUM[i-1]));
}
}
