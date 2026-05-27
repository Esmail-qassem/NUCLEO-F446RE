#include "SwM.hpp"
#include "Telemetry.hpp"
#include "App_Ctrl.hpp"
#include "OLED.h"
#include "IWDG.h"
#include "MPU.h"
#include "UART.hpp"
#include "App_Config.hpp"

ACCEL_t accel_data;
GYRO_t  gyro_data;
extern ACCEL_t Bias_accel_data;
extern GYRO_t  Bias_gyro_data;

void OS_5ms_Task(void) {}

void OS_10ms_Task(void)
{
    LED();
}

void OS_20ms_Task(void)
{
    MPU_GetAccelerometer(&accel_data);
    MPU_GetGyroscope(&gyro_data);

    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("$DATA:"), 6u);
    UART::SendNumber(UART_HardWare::UART2, accel_data.Accel_X);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(","), 1u);
    UART::SendNumber(UART_HardWare::UART2, accel_data.Accel_Y);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(","), 1u);
    UART::SendNumber(UART_HardWare::UART2, accel_data.Accel_Z);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(","), 1u);
    UART::SendNumber(UART_HardWare::UART2, gyro_data.GYRO_X - Bias_gyro_data.GYRO_X);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(","), 1u);
    UART::SendNumber(UART_HardWare::UART2, gyro_data.GYRO_Y - Bias_gyro_data.GYRO_Y);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(","), 1u);
    UART::SendNumber(UART_HardWare::UART2, gyro_data.GYRO_Z - Bias_gyro_data.GYRO_Z);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("\r\n"), 2u);

    uint8 pixel_x = static_cast<uint8>(64u - ((accel_data.Accel_Y - Bias_accel_data.Accel_Y) * 64) / 100);
    uint8 pixel_y = static_cast<uint8>(32u - ((accel_data.Accel_X - Bias_accel_data.Accel_X) * 32) / 100);
    OLED_Clear();
    OLED_DrawPixel(pixel_x, pixel_y, OLED_COLOR_WHITE);
    OLED_UpdateScreen(I2C1_PORT);
}

void OS_50ms_Task(void) {}

void OS_100ms_Task(void)
{
    IWDG_Refresh();
}

void OS_1000ms_Task(void)
{
    static uint8 tick = 0u;
    tick++;
    LifeCounter();
    if (tick % 5u  == 0u) INTERNAL_TEMP_TASK();
    if (tick % 10u == 0u) RUN_TIME();
    if (tick % 30u == 0u) STACK_MONITOR();
    if (tick == 1u)       SW_VERSION();

    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("=== MPU-6050 ===\r\n"), 18u);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("Temp  : "), 8u);
    UART::SendNumber(UART_HardWare::UART2, MPU_GetTemp());
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(" (x100 C)\r\n"), 11u);
}

void OS_IDLE_TASK(void)
{
    __asm("NOP");
}
