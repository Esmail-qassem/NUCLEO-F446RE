#ifndef UART_PRIVATE_H_
#define UART_PRIVATE_H_

/*==================================================================
 *  UART_private.h
 *  STM32F446 USART register bit definitions (RM0390 §25.6).
 *  Internal to the UART MCAL driver.
 *================================================================*/

/*------------------------------------------------------------------
 *  USART_SR — Status Register
 *------------------------------------------------------------------*/
#define USART_SR_NE         (1UL << 1U)     /* Noise error   */
#define USART_SR_FE         (1UL << 2U)     /* Framing error */
#define USART_SR_ORE        (1UL << 3U)     /* Overrun error */
#define USART_SR_RXNE       (1UL << 5U)     /* RX not empty  */
#define USART_SR_TXE        (1UL << 7U)     /* TX empty      */

/*------------------------------------------------------------------
 *  USART_CR1 — Control Register 1
 *------------------------------------------------------------------*/
#define USART_CR1_RE        (1UL << 2U)     /* Receiver enable          */
#define USART_CR1_TE        (1UL << 3U)     /* Transmitter enable       */
#define USART_CR1_RXNEIE    (1UL << 5U)     /* RXNE interrupt enable    */
#define USART_CR1_PS        (1UL << 9U)     /* Parity selection (0=even)*/
#define USART_CR1_PCE       (1UL << 10U)    /* Parity control enable    */
#define USART_CR1_M         (1UL << 12U)    /* Word length (0=8b,1=9b)  */
#define USART_CR1_UE        (1UL << 13U)    /* USART enable             */

/*------------------------------------------------------------------
 *  USART_CR2 — Control Register 2
 *------------------------------------------------------------------*/
#define USART_CR2_STOP_POS  (12U)
#define USART_CR2_STOP_MSK  (3UL << USART_CR2_STOP_POS)

/*------------------------------------------------------------------
 *  Decimal-to-ASCII conversion buffer (max digits in a uint32)
 *------------------------------------------------------------------*/
#define UART_NUM_BUF_LEN    (10U)

#endif /* UART_PRIVATE_H_ */
