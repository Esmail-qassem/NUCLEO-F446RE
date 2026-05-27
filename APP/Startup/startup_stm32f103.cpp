/* startup_stm32f103.cpp — STM32F446RE vector table and reset handler */
#include "STD_TYPES.h"

using isr_t = void(*)(void);

/* Linker-script symbols */
extern "C" {
    extern uint32 _estack, _etext, _sdata, _edata, _sbss, _ebss, _sidata;

    void Reset_Handler (void);
    void Default_Handler(void);

    void NMI_Handler       (void) __attribute__((alias("Default_Handler")));
    void HardFault_Handler (void);
    void MemManage_Handler (void);
    void BusFault_Handler  (void) __attribute__((alias("Default_Handler")));
    void UsageFault_Handler(void) __attribute__((alias("Default_Handler")));
    void SVC_Handler       (void);
    void DebugMon_Handler  (void) __attribute__((alias("Default_Handler")));
    void PendSV_Handler    (void);
    void SysTick_Handler   (void);

    void WWDG_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void PVD_IRQHandler                (void) __attribute__((alias("Default_Handler")));
    void TAMP_STAMP_IRQHandler         (void) __attribute__((alias("Default_Handler")));
    void RTC_WKUP_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void FLASH_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void RCC_IRQHandler                (void) __attribute__((alias("Default_Handler")));
    void EXTI0_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void EXTI1_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void EXTI2_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void EXTI3_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void EXTI4_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream0_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream1_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream2_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream3_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream4_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream5_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream6_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void ADC_IRQHandler                (void) __attribute__((alias("Default_Handler")));
    void CAN1_TX_IRQHandler            (void) __attribute__((alias("Default_Handler")));
    void CAN1_RX0_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void CAN1_RX1_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void CAN1_SCE_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void EXTI9_5_IRQHandler            (void) __attribute__((alias("Default_Handler")));
    void TIM1_BRK_TIM9_IRQHandler      (void) __attribute__((alias("Default_Handler")));
    void TIM1_UP_TIM10_IRQHandler      (void) __attribute__((alias("Default_Handler")));
    void TIM1_TRG_COM_TIM11_IRQHandler (void) __attribute__((alias("Default_Handler")));
    void TIM1_CC_IRQHandler            (void) __attribute__((alias("Default_Handler")));
    void TIM2_IRQHandler               (void);
    void TIM3_IRQHandler               (void);
    void TIM4_IRQHandler               (void);
    void I2C1_EV_IRQHandler            (void);
    void I2C1_ER_IRQHandler            (void);
    void I2C2_EV_IRQHandler            (void);
    void I2C2_ER_IRQHandler            (void);
    void SPI1_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void SPI2_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void USART1_IRQHandler             (void);
    void USART2_IRQHandler             (void);
    void USART3_IRQHandler             (void);
    void EXTI15_10_IRQHandler          (void) __attribute__((alias("Default_Handler")));
    void RTC_Alarm_IRQHandler          (void) __attribute__((alias("Default_Handler")));
    void OTG_FS_WKUP_IRQHandler        (void) __attribute__((alias("Default_Handler")));
    void TIM8_BRK_TIM12_IRQHandler     (void) __attribute__((alias("Default_Handler")));
    void TIM8_UP_TIM13_IRQHandler      (void) __attribute__((alias("Default_Handler")));
    void TIM8_TRG_COM_TIM14_IRQHandler (void) __attribute__((alias("Default_Handler")));
    void TIM8_CC_IRQHandler            (void) __attribute__((alias("Default_Handler")));
    void DMA1_Stream7_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void FMC_IRQHandler                (void) __attribute__((alias("Default_Handler")));
    void SDIO_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void TIM5_IRQHandler               (void);
    void SPI3_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void UART4_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void UART5_IRQHandler              (void) __attribute__((alias("Default_Handler")));
    void TIM6_DAC_IRQHandler           (void);
    void TIM7_IRQHandler               (void);
    void DMA2_Stream0_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream1_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream2_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream3_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream4_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void CAN2_TX_IRQHandler            (void) __attribute__((alias("Default_Handler")));
    void CAN2_RX0_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void CAN2_RX1_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void CAN2_SCE_IRQHandler           (void) __attribute__((alias("Default_Handler")));
    void OTG_FS_IRQHandler             (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream5_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream6_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void DMA2_Stream7_IRQHandler       (void) __attribute__((alias("Default_Handler")));
    void USART6_IRQHandler             (void) __attribute__((alias("Default_Handler")));
    void I2C3_EV_IRQHandler            (void);
    void I2C3_ER_IRQHandler            (void);
    void FPU_IRQHandler                (void) __attribute__((alias("Default_Handler")));
    void SPI4_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void SPI5_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void SPI6_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void SAI1_IRQHandler               (void) __attribute__((alias("Default_Handler")));
    void DMA2D_IRQHandler              (void) __attribute__((alias("Default_Handler")));
} /* extern "C" */

/* ── Vector Table ────────────────────────────────────────────────────── */
isr_t Vector_Table[] __attribute__((section(".isr_vector"))) =
{
    reinterpret_cast<isr_t>(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    nullptr, nullptr, nullptr, nullptr,
    SVC_Handler,
    DebugMon_Handler,
    nullptr,
    PendSV_Handler,
    SysTick_Handler,
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FMC_IRQHandler,
    SDIO_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    FPU_IRQHandler,
    SPI4_IRQHandler,
    SPI5_IRQHandler,
    SPI6_IRQHandler,
    SAI1_IRQHandler,
    DMA2D_IRQHandler
};

/* ── Reset Handler ───────────────────────────────────────────────────── */
extern "C" void Reset_Handler(void)
{
    uint32 *src = &_sidata;
    uint32 *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;

    dst = &_sbss;
    while (dst < &_ebss) *dst++ = 0u;

    extern void SystemInit(void);
    SystemInit();

    extern int main(void);
    main();

    while (1) {}
}

extern "C" void Default_Handler(void)
{
    while (1) {}
}
