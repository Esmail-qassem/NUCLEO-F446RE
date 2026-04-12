#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include "STD_TYPES.h"
#include "UART.h"
#include "I2C.h"
#include "RTC.h"

/*------------------------------------------------------------------
 *  Shared globals (defined in App_Config.c, used across modules)
 *------------------------------------------------------------------*/
#define FIRMWARE_VERSION_STR  "1.0.1"
extern const uint8        FIRMWARE_VERSION[];
extern UART_Config_t      Uart1_configuration;
extern UART_Config_t      Uart2_configuration;
extern I2C_Config_t       i2c1_cfg;
extern RTC_Config_t       RTC_config;
extern RTC_Time_t         Time;

/*------------------------------------------------------------------
 *  System setup
 *------------------------------------------------------------------*/
void APP_init(void);
void GPIO_PIN_CONFIG(void);
void ENABLE_NVIC_INTERRUPTS(void);
void CallBackFunctions(void);

#endif /* APP_CONFIG_H_ */
