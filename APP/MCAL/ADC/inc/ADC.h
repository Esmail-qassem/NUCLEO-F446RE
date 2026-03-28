#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/* ADC Channels (for STM32F4) */
#define ADC_CHANNEL_0    0
#define ADC_CHANNEL_1    1
#define ADC_CHANNEL_2    2
#define ADC_CHANNEL_3    3
#define ADC_CHANNEL_4    4
#define ADC_CHANNEL_5    5
#define ADC_CHANNEL_6    6
#define ADC_CHANNEL_7    7
#define ADC_CHANNEL_8    8
#define ADC_CHANNEL_9    9
/* Base addresses (STM32F4) */
#define PERIPH_BASE        0x40000000UL
#define APB2_OFFSET        0x00010000UL

#define APB2PERIPH_BASE    (PERIPH_BASE + APB2_OFFSET)


/* ADC1 */
#define ADC1_BASE          (APB2PERIPH_BASE + 0x2000)

#define ADC_SR             (*(volatile uint32*)(ADC1_BASE + 0x00))
#define ADC_CR1            (*(volatile uint32*)(ADC1_BASE + 0x04))
#define ADC_CR2            (*(volatile uint32*)(ADC1_BASE + 0x08))
#define ADC_SMPR2          (*(volatile uint32*)(ADC1_BASE + 0x10))
#define ADC_SQR3           (*(volatile uint32*)(ADC1_BASE + 0x34))
#define ADC_DR             (*(volatile uint32*)(ADC1_BASE + 0x4C))

/* Bits */
#define ADC_CR2_ADON       (1 << 0)
#define ADC_CR2_SWSTART    (1 << 30)
#define ADC_SR_EOC         (1 << 1)


void ADC_Init(void);
uint16_t ADC_Read(uint8_t channel);

#endif