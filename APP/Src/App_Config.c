#include "App_Config.h"
#include "App_Config_Cfg.h"
#include "Platform_Cfg.h"
#include "RCC.h"
#include "GPIO_interface.h"
#include "NVIC_interface.h"
#include "ADC.h"
#include "GP_Timer.h"
#include "IWDG.h"
#include "I2C.h"
#include "OLED.h"
#include "Telemetry.h"
#include "ISR.h"
#include "MPU.h"
#include "FPU.h"
#include "UART.h"
#include "RTC.h"
#include "FLAPPY_Bird.h"
#include "FLAPPY_Bird_Cfg.h"

/*------------------------------------------------------------------
 *  Firmware version — single definition, shared via App_Config.h
 *------------------------------------------------------------------*/
const uint8 FIRMWARE_VERSION[] = FIRMWARE_VERSION_STR;

/*------------------------------------------------------------------
 *  Sensor calibration bias (filled in APP_Init, consumed by SwM)
 *------------------------------------------------------------------*/
ACCEL_t Bias_accel_data;
GYRO_t  Bias_gyro_data;

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static void GPIO_PinConfig(void);
static void NVIC_EnableInterrupts(void);
static void RegisterCallbacks(void);
static void MPU_SettleDelay(void);

/*------------------------------------------------------------------
 *  Clock-tree configuration
 *------------------------------------------------------------------*/
static const RCC_Config_t RCC_Configuration =
{
    RCC_CLK_HSI,
    { 0U, 0U, 0U, 0U, 0U },
    AHB_PRE_1,
    APB_PRE_1,
    APB_PRE_1
};

/*------------------------------------------------------------------
 *  Peripheral configuration structs
 *------------------------------------------------------------------*/
static UART_Config_t Uart1_configuration =
{
    UART_BAUDRATE,
    UART_MODE_TX_RX,
    UART_PARITY_NONE,
    UART_STOPBITS_1,
    UART_WORDLEN_8B,
    Interrupt
};

static UART_Config_t Uart2_configuration =
{
    UART_BAUDRATE,
    UART_MODE_TX_RX,
    UART_PARITY_NONE,
    UART_STOPBITS_1,
    UART_WORDLEN_8B,
    Interrupt
};

static I2C_Config_t i2c1_cfg =
{
    .PCLK1_Hz        = SYS_CLOCK_HZ,
    .ClockSpeed      = I2C_CLOCK_HZ,
    .DutyCycle       = I2C_DUTY_2,
    .AddressingMode  = I2C_ADDR_7BIT,
    .OwnAddress      = I2C_OWN_ADDRESS,
    .Acknowledgement = 1U,
    .TransferMode    = I2C_POLLING,
};

static const RTC_Config_t RTC_config =
{
    RTC_CLK_LSI,
    RTC_PREDIV_A,
    RTC_PREDIV_S,
    RTC_HOURFORMAT_24
};

static const RTC_Time_t Time = { 0U, 0U, 0U, RTC_AM };

/*==================================================================
 *  APP_ClockInit
 *  Called from SystemInit() (startup, before main).
 *================================================================*/
void APP_ClockInit(void)
{
    RCC_Init(&RCC_Configuration);
    RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
    RCC_EnableClock(RCC_AHB1, AHB1_GPIOB);
    RCC_EnableClock(RCC_AHB1, AHB1_GPIOC);
    RCC_EnableClock(RCC_APB1, APB1_USART2);
    RCC_EnableClock(RCC_APB2, APB2_USART1);
    RCC_EnableClock(RCC_APB2, APB2_ADC1);
    RCC_EnableClock(RCC_APB1, APB1_PWR);
    RCC_EnableClock(RCC_APB1, APB1_WWDG);
    RCC_EnableClock(RCC_AHB1, AHB1_CRC);
    RCC_EnableClock(RCC_APB1, APB1_I2C1);
}

/*==================================================================
 *  APP_Init
 *  Full peripheral bring-up sequence. Called from main().
 *================================================================*/
void APP_Init(void)
{
    GPIO_PinConfig();
    NVIC_EnableInterrupts();
    RegisterCallbacks();

    UART_Init(UART1, &Uart1_configuration, SYS_CLOCK_HZ);
    UART_Init(UART2, &Uart2_configuration, SYS_CLOCK_HZ);
    BOOT_REASON_REPORT();           /* must run right after UART init */

    ADC_Init();
    GP_Timer_PWM_Init(PWM_TIMER);

    RTC_Init(&RTC_config);
    if (RTC_IsInitialized() == 0U)
    {
        RTC_SetTime(&Time);
    }

    I2C_Init(I2C1_PORT, &i2c1_cfg);
    OLED_Init(I2C1_PORT);

    MPU_Init();
    MPU_SettleDelay();
    MPU_Calibrate(&Bias_accel_data, &Bias_gyro_data);

    Game_Init();

    IWDG_Init(IWDG_PRE_32,
              IWDG_CalcReload(IWDG_TIMEOUT_MS, IWDG_PRE_32, IWDG_LSI_HZ));
}

/*==================================================================
 *  Local helpers
 *================================================================*/

/*------------------------------------------------------------------
 *  GPIO_PinConfig — all pin mux for this application
 *------------------------------------------------------------------*/
static void GPIO_PinConfig(void)
{
    /* On-board LED */
    GPIO_InitPin(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);

    /* MCO1 */
    GPIO_InitPin(GPIO_PORTA, PIN8,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

    /* UART1 — PA9 TX, PA10 RX */
    GPIO_InitPin(GPIO_PORTA, PIN9,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_InitPin(GPIO_PORTA, PIN10, GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_SetAF  (GPIO_PORTA, PIN9,  AF7_USART1);
    GPIO_SetAF  (GPIO_PORTA, PIN10, AF7_USART1);

    /* UART2 — PA2 TX, PA3 RX */
    GPIO_InitPin(GPIO_PORTA, PIN2,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_InitPin(GPIO_PORTA, PIN3,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_SetAF  (GPIO_PORTA, PIN2,  AF7_USART2);
    GPIO_SetAF  (GPIO_PORTA, PIN3,  AF7_USART2);

    /* ADC1 — PA0 (LDR), PA1 (spare) */
    GPIO_InitPin(GPIO_PORTA, PIN0,  GPIO_MODE_ANALOG, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
    GPIO_InitPin(GPIO_PORTA, PIN1,  GPIO_MODE_ANALOG, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);

    /* PWM — TIM3 CH2 on PC7 (AF2) */
    GPIO_InitPin(GPIO_PORTC, PIN7,  GPIO_MODE_AF,     GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
    GPIO_SetAF  (GPIO_PORTC, PIN7,  GPIO_AF2_TIM3);

    /* Bootloader trigger pin — input pull-up */
    GPIO_InitPin(BTLD_TRIG_PORT, BTLD_TRIG_PIN, GPIO_MODE_INPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_PULL_UP);

    /* Flappy-Bird buttons — input pull-up, active low */
    GPIO_InitPin(GAME_BTN_JUMP_PORT,  GAME_BTN_JUMP_PIN,  GPIO_MODE_INPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_PULL_UP);
    GPIO_InitPin(GAME_BTN_RESET_PORT, GAME_BTN_RESET_PIN, GPIO_MODE_INPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_PULL_UP);

    /* I2C1 — PB8 SCL, PB9 SDA (AF4, open-drain) */
    GPIO_InitPin(GPIO_PORTB, PIN8,  GPIO_MODE_AF,     GPIO_OTYPE_OD, GPIO_SPEED_FAST, GPIO_PULL_UP);
    GPIO_InitPin(GPIO_PORTB, PIN9,  GPIO_MODE_AF,     GPIO_OTYPE_OD, GPIO_SPEED_FAST, GPIO_PULL_UP);
    GPIO_SetAF  (GPIO_PORTB, PIN8,  GPIO_AF4_I2C1);
    GPIO_SetAF  (GPIO_PORTB, PIN9,  GPIO_AF4_I2C1);
}

/*------------------------------------------------------------------
 *  NVIC & callbacks
 *------------------------------------------------------------------*/
static void NVIC_EnableInterrupts(void)
{
    NVIC_EnableInterrupt(UART1_IQ_NUM);
    NVIC_EnableInterrupt(UART2_IQ_NUM);
}

static void RegisterCallbacks(void)
{
    UART1_CALLBACK(UART1_ISR);
    UART2_CALLBACK(UART2_ISR);
}

/*------------------------------------------------------------------
 *  MPU_SettleDelay — ~50 ms busy-wait for sensor power-up
 *------------------------------------------------------------------*/
static void MPU_SettleDelay(void)
{
    uint16 i;
    uint16 j;

    for (i = 0U; i < MPU_SETTLE_DELAY_OUTER; i++)
    {
        for (j = 0U; j < MPU_SETTLE_DELAY_INNER; j++)
        {
            __asm("NOP");
        }
    }
}
