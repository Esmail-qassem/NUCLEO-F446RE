#include "App_Config.h"
#include "GPIO_interface.h"
#include "NVIC_interface.h"
#include "ADC.h"
#include "GP_Timer.h"
#include "IWDG.h"
#include "I2C.h"
#include "OLED.h"
#include "Telemetry.h"
#include "ISR.h"

/*------------------------------------------------------------------
 *  Firmware version — single definition, shared via App_Config.h
 *------------------------------------------------------------------*/
const uint8 FIRMWARE_VERSION[] = FIRMWARE_VERSION_STR;

/*------------------------------------------------------------------
 *  Peripheral configuration structs
 *------------------------------------------------------------------*/
UART_Config_t Uart1_configuration = {
    115200,
    UART_MODE_TX_RX,
    UART_PARITY_NONE,
    UART_STOPBITS_1,
    UART_WORDLEN_8B,
    Interrupt
};

UART_Config_t Uart2_configuration = {
    115200,
    UART_MODE_TX_RX,
    UART_PARITY_NONE,
    UART_STOPBITS_1,
    UART_WORDLEN_8B,
    Interrupt
};

I2C_Config_t i2c1_cfg = {
    .PCLK1_Hz        = 16000000,
    .ClockSpeed      = 400000,
    .DutyCycle       = I2C_DUTY_2,
    .AddressingMode  = I2C_ADDR_7BIT,
    .OwnAddress      = 0x00,
    .Acknowledgement = 1,
    .TransferMode    = I2C_POLLING,
};

/* LSI ~32 kHz: prediv_a=99, prediv_s=319 → 1 Hz calendar tick */
RTC_Config_t RTC_config = {
    RTC_CLK_LSI,
    99,
    319,
    RTC_HOURFORMAT_24
};

RTC_Time_t Time = { 0, 0, 0, RTC_AM };

/*------------------------------------------------------------------
 *  APP_init — peripheral bring-up sequence
 *------------------------------------------------------------------*/
void APP_init(void)
{
    UART_Init(UART1, &Uart1_configuration, 16000000);
    UART_Init(UART2, &Uart2_configuration, 16000000);
    BOOT_REASON_REPORT();   /* must run right after UART init */
    ADC_Init();
    GP_Timer_PWM_Init(TIMER3);
    RTC_Init(&RTC_config);
    if (!RTC_IsInitialized())
    {
        RTC_SetTime(&Time);
    }
    I2C_Init(I2C1_PORT, &i2c1_cfg);
    OLED_Init(I2C1_PORT);
    IWDG_Init(IWDG_PRE_32, IWDG_CalcReload(150, IWDG_PRE_32, 32000));
}

/*------------------------------------------------------------------
 *  GPIO_PIN_CONFIG — all pin mux for this application
 *------------------------------------------------------------------*/
void GPIO_PIN_CONFIG(void)
{
    /* On-board LED */
    GPIO_InitPin(GPIO_PORTA, PIN5,  GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);

    /* MCO1 */
    GPIO_InitPin(GPIO_PORTA, PIN8,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

    /* UART1 — PA9 TX, PA10 RX */
    GPIO_InitPin(GPIO_PORTA, PIN9,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_InitPin(GPIO_PORTA, PIN10, GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_SetAF(GPIO_PORTA, PIN9,  7);
    GPIO_SetAF(GPIO_PORTA, PIN10, 7);

    /* UART2 — PA2 TX, PA3 RX */
    GPIO_InitPin(GPIO_PORTA, PIN2,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_InitPin(GPIO_PORTA, PIN3,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_SetAF(GPIO_PORTA, PIN2, 7);
    GPIO_SetAF(GPIO_PORTA, PIN3, 7);

    /* ADC1 — PA0 (LDR), PA1 (spare) */
    GPIO_InitPin(GPIO_PORTA, PIN0,  GPIO_MODE_ANALOG, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
    GPIO_InitPin(GPIO_PORTA, PIN1,  GPIO_MODE_ANALOG, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);

    /* PWM — TIM3 CH2 on PC7 (AF2) */
    GPIO_InitPin(GPIO_PORTC, PIN7,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_SetAF(GPIO_PORTC, PIN7, 2);

    /* Bootloader trigger pin — PC3 input pull-up */
    GPIO_InitPin(GPIO_PORTC, PIN3,  GPIO_MODE_INPUT,  GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_PULL_UP);

    /* I2C1 — PB8 SCL, PB9 SDA (AF4, open-drain) */
    GPIO_InitPin(GPIO_PORTB, PIN8,  GPIO_MODE_AF,     GPIO_OTYPE_OD, GPIO_SPEED_FAST, GPIO_PULL_UP);
    GPIO_InitPin(GPIO_PORTB, PIN9,  GPIO_MODE_AF,     GPIO_OTYPE_OD, GPIO_SPEED_FAST, GPIO_PULL_UP);
    GPIO_SetAF(GPIO_PORTB, PIN8, 4);
    GPIO_SetAF(GPIO_PORTB, PIN9, 4);
}

/*------------------------------------------------------------------
 *  NVIC & callbacks
 *------------------------------------------------------------------*/
void ENABLE_NVIC_INTERRUPTS(void)
{
    NVIC_EnableInterrupt(UART1_IQ_NUM);
    NVIC_EnableInterrupt(UART2_IQ_NUM);
}

void CallBackFunctions(void)
{
    UART1_CALLBACK(UART1_ISR);
    UART2_CALLBACK(UART2_ISR);
}
