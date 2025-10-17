/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* Date       : 17 OCT 2025                                            */
/* SWC        : GPIO                                                   */
/* Version    : V2.0 (For STM32F446)                                  */
/***********************************************************************/
#ifndef GPIO_PRIVATE_H_
#define GPIO_PRIVATE_H_

typedef struct
{
    volatile uint32 MODER;    /* Mode Register                     Offset 0x00 */
    volatile uint32 OTYPER;   /* Output Type Register               Offset 0x04 */
    volatile uint32 OSPEEDR;  /* Output Speed Register              Offset 0x08 */
    volatile uint32 PUPDR;    /* Pull-up/Pull-down Register         Offset 0x0C */
    volatile uint32 IDR;      /* Input Data Register                Offset 0x10 */
    volatile uint32 ODR;      /* Output Data Register               Offset 0x14 */
    volatile uint32 BSRR;     /* Bit Set/Reset Register             Offset 0x18 */
    volatile uint32 LCKR;     /* Configuration Lock Register        Offset 0x1C */
    volatile uint32 AFRL;     /* Alternate Function Low Register    Offset 0x20 */
    volatile uint32 AFRH;     /* Alternate Function High Register   Offset 0x24 */
} GPIO_RegDef_t;

/* Base Addresses */
#define GPIOA_BASE   0x40020000UL
#define GPIOB_BASE   0x40020400UL
#define GPIOC_BASE   0x40020800UL
#define GPIOD_BASE   0x40020C00UL
#define GPIOE_BASE   0x40021000UL
#define GPIOF_BASE   0x40021400UL
#define GPIOG_BASE   0x40021800UL
#define GPIOH_BASE   0x40021C00UL

/* Pointers to structures */
#define GPIOA   ((GPIO_RegDef_t*)GPIOA_BASE)
#define GPIOB   ((GPIO_RegDef_t*)GPIOB_BASE)
#define GPIOC   ((GPIO_RegDef_t*)GPIOC_BASE)
#define GPIOD   ((GPIO_RegDef_t*)GPIOD_BASE)
#define GPIOE   ((GPIO_RegDef_t*)GPIOE_BASE)
#define GPIOF   ((GPIO_RegDef_t*)GPIOF_BASE)
#define GPIOG   ((GPIO_RegDef_t*)GPIOG_BASE)
#define GPIOH   ((GPIO_RegDef_t*)GPIOH_BASE)

#endif /* GPIO_PRIVATE_H_ */
