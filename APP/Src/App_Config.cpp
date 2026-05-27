#include "App_Config.hpp"
#include "GPIO.hpp"
#include "NVIC.hpp"
#include "ADC.hpp"
#include "GP_Timer.h"
#include "IWDG.h"
#include "I2C.hpp"
#include "OLED.h"
#include "Telemetry.hpp"
#include "ISR.hpp"
#include "MPU.h"
#include "FPU.h"

const uint8 FIRMWARE_VERSION[] = FIRMWARE_VERSION_STR;

UART_Config_t Uart1_configuration = {
    115200u,
    UART_Mode::TX_RX,
    UART_Parity::NONE,
    UART_StopBits::BITS_1,
    UART_WordLength::BITS_8,
    UART_Synch::Interrupt
};

UART_Config_t Uart2_configuration = {
    115200u,
    UART_Mode::TX_RX,
    UART_Parity::NONE,
    UART_StopBits::BITS_1,
    UART_WordLength::BITS_8,
    UART_Synch::Interrupt
};

I2C_Config_t i2c1_cfg = {
    16000000u,
    400000u,
    I2C_DutyCycle::DUTY_2,
    I2C_AddressingMode::ADDR_7BIT,
    0x00u,
    1u,
    0u,
    0u,
    0u,
    0u,
    I2C_TransferMode::POLLING
};

RTC_Config_t RTC_config = {
    RTC_CLK_LSI,
    99u,
    319u,
    RTC_HOURFORMAT_24
};

RTC_Time_t Time = { 0u, 0u, 0u, RTC_AM };

ACCEL_t Bias_accel_data;
GYRO_t  Bias_gyro_data;

void APP_init(void)
{
    UART::Init(UART_HardWare::UART1, Uart1_configuration, 16000000u);
    UART::Init(UART_HardWare::UART2, Uart2_configuration, 16000000u);
    BOOT_REASON_REPORT();
    ADC_Init();
    GP_Timer_PWM_Init(TIMER3);
    RTC_Init(&RTC_config);
    if (!RTC_IsInitialized()) RTC_SetTime(&Time);
    I2C::Init(I2C_Port::I2C1, i2c1_cfg);
    OLED_Init(I2C1_PORT);
    MPU_Init();
    for (uint16 i = 0u; i < 3000u; i++)
        for (uint8 j = 0u; j < 255u; j++);
    MPU_Calibrate(&Bias_accel_data, &Bias_gyro_data);
    IWDG_Init(IWDG_PRE_32, IWDG_CalcReload(150u, IWDG_PRE_32, 32000u));
}

void GPIO_PIN_CONFIG(void)
{
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P5,  GPIO_Mode::OUTPUT, GPIO_OType::PP, GPIO_Speed::FAST, GPIO_Pull::NONE);
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P8,  GPIO_Mode::AF,     GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);

    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P9,  GPIO_Mode::AF,     GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P10, GPIO_Mode::AF,     GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::A, 9u,  7u);
    GPIO::SetAF(GPIO_Port::A, 10u, 7u);

    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P2,  GPIO_Mode::AF,     GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P3,  GPIO_Mode::AF,     GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::A, 2u, 7u);
    GPIO::SetAF(GPIO_Port::A, 3u, 7u);

    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P0,  GPIO_Mode::ANALOG, GPIO_OType::PP, GPIO_Speed::FAST, GPIO_Pull::NONE);
    GPIO::InitPin(GPIO_Port::A, GPIO_Pin::P1,  GPIO_Mode::ANALOG, GPIO_OType::PP, GPIO_Speed::FAST, GPIO_Pull::NONE);

    GPIO::InitPin(GPIO_Port::C, GPIO_Pin::P7,  GPIO_Mode::AF,     GPIO_OType::PP, GPIO_Speed::HIGH, GPIO_Pull::NONE);
    GPIO::SetAF(GPIO_Port::C, 7u, 2u);

    GPIO::InitPin(GPIO_Port::C, GPIO_Pin::P3,  GPIO_Mode::INPUT,  GPIO_OType::PP, GPIO_Speed::FAST, GPIO_Pull::UP);

    GPIO::InitPin(GPIO_Port::B, GPIO_Pin::P8,  GPIO_Mode::AF,     GPIO_OType::OD, GPIO_Speed::FAST, GPIO_Pull::UP);
    GPIO::InitPin(GPIO_Port::B, GPIO_Pin::P9,  GPIO_Mode::AF,     GPIO_OType::OD, GPIO_Speed::FAST, GPIO_Pull::UP);
    GPIO::SetAF(GPIO_Port::B, 8u, 4u);
    GPIO::SetAF(GPIO_Port::B, 9u, 4u);
}

void ENABLE_NVIC_INTERRUPTS(void)
{
    NVIC::EnableInterrupt(UART1_IQ_NUM);
    NVIC::EnableInterrupt(UART2_IQ_NUM);
}

void CallBackFunctions(void)
{
    UART::SetCallback1(UART1_ISR);
    UART::SetCallback2(UART2_ISR);
}
