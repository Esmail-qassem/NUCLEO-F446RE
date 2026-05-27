#include "ADC.hpp"
/* Legacy shims */
#define ADC_Init()              ADC::Init()
#define ADC_Read(ch)            ADC::Read(ch)
#define ADC_ReadAveraged(ch)    ADC::ReadAveraged(ch)
#define ADC_ReadVoltage_mV(ch)  ADC::ReadVoltage_mV(ch)
#define ADC_SetSampleTime(ch,s) ADC::SetSampleTime(ch, static_cast<ADC_SampleTime>(s))
#define ADC_SAMPLETIME_3   ADC_SampleTime::CYCLES_3
#define ADC_SAMPLETIME_480 ADC_SampleTime::CYCLES_480

#ifndef ADC_H
#define ADC_H

#include "STD_TYPES.h"

/*===========================================================================
                        REGISTER BASE ADDRESSES
===========================================================================*/
#define ADC1_BASE        0x40012000UL
#define ADC_COMMON_BASE  0x40012300UL

/*===========================================================================
                        ADC1 REGISTERS
===========================================================================*/
#define ADC_SR           (*((volatile uint32*)(ADC1_BASE + 0x00)))
#define ADC_CR1          (*((volatile uint32*)(ADC1_BASE + 0x04)))
#define ADC_CR2          (*((volatile uint32*)(ADC1_BASE + 0x08)))
#define ADC_SMPR1        (*((volatile uint32*)(ADC1_BASE + 0x0C)))
#define ADC_SMPR2        (*((volatile uint32*)(ADC1_BASE + 0x10)))
#define ADC_SQR1         (*((volatile uint32*)(ADC1_BASE + 0x2C)))
#define ADC_SQR2         (*((volatile uint32*)(ADC1_BASE + 0x30)))
#define ADC_SQR3         (*((volatile uint32*)(ADC1_BASE + 0x34)))
#define ADC_DR           (*((volatile uint32*)(ADC1_BASE + 0x4C)))

/*===========================================================================
                        ADC COMMON REGISTERS
===========================================================================*/
#define ADC_CCR          (*((volatile uint32*)(ADC_COMMON_BASE + 0x04)))

/*===========================================================================
                        RCC REGISTERS
===========================================================================*/

/*===========================================================================
                        ADC SR BITS
===========================================================================*/
#define ADC_SR_EOC       (1 << 1)   /* End of conversion */
#define ADC_SR_OVR       (1 << 5)   /* Overrun */

/*===========================================================================
                        ADC CR1 BITS
===========================================================================*/
#define ADC_CR1_RES_12BIT  (0 << 24)   /* 12-bit resolution */
#define ADC_CR1_RES_10BIT  (1 << 24)   /* 10-bit resolution */
#define ADC_CR1_RES_8BIT   (2 << 24)   /* 8-bit resolution  */
#define ADC_CR1_RES_6BIT   (3 << 24)   /* 6-bit resolution  */
#define ADC_CR1_SCAN       (1 << 8)    /* Scan mode         */

/*===========================================================================
                        ADC CR2 BITS
===========================================================================*/
#define ADC_CR2_ADON       (1 << 0)    /* ADC ON            */
#define ADC_CR2_CONT       (1 << 1)    /* Continuous mode   */
#define ADC_CR2_ALIGN      (1 << 11)   /* Data alignment    */
#define ADC_CR2_EOCS       (1 << 10)   /* EOC after each    */
#define ADC_CR2_SWSTART    (1 << 30)   /* Software start    */

/*===========================================================================
                        ADC CCR BITS (Common)
===========================================================================*/
#define ADC_CCR_ADCPRE_2   (0 << 16)   /* PCLK2 / 2        */
#define ADC_CCR_ADCPRE_4   (1 << 16)   /* PCLK2 / 4        */
#define ADC_CCR_ADCPRE_6   (2 << 16)   /* PCLK2 / 6        */
#define ADC_CCR_ADCPRE_8   (3 << 16)   /* PCLK2 / 8        */

/*===========================================================================
                        SAMPLING TIME OPTIONS
===========================================================================*/
/* ADC_SMPR: 3 bits per channel */
#define ADC_SAMPLETIME_3    0   /* 3 cycles   — fastest, noisiest */
#define ADC_SAMPLETIME_15   1   /* 15 cycles                      */
#define ADC_SAMPLETIME_28   2   /* 28 cycles                      */
#define ADC_SAMPLETIME_56   3   /* 56 cycles                      */
#define ADC_SAMPLETIME_84   4   /* 84 cycles                      */
#define ADC_SAMPLETIME_112  5   /* 112 cycles                     */
#define ADC_SAMPLETIME_144  6   /* 144 cycles                     */
#define ADC_SAMPLETIME_480  7   /* 480 cycles — slowest, cleanest */

/*===========================================================================
                        CHANNEL DEFINITIONS
===========================================================================*/
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
#define ADC_CHANNEL_10   10
#define ADC_CHANNEL_11   11
#define ADC_CHANNEL_12   12
#define ADC_CHANNEL_13   13
#define ADC_CHANNEL_14   14
#define ADC_CHANNEL_15   15
#define ADC_CHANNEL_TEMP 16   /* Internal temperature sensor */
#define ADC_CHANNEL_VREF 17   /* Internal VREF               */
#define ADC_CHANNEL_VBAT 18   /* VBAT / 4                    */

/*===========================================================================
                        AVERAGING SAMPLES
===========================================================================*/
#define ADC_AVERAGE_SAMPLES  64

/*===========================================================================
                        FUNCTION PROTOTYPES
===========================================================================*/
void     ADC_Init(void);
uint16   ADC_Read(uint8 channel);
uint16   ADC_ReadAveraged(uint8 channel);
uint32   ADC_ReadVoltage_mV(uint8 channel);
void     ADC_SetSampleTime(uint8 channel, uint8 sampleTime);

#endif /* ADC_H */