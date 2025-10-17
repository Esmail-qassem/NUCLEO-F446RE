/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* Date       : 17 OCT 2025                                            */
/* SWC        : GPIO                                                   */
/* Version    : V2.0 (For STM32F446)                                  */
/***********************************************************************/
#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "GPIO_interface.h"
static GPIO_RegDef_t* GPIO_GetPort(PORT_t port)
{
    switch (port)
    {
        case GPIO_PORTA: return GPIOA;
        case GPIO_PORTB: return GPIOB;
        case GPIO_PORTC: return GPIOC;
        case GPIO_PORTD: return GPIOD;
        case GPIO_PORTE: return GPIOE;
        case GPIO_PORTF: return GPIOF;
        case GPIO_PORTG: return GPIOG;
        case GPIO_PORTH: return GPIOH;
        default: return (GPIO_RegDef_t*)0;
    }
}

GPIO_Status_t GPIO_InitPin(PORT_t port, PIN_t pin,
                           GPIO_Mode_t mode,
                           GPIO_OutputType_t otype,
                           GPIO_Speed_t speed,
                           GPIO_Pull_t pull)
{
    GPIO_RegDef_t *GPIOx = GPIO_GetPort(port);
    if (!GPIOx) return GPIO_Wrong_Port;
    if (pin > PIN15) return GPIO_Wrong_Pin;

    /* Mode Configuration */
    GPIOx->MODER &= ~(0b11 << (pin * 2));
    GPIOx->MODER |=  ((mode & 0b11) << (pin * 2));

    /* Output Type */
    if (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_AF)
    {
        GPIOx->OTYPER &= ~(1 << pin);
        GPIOx->OTYPER |= (otype << pin);
    }

    /* Output Speed */
    GPIOx->OSPEEDR &= ~(0b11 << (pin * 2));
    GPIOx->OSPEEDR |= ((speed & 0b11) << (pin * 2));

    /* Pull-up / Pull-down */
    GPIOx->PUPDR &= ~(0b11 << (pin * 2));
    GPIOx->PUPDR |= ((pull & 0b11) << (pin * 2));

    return GPIO_OK;
}

GPIO_Status_t GPIO_WritePin(PORT_t port, PIN_t pin, uint8 value)
{
    GPIO_RegDef_t *GPIOx = GPIO_GetPort(port);
    if (!GPIOx) return GPIO_Wrong_Port;
    if (pin > PIN15) return GPIO_Wrong_Pin;

    if (value == GPIO_HIGH)
        GPIOx->BSRR = (1 << pin);
    else
        GPIOx->BSRR = (1 << (pin + 16));

    return GPIO_OK;
}

GPIO_Status_t GPIO_TogglePin(PORT_t port, PIN_t pin)
{
    GPIO_RegDef_t *GPIOx = GPIO_GetPort(port);
    if (!GPIOx) return GPIO_Wrong_Port;
    if (pin > PIN15) return GPIO_Wrong_Pin;

    TOGGLE_BIT(GPIOx->ODR, pin);
    return GPIO_OK;
}

GPIO_Status_t GPIO_ReadPin(PORT_t port, PIN_t pin, uint8 *value)
{
    GPIO_RegDef_t *GPIOx = GPIO_GetPort(port);
    if (!GPIOx) return GPIO_Wrong_Port;
    if (pin > PIN15) return GPIO_Wrong_Pin;

    *value = GET_BIT(GPIOx->IDR, pin);
    return GPIO_OK;
}

void GPIO_SetAF(PORT_t port, uint8 pin, uint8 AF)
{
    uint32 base;

    /* Get base address for selected port */
    switch (port)
    {
        case GPIO_PORTA: base = GPIOA_BASE; break;
        case GPIO_PORTB: base = GPIOB_BASE; break;
        case GPIO_PORTC: base = GPIOC_BASE; break;
        case GPIO_PORTD: base = GPIOD_BASE; break;
        case GPIO_PORTE: base = GPIOE_BASE; break;
        case GPIO_PORTF: base = GPIOF_BASE; break;
        case GPIO_PORTG: base = GPIOG_BASE; break;
        case GPIO_PORTH: base = GPIOH_BASE; break;
        default: return;
    }

    volatile uint32 *AFR_L = (volatile uint32 *)(base + 0x20); // AFRL
    volatile uint32 *AFR_H = (volatile uint32 *)(base + 0x24); // AFRH

    if (pin < 8)
    {
        uint8 shift = pin * 4;
        *AFR_L &= ~(0xF << shift);
        *AFR_L |=  ((AF & 0xF) << shift);
    }
    else
    {
        uint8 shift = (pin - 8) * 4;
        *AFR_H &= ~(0xF << shift);
        *AFR_H |=  ((AF & 0xF) << shift);
    }
}
