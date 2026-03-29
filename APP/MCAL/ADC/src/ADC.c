#include "ADC.h"

/*===========================================================================
                        PRIVATE HELPERS
===========================================================================*/
static void ADC_SetChannelSampleTime(uint8 channel, uint8 sampleTime)
{
    if (channel <= 9)
    {
        /* SMPR2 handles channels 0-9 */
        ADC_SMPR2 &= ~(0x7 << (channel * 3));
        ADC_SMPR2 |=  (sampleTime << (channel * 3));
    }
    else if (channel <= 18)
    {
        /* SMPR1 handles channels 10-18 */
        uint8 shift = (channel - 10) * 3;
        ADC_SMPR1 &= ~(0x7 << shift);
        ADC_SMPR1 |=  (sampleTime << shift);
    }
}

/*===========================================================================
                        ADC INIT
===========================================================================*/
void ADC_Init(void)
{
     /*--- ADC prescaler: PCLK2/4 → 16MHz/4 = 4MHz (must be ≤ 36MHz) ---*/
    ADC_CCR &= ~(0x3 << 16);
    ADC_CCR |=  ADC_CCR_ADCPRE_4;

    /*--- Disable ADC before configuring ---*/
    ADC_CR2 &= ~ADC_CR2_ADON;

    /*--- Resolution: 12-bit ---*/
    ADC_CR1 &= ~(0x3 << 24);
    ADC_CR1 |=  ADC_CR1_RES_12BIT;

    /*--- Single conversion mode (not continuous) ---*/
    ADC_CR2 &= ~ADC_CR2_CONT;

    /*--- EOC set after each single conversion ---*/
    ADC_CR2 |= ADC_CR2_EOCS;

    /*--- Right data alignment ---*/
    ADC_CR2 &= ~ADC_CR2_ALIGN;
    ADC_CCR &= ~(1 << 22); // VBATE bit — disable VBAT channel (not used)

    ADC_CCR |= (1 << 23);  // TSVREFE bit — enables internal temp sensor + VREF
    /*--- Disable scan mode (single channel per conversion) ---*/
    ADC_CR1 &= ~ADC_CR1_SCAN;

    /*--- Sequence length = 1 conversion ---*/
    ADC_SQR1 &= ~(0xF << 20);  /* L[3:0] = 0000 → 1 conversion */

    /*--- Set sampling time for all channels to 480 cycles (most stable) ---*/
    for (uint8 ch = 0; ch <= 18; ch++)
    {
        ADC_SetChannelSampleTime(ch, ADC_SAMPLETIME_480);
    }

    /*--- Enable ADC ---*/
    ADC_CR2 |= ADC_CR2_ADON;

    /*--- Stabilization delay — required after ADON ---*/
    for (volatile uint32 i = 0; i < 100000; i++);
}

/*===========================================================================
                        ADC READ — Single conversion
===========================================================================*/
uint16 ADC_Read(uint8 channel)
{
    /*--- Clear overrun and EOC flags ---*/
    ADC_SR &= ~(ADC_SR_EOC | ADC_SR_OVR);

    /*--- Select channel in sequence register 3 ---*/
    ADC_SQR3 = (channel & 0x1F);

    /*--- Ensure sequence length = 1 ---*/
    ADC_SQR1 &= ~(0xF << 20);

    /*--- Start conversion ---*/
    ADC_CR2 |= ADC_CR2_SWSTART;

    /*--- Wait for end of conversion ---*/
    while (!(ADC_SR & ADC_SR_EOC));

    /*--- Read and return result (reading DR clears EOC) ---*/
    return (uint16)(ADC_DR & 0x0FFF);
}

/*===========================================================================
                        ADC READ AVERAGED — 16 samples
===========================================================================*/
uint16 ADC_ReadAveraged(uint8 channel)
{
    uint32 sum = 0;

    for (uint8 i = 0; i < ADC_AVERAGE_SAMPLES; i++)
    {
        sum += ADC_Read(channel);

        /* Short delay between samples */
        for (volatile uint32 d = 0; d < 500; d++);
    }

    return (uint16)(sum / ADC_AVERAGE_SAMPLES);
}

/*===========================================================================
                        ADC READ VOLTAGE — returns mV
                        VREF = 3300mV, 12-bit resolution = 4096 steps
===========================================================================*/
uint32 ADC_ReadVoltage_mV(uint8 channel)
{
    uint16 raw = ADC_ReadAveraged(channel);
    return ((uint32)raw * 3300) / 4096;
}

/*===========================================================================
                        SET SAMPLE TIME FOR SPECIFIC CHANNEL
===========================================================================*/
void ADC_SetSampleTime(uint8 channel, uint8 sampleTime)
{
    ADC_SetChannelSampleTime(channel, sampleTime);
}