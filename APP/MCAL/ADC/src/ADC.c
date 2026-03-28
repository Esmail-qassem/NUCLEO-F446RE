#include "STD_TYPES.h"

#include "ADC.h"

void ADC_Init(void)
{

    /*  Sampling time (longer = more stable) */
    ADC_SMPR2 |= (7 << (0 * 3));  // Channel 0 sample time

    /*  Enable ADC */
    ADC_CR2 |= ADC_CR2_ADON;
}

uint16_t ADC_Read(uint8_t channel)
{
    /* 1. Select channel */
    ADC_SQR3 = channel;

    /* 2. Start conversion */
    ADC_CR2 |= ADC_CR2_SWSTART;

    /* 3. Wait for conversion complete */
    while (!(ADC_SR & ADC_SR_EOC));

    /* 4. Read data */
    return (uint16_t)ADC_DR;
}