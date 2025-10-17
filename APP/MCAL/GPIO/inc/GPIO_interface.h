/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* Date       : 17 OCT 2025                                            */
/* SWC        : GPIO                                                   */
/* Version    : V2.0 (For STM32F446)                                  */
/***********************************************************************/
#ifndef GPIO_INTERFACE_H_
#define GPIO_INTERFACE_H_
#include "GPIO_private.h"

typedef enum
{
    GPIO_MODE_INPUT      = 0b00,
    GPIO_MODE_OUTPUT     = 0b01,
    GPIO_MODE_AF         = 0b10,
    GPIO_MODE_ANALOG     = 0b11
} GPIO_Mode_t;

typedef enum
{
    GPIO_OTYPE_PP = 0,
    GPIO_OTYPE_OD
} GPIO_OutputType_t;

typedef enum
{
    GPIO_SPEED_LOW = 0b00,
    GPIO_SPEED_MEDIUM = 0b01,
    GPIO_SPEED_FAST = 0b10,
    GPIO_SPEED_HIGH = 0b11
} GPIO_Speed_t;

typedef enum
{
    GPIO_NO_PULL = 0b00,
    GPIO_PULL_UP = 0b01,
    GPIO_PULL_DOWN = 0b10
} GPIO_Pull_t;

typedef enum
{
    GPIO_PORTA,
    GPIO_PORTB,
    GPIO_PORTC,
    GPIO_PORTD,
    GPIO_PORTE,
    GPIO_PORTF,
    GPIO_PORTG,
    GPIO_PORTH
} PORT_t;

typedef enum
{
    PIN0, PIN1, PIN2, PIN3, PIN4, PIN5, PIN6, PIN7,
    PIN8, PIN9, PIN10, PIN11, PIN12, PIN13, PIN14, PIN15
} PIN_t;

typedef enum
{
    GPIO_OK,
    GPIO_Wrong_Port,
    GPIO_Wrong_Pin
} GPIO_Status_t;

#define GPIO_HIGH 1
#define GPIO_LOW  0

/* --------- Function Prototypes --------- */
GPIO_Status_t GPIO_InitPin(PORT_t port, PIN_t pin,
                           GPIO_Mode_t mode,
                           GPIO_OutputType_t otype,
                           GPIO_Speed_t speed,
                           GPIO_Pull_t pull);

GPIO_Status_t GPIO_WritePin(PORT_t port, PIN_t pin, uint8 value);
GPIO_Status_t GPIO_TogglePin(PORT_t port, PIN_t pin);
GPIO_Status_t GPIO_ReadPin(PORT_t port, PIN_t pin, uint8 *value);
void GPIO_SetAF(PORT_t port, uint8 pin, uint8 AF);
#endif /* GPIO_INTERFACE_H_ */
