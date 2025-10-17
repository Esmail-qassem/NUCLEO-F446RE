#ifndef RCC_H_
#define RCC_H_

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ------------------ Base Address ------------------ */
#define RCC_BASE        0x40023800UL

/* ------------------ Register Definitions ------------------ */
#define RCC_CR          (*(volatile uint32*)(RCC_BASE + 0x00))
#define RCC_PLLCFGR     (*(volatile uint32*)(RCC_BASE + 0x04))
#define RCC_CFGR        (*(volatile uint32*)(RCC_BASE + 0x08))
#define RCC_CIR         (*(volatile uint32*)(RCC_BASE + 0x0C))
#define RCC_AHB1RSTR    (*(volatile uint32*)(RCC_BASE + 0x10))
#define RCC_AHB2RSTR    (*(volatile uint32*)(RCC_BASE + 0x14))
#define RCC_AHB3RSTR    (*(volatile uint32*)(RCC_BASE + 0x18))
#define RCC_APB1RSTR    (*(volatile uint32*)(RCC_BASE + 0x20))
#define RCC_APB2RSTR    (*(volatile uint32*)(RCC_BASE + 0x24))
#define RCC_AHB1ENR     (*(volatile uint32*)(RCC_BASE + 0x30))
#define RCC_AHB2ENR     (*(volatile uint32*)(RCC_BASE + 0x34))
#define RCC_AHB3ENR     (*(volatile uint32*)(RCC_BASE + 0x38))
#define RCC_APB1ENR     (*(volatile uint32*)(RCC_BASE + 0x40))
#define RCC_APB2ENR     (*(volatile uint32*)(RCC_BASE + 0x44))
#define RCC_AHB1LPENR   (*(volatile uint32*)(RCC_BASE + 0x50))
#define RCC_AHB2LPENR   (*(volatile uint32*)(RCC_BASE + 0x54))
#define RCC_APB1LPENR   (*(volatile uint32*)(RCC_BASE + 0x60))
#define RCC_APB2LPENR   (*(volatile uint32*)(RCC_BASE + 0x64))
#define RCC_BDCR        (*(volatile uint32*)(RCC_BASE + 0x70))
#define RCC_CSR         (*(volatile uint32*)(RCC_BASE + 0x74))
#define RCC_SSCGR       (*(volatile uint32*)(RCC_BASE + 0x80))
#define RCC_PLLI2SCFGR  (*(volatile uint32*)(RCC_BASE + 0x84))
#define RCC_DCKCFGR     (*(volatile uint32*)(RCC_BASE + 0x8C))



typedef enum {
    AHB1_GPIOA = 0,
    AHB1_GPIOB = 1,
    AHB1_GPIOC = 2,
    AHB1_GPIOD = 3,
    AHB1_GPIOE = 4,
    AHB1_GPIOF = 5,
    AHB1_GPIOG = 6,
    AHB1_GPIOH = 7,
    AHB1_CRC   = 12,
    AHB1_BKPSRAM = 18,
    AHB1_DMA1  = 21,
    AHB1_DMA2  = 22,
    AHB1_OTGHS  = 29,
    AHB1_OTGHSULPI = 30,
    AHB2_DCMIEN=0,
    AHB2_OTGFSEN=7,
    AHB3_FMCEN=0,
    AHB3_QSPIEN=1,
    APB1_TIM2 = 0,
    APB1_TIM3,
    APB1_TIM4,
    APB1_TIM5,
    APB1_WWDG = 11,
    APB1_SPI2 = 14,
    APB1_SPI3,
    APB1_USART2 = 17,
    APB1_USART3 ,
    APB1_USART4 ,
    APB1_USART5 ,
    APB1_I2C1 = 21,
    APB1_I2C2,
    APB1_I2C3,
    APB1_PWR = 28,

    APB2_TIM1 = 0,
    APB2_USART1 = 4,
    APB2_USART6,
    APB2_ADC1 = 8,
    APB2_SDIO = 11,
    APB2_SPI1,
    APB2_SPI4,
    APB2_SYSCFG = 14,
    APB2_TIM9 = 16,
    APB2_TIM10,
    APB2_TIM11
} RCC_Peripheral_t;

typedef enum {
    RCC_CLK_HSI = 0x00,
    RCC_CLK_HSE = 0x01,
    RCC_CLK_PLL = 0x02
} RCC_ClockSource_t;


typedef enum {
    RCC_PLLSRC_HSI = 0,
    RCC_PLLSRC_HSE = 1
} RCC_PLLSource_t;

typedef enum {
    AHB_PRE_1   = 0x0,
    AHB_PRE_2   = 0x8,
    AHB_PRE_4   = 0x9,
    AHB_PRE_8   = 0xA,
    AHB_PRE_16  = 0xB,
    AHB_PRE_64  = 0xC,
    AHB_PRE_128 = 0xD,
    AHB_PRE_256 = 0xE,
    AHB_PRE_512 = 0xF
} RCC_AHBPrescaler_t;

/* ------------------ Bus Enums ------------------ */
typedef enum {
    RCC_AHB1,
    RCC_AHB2,
    RCC_AHB3,
    RCC_APB1,
    RCC_APB2
} RCC_Bus_t;

typedef enum {
    APB_PRE_1 = 0x0,
    APB_PRE_2 = 0x4,
    APB_PRE_4 = 0x5,
    APB_PRE_8 = 0x6,
    APB_PRE_16 = 0x7
} RCC_APBPrescaler_t;

typedef struct {
    RCC_PLLSource_t PLL_Source;
    uint8 PLL_M;   // Divider
    uint16 PLL_N;  // Multiplier
    uint8 PLL_P;   // Main system divider
    uint8 PLL_Q;   // USB/SDIO/RNG clock divider
} RCC_PLLConfig_t;


typedef struct {
    RCC_ClockSource_t ClockSource;
    RCC_PLLConfig_t PLL_Config;
    RCC_AHBPrescaler_t AHB_Prescaler;
    RCC_APBPrescaler_t APB1_Prescaler;
    RCC_APBPrescaler_t APB2_Prescaler;
} RCC_Config_t;

/* ------------------ API ------------------ */
void RCC_Init(const RCC_Config_t *cfg);

void RCC_EnableClock(uint8 bus, uint8 periphID);
void RCC_DisableClock(uint8 bus, uint8 periphID);


#endif /* RCC_H_ */
