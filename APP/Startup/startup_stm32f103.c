#define NULL 0

typedef unsigned int uint32;

/* Symbols defined in linker script */
extern uint32 _estack, _etext, _sdata, _edata, _sbss, _ebss, _sidata;

/* External functions */
extern void main(void);
extern void SystemInit(void);

/*======================
 * Exception Handlers
 *======================*/
void Reset_Handler                     (void);
void NMI_Handler                       (void) __attribute__ ((alias ("Default_Handler")));
void HardFault_Handler                 (void) __attribute__ ((alias ("Default_Handler")));
void MemManage_Handler                 (void) __attribute__ ((alias ("Default_Handler")));
void BusFault_Handler                  (void) __attribute__ ((alias ("Default_Handler")));
void UsageFault_Handler                (void) __attribute__ ((alias ("Default_Handler")));
void SVC_Handler                       (void) __attribute__ ((alias ("Default_Handler")));
void DebugMon_Handler                  (void) __attribute__ ((alias ("Default_Handler")));
void PendSV_Handler                    (void) __attribute__ ((alias ("Default_Handler")));
void SysTick_Handler                   (void);

/*======================
 * Peripheral IRQs (STM32F446xx)
 *======================*/
void WWDG_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));                                        
void PVD_IRQHandler                    (void) __attribute__ ((alias ("Default_Handler")));
void TAMP_STAMP_IRQHandler             (void) __attribute__ ((alias ("Default_Handler")));
void RTC_WKUP_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void FLASH_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void RCC_IRQHandler                    (void) __attribute__ ((alias ("Default_Handler")));
void EXTI0_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void EXTI1_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void EXTI2_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void EXTI3_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void EXTI4_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream0_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream1_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream2_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream3_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream4_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream5_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream6_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void ADC_IRQHandler                    (void) __attribute__ ((alias ("Default_Handler")));
void CAN1_TX_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void CAN1_RX0_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void CAN1_RX1_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void CAN1_SCE_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void EXTI9_5_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void TIM1_BRK_TIM9_IRQHandler          (void) __attribute__ ((alias ("Default_Handler")));
void TIM1_UP_TIM10_IRQHandler          (void) __attribute__ ((alias ("Default_Handler")));
void TIM1_TRG_COM_TIM11_IRQHandler     (void) __attribute__ ((alias ("Default_Handler")));
void TIM1_CC_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void TIM2_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void TIM3_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void TIM4_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void I2C1_EV_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void I2C1_ER_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void I2C2_EV_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void I2C2_ER_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void SPI1_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void SPI2_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void USART1_IRQHandler                 (void) __attribute__ ((alias ("Default_Handler")));
void USART2_IRQHandler                 (void) __attribute__ ((alias ("Default_Handler")));
void USART3_IRQHandler                 (void) __attribute__ ((alias ("Default_Handler")));
void EXTI15_10_IRQHandler              (void) __attribute__ ((alias ("Default_Handler")));
void RTC_Alarm_IRQHandler              (void) __attribute__ ((alias ("Default_Handler")));
void OTG_FS_WKUP_IRQHandler            (void) __attribute__ ((alias ("Default_Handler")));
void TIM8_BRK_TIM12_IRQHandler         (void) __attribute__ ((alias ("Default_Handler")));
void TIM8_UP_TIM13_IRQHandler          (void) __attribute__ ((alias ("Default_Handler")));
void TIM8_TRG_COM_TIM14_IRQHandler     (void) __attribute__ ((alias ("Default_Handler")));
void TIM8_CC_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void DMA1_Stream7_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void FMC_IRQHandler                    (void) __attribute__ ((alias ("Default_Handler")));
void SDIO_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void TIM5_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void SPI3_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void UART4_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void UART5_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));
void TIM6_DAC_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void TIM7_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream0_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream1_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream2_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream3_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream4_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void CAN2_TX_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void CAN2_RX0_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void CAN2_RX1_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void CAN2_SCE_IRQHandler               (void) __attribute__ ((alias ("Default_Handler")));
void OTG_FS_IRQHandler                 (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream5_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream6_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void DMA2_Stream7_IRQHandler           (void) __attribute__ ((alias ("Default_Handler")));
void USART6_IRQHandler                 (void) __attribute__ ((alias ("Default_Handler")));
void I2C3_EV_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void I2C3_ER_IRQHandler                (void) __attribute__ ((alias ("Default_Handler")));
void FPU_IRQHandler                    (void) __attribute__ ((alias ("Default_Handler")));
void SPI4_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void SPI5_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void SPI6_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void SAI1_IRQHandler                   (void) __attribute__ ((alias ("Default_Handler")));
void DMA2D_IRQHandler                  (void) __attribute__ ((alias ("Default_Handler")));

/*======================
 * Vector Table
 *======================*/
uint32 * const MSP_Value = (uint32 *)&_estack;

uint32 *Vector_Table[] __attribute__ ((section(".isr_vector"))) =
{
    (uint32 *)MSP_Value,                    /* Main Stack Pointer */
    (uint32 *)Reset_Handler,                /* Reset Handler */
    (uint32 *)NMI_Handler,                 
    (uint32 *)HardFault_Handler,
    (uint32 *)MemManage_Handler,
    (uint32 *)BusFault_Handler,
    (uint32 *)UsageFault_Handler,
    0, 0, 0, 0,
    (uint32 *)SVC_Handler,
    (uint32 *)DebugMon_Handler,
    0,
    (uint32 *)PendSV_Handler,
    (uint32 *)SysTick_Handler,

    /* External Interrupts (up to 91 for STM32F446) */
    (uint32 *)WWDG_IRQHandler,
    (uint32 *)PVD_IRQHandler,
    (uint32 *)TAMP_STAMP_IRQHandler,
    (uint32 *)RTC_WKUP_IRQHandler,
    (uint32 *)FLASH_IRQHandler,
    (uint32 *)RCC_IRQHandler,
    (uint32 *)EXTI0_IRQHandler,
    (uint32 *)EXTI1_IRQHandler,
    (uint32 *)EXTI2_IRQHandler,
    (uint32 *)EXTI3_IRQHandler,
    (uint32 *)EXTI4_IRQHandler,
    (uint32 *)DMA1_Stream0_IRQHandler,
    (uint32 *)DMA1_Stream1_IRQHandler,
    (uint32 *)DMA1_Stream2_IRQHandler,
    (uint32 *)DMA1_Stream3_IRQHandler,
    (uint32 *)DMA1_Stream4_IRQHandler,
    (uint32 *)DMA1_Stream5_IRQHandler,
    (uint32 *)DMA1_Stream6_IRQHandler,
    (uint32 *)ADC_IRQHandler,
    (uint32 *)CAN1_TX_IRQHandler,
    (uint32 *)CAN1_RX0_IRQHandler,
    (uint32 *)CAN1_RX1_IRQHandler,
    (uint32 *)CAN1_SCE_IRQHandler,
    (uint32 *)EXTI9_5_IRQHandler,
    (uint32 *)TIM1_BRK_TIM9_IRQHandler,
    (uint32 *)TIM1_UP_TIM10_IRQHandler,
    (uint32 *)TIM1_TRG_COM_TIM11_IRQHandler,
    (uint32 *)TIM1_CC_IRQHandler,
    (uint32 *)TIM2_IRQHandler,
    (uint32 *)TIM3_IRQHandler,
    (uint32 *)TIM4_IRQHandler,
    (uint32 *)I2C1_EV_IRQHandler,
    (uint32 *)I2C1_ER_IRQHandler,
    (uint32 *)I2C2_EV_IRQHandler,
    (uint32 *)I2C2_ER_IRQHandler,
    (uint32 *)SPI1_IRQHandler,
    (uint32 *)SPI2_IRQHandler,
    (uint32 *)USART1_IRQHandler,
    (uint32 *)USART2_IRQHandler,
    (uint32 *)USART3_IRQHandler,
    (uint32 *)EXTI15_10_IRQHandler,
    (uint32 *)RTC_Alarm_IRQHandler,
    (uint32 *)OTG_FS_WKUP_IRQHandler,
    (uint32 *)TIM8_BRK_TIM12_IRQHandler,
    (uint32 *)TIM8_UP_TIM13_IRQHandler,
    (uint32 *)TIM8_TRG_COM_TIM14_IRQHandler,
    (uint32 *)TIM8_CC_IRQHandler,
    (uint32 *)DMA1_Stream7_IRQHandler,
    (uint32 *)FMC_IRQHandler,
    (uint32 *)SDIO_IRQHandler,
    (uint32 *)TIM5_IRQHandler,
    (uint32 *)SPI3_IRQHandler,
    (uint32 *)UART4_IRQHandler,
    (uint32 *)UART5_IRQHandler,
    (uint32 *)TIM6_DAC_IRQHandler,
    (uint32 *)TIM7_IRQHandler,
    (uint32 *)DMA2_Stream0_IRQHandler,
    (uint32 *)DMA2_Stream1_IRQHandler,
    (uint32 *)DMA2_Stream2_IRQHandler,
    (uint32 *)DMA2_Stream3_IRQHandler,
    (uint32 *)DMA2_Stream4_IRQHandler,
    (uint32 *)CAN2_TX_IRQHandler,
    (uint32 *)CAN2_RX0_IRQHandler,
    (uint32 *)CAN2_RX1_IRQHandler,
    (uint32 *)CAN2_SCE_IRQHandler,
    (uint32 *)OTG_FS_IRQHandler,
    (uint32 *)DMA2_Stream5_IRQHandler,
    (uint32 *)DMA2_Stream6_IRQHandler,
    (uint32 *)DMA2_Stream7_IRQHandler,
    (uint32 *)USART6_IRQHandler,
    (uint32 *)I2C3_EV_IRQHandler,
    (uint32 *)I2C3_ER_IRQHandler,
    (uint32 *)FPU_IRQHandler,
    (uint32 *)SPI4_IRQHandler,
    (uint32 *)SPI5_IRQHandler,
    (uint32 *)SPI6_IRQHandler,
    (uint32 *)SAI1_IRQHandler,
    (uint32 *)DMA2D_IRQHandler
};

/*======================
 * Reset Handler
 *======================*/
void Reset_Handler(void)
{
    uint32 Section_Size = NULL;
    uint32 *MemSourceAddr = NULL;
    uint32 *MemDestAddr = NULL;

    /* 1) Copy initialized data from Flash to SRAM */
    Section_Size = &_edata - &_sdata;
    MemSourceAddr = (uint32 *)&_sidata;
    MemDestAddr = (uint32 *)&_sdata;

    for(uint32 i = 0; i < Section_Size; i++)
        *MemDestAddr++ = *MemSourceAddr++;

    /* 2) Zero initialize the .bss section */
    Section_Size = &_ebss - &_sbss;
    MemDestAddr = (uint32 *)&_sbss;

    for(uint32 i = 0; i < Section_Size; i++)
        *MemDestAddr++ = 0;

    /* 3) Initialize system */
    SystemInit();

    /* 4) Jump to main */
    main();

    /* If main() returns */
    while(1);
}

/*======================
 * Default Handler
 *======================*/
void Default_Handler(void)
{
    while(1);
}
