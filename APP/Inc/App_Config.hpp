#pragma once

#include "STD_TYPES.h"
#include "UART.hpp"
#include "I2C.hpp"
#include "RTC.h"

#define FIRMWARE_VERSION_STR  "1.0.1"
extern const uint8        FIRMWARE_VERSION[];
extern UART_Config_t      Uart1_configuration;
extern UART_Config_t      Uart2_configuration;
extern I2C_Config_t       i2c1_cfg;
extern RTC_Config_t       RTC_config;
extern RTC_Time_t         Time;

void APP_init(void);
void GPIO_PIN_CONFIG(void);
void ENABLE_NVIC_INTERRUPTS(void);
void CallBackFunctions(void);
