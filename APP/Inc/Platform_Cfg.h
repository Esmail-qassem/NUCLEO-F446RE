#ifndef PLATFORM_CFG_H_
#define PLATFORM_CFG_H_

/*==================================================================
 *  Platform_Cfg.h
 *  Board / SoC constants shared across the APP layer.
 *  STM32F446RE on NUCLEO-F446RE.
 *================================================================*/

/*------------------------------------------------------------------
 *  System clock
 *------------------------------------------------------------------*/
#define SYS_CLOCK_HZ            (16000000UL)    /* HSI, no PLL */

/*------------------------------------------------------------------
 *  ADC characteristics (12-bit, VREF = 3.3 V)
 *------------------------------------------------------------------*/
#define ADC_VREF_MV             (3300UL)
#define ADC_FULL_SCALE          (4095UL)

/*------------------------------------------------------------------
 *  Internal temperature sensor (RM0390 §13.10)
 *  V25     = 0.76 V  (typical)
 *  AvgSlope = 2.5 mV/°C  (stored ×10 to avoid float)
 *------------------------------------------------------------------*/
#define TEMP_V25_MV             (760L)
#define TEMP_AVG_SLOPE_X10      (25L)
#define TEMP_OFFSET_C           (25L)

/*------------------------------------------------------------------
 *  Cortex-M4 System Control Block
 *------------------------------------------------------------------*/
#define SCB_AIRCR_ADDR          (0xE000ED0CUL)
#define SCB_AIRCR_VECTKEY       (0x5FAUL)
#define SCB_AIRCR_VECTKEY_POS   (16U)
#define SCB_AIRCR_SYSRESETREQ   (1UL << 2U)

/*------------------------------------------------------------------
 *  RCC reset-cause register (RCC_CSR)
 *------------------------------------------------------------------*/
#define RCC_CSR_ADDR            (0x40023874UL)
#define RCC_CSR_RMVF            (1UL << 24U)
#define RCC_CSR_PINRSTF         (1UL << 26U)
#define RCC_CSR_PORRSTF         (1UL << 27U)
#define RCC_CSR_SFTRSTF         (1UL << 28U)
#define RCC_CSR_IWDGRSTF        (1UL << 29U)
#define RCC_CSR_WWDGRSTF        (1UL << 30U)

/*------------------------------------------------------------------
 *  GPIO Alternate-Function numbers not provided by MCAL headers
 *------------------------------------------------------------------*/
#define GPIO_AF2_TIM3           (2U)
#define GPIO_AF4_I2C1           (4U)

/*------------------------------------------------------------------
 *  Board pin map
 *------------------------------------------------------------------*/
#define LED_PORT                GPIO_PORTA
#define LED_PIN                 PIN5

#define BTLD_TRIG_PORT          GPIO_PORTC
#define BTLD_TRIG_PIN           PIN3

#define PWM_TIMER               TIMER3
#define PWM_CHANNEL             (2U)

#endif /* PLATFORM_CFG_H_ */
