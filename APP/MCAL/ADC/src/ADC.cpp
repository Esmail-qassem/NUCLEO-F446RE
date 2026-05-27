/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : ADC                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "ADC.hpp"

/* SR bits */
static constexpr uint32 ADC_SR_EOC  = (1u << 1u);
static constexpr uint32 ADC_SR_OVR  = (1u << 5u);
/* CR1 bits */
static constexpr uint32 ADC_CR1_RES_12BIT = (0u << 24u);
static constexpr uint32 ADC_CR1_SCAN      = (1u <<  8u);
/* CR2 bits */
static constexpr uint32 ADC_CR2_ADON    = (1u <<  0u);
static constexpr uint32 ADC_CR2_CONT    = (1u <<  1u);
static constexpr uint32 ADC_CR2_ALIGN   = (1u << 11u);
static constexpr uint32 ADC_CR2_EOCS    = (1u << 10u);
static constexpr uint32 ADC_CR2_SWSTART = (1u << 30u);
/* CCR bits */
static constexpr uint32 ADC_CCR_ADCPRE_4 = (1u << 16u);

void ADC::SetChannelSampleTime(uint8 channel, ADC_SampleTime st)
{
    uint8 bits = static_cast<uint8>(st);
    if (channel <= 9u) {
        ADC1_REGS->SMPR2 &= ~(0x7u << (channel * 3u));
        ADC1_REGS->SMPR2 |=  (static_cast<uint32>(bits) << (channel * 3u));
    } else if (channel <= 18u) {
        uint8 shift = static_cast<uint8>((channel - 10u) * 3u);
        ADC1_REGS->SMPR1 &= ~(0x7u << shift);
        ADC1_REGS->SMPR1 |=  (static_cast<uint32>(bits) << shift);
    }
}

void ADC::Init(void)
{
    ADC_COMMON_REGS->CCR &= ~(0x3u << 16u);
    ADC_COMMON_REGS->CCR |=  ADC_CCR_ADCPRE_4;

    ADC1_REGS->CR2 &= ~ADC_CR2_ADON;

    ADC1_REGS->CR1 &= ~(0x3u << 24u);
    ADC1_REGS->CR1 |=  ADC_CR1_RES_12BIT;

    ADC1_REGS->CR2 &= ~ADC_CR2_CONT;
    ADC1_REGS->CR2 |=  ADC_CR2_EOCS;
    ADC1_REGS->CR2 &= ~ADC_CR2_ALIGN;

    ADC_COMMON_REGS->CCR &= ~(1u << 22u);
    ADC_COMMON_REGS->CCR |=  (1u << 23u);

    ADC1_REGS->CR1 &= ~ADC_CR1_SCAN;
    ADC1_REGS->SQR1 &= ~(0xFu << 20u);

    for (uint8 ch = 0u; ch <= 18u; ch++)
        SetChannelSampleTime(ch, ADC_SampleTime::CYCLES_480);

    ADC1_REGS->CR2 |= ADC_CR2_ADON;
    for (volatile uint32 i = 0u; i < 100000u; i++) {}
}

uint16 ADC::Read(uint8 channel)
{
    ADC1_REGS->SR  &= ~(ADC_SR_EOC | ADC_SR_OVR);
    ADC1_REGS->SQR3 = (channel & 0x1Fu);
    ADC1_REGS->SQR1 &= ~(0xFu << 20u);
    ADC1_REGS->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1_REGS->SR & ADC_SR_EOC)) {}
    return static_cast<uint16>(ADC1_REGS->DR & 0x0FFFu);
}

uint16 ADC::ReadAveraged(uint8 channel)
{
    uint32 sum = 0u;
    for (uint8 i = 0u; i < ADC_AVERAGE_SAMPLES; i++) {
        sum += Read(channel);
        for (volatile uint32 d = 0u; d < 500u; d++) {}
    }
    return static_cast<uint16>(sum / ADC_AVERAGE_SAMPLES);
}

uint32 ADC::ReadVoltage_mV(uint8 channel)
{
    uint16 raw = ReadAveraged(channel);
    return (static_cast<uint32>(raw) * 3300u) / 4096u;
}

void ADC::SetSampleTime(uint8 channel, ADC_SampleTime st)
{
    SetChannelSampleTime(channel, st);
}
