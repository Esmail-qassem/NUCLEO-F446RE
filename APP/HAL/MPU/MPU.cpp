/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : MPU HAL (MPU6050)                                      */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "MPU.hpp"
#include "I2C.hpp"
#include "App_Config.h"

/* ── Recover ──────────────────────────────────────────────────────── */
void MPU::Recover(void)
{
    I2C::Init(I2C_Port::I2C1, i2c1_cfg);
    MPU::Init();
}

/* ── Init ─────────────────────────────────────────────────────────── */
void MPU::Init(void)
{
    I2C::WriteRegister(I2C_Port::I2C1, MPU_ADDR, REG_PWR, 0U);
}

/* ── GetTemp ──────────────────────────────────────────────────────── */
sint16 MPU::GetTemp(void)
{
    uint8 TempArray[2];
    I2C_Status status = I2C::ReadRegisters(I2C_Port::I2C1, MPU_ADDR, REG_TEMP_H, TempArray, 2U);
    if (status != I2C_Status::OK)
    {
        Recover();
        return 0;
    }
    sint16 raw    = static_cast<sint16>(static_cast<uint16>(TempArray[0]) << 8U | TempArray[1]);
    sint32 temp_c = (static_cast<sint32>(raw) * 100) / 340 + 3653;
    return static_cast<sint16>(temp_c);
}

/* ── GetAccelerometer ─────────────────────────────────────────────── */
void MPU::GetAccelerometer(ACCEL_t &accel)
{
    uint8 AccelArray[6] = {0};
    I2C_Status status = I2C::ReadRegisters(I2C_Port::I2C1, MPU_ADDR, ACCEL_XOUT_H, AccelArray, 6U);
    if (status != I2C_Status::OK)
    {
        Recover();
        return;
    }
    sint16 rx = static_cast<sint16>(static_cast<uint16>(AccelArray[0]) << 8U | AccelArray[1]);
    sint16 ry = static_cast<sint16>(static_cast<uint16>(AccelArray[2]) << 8U | AccelArray[3]);
    sint16 rz = static_cast<sint16>(static_cast<uint16>(AccelArray[4]) << 8U | AccelArray[5]);
    accel.Accel_X = static_cast<sint16>((static_cast<sint32>(rx) * 100) / 16384);
    accel.Accel_Y = static_cast<sint16>((static_cast<sint32>(ry) * 100) / 16384);
    accel.Accel_Z = static_cast<sint16>((static_cast<sint32>(rz) * 100) / 16384);
}

/* ── GetGyroscope ─────────────────────────────────────────────────── */
void MPU::GetGyroscope(GYRO_t &gyro)
{
    uint8 GyroArray[6] = {0};
    I2C_Status status = I2C::ReadRegisters(I2C_Port::I2C1, MPU_ADDR, GYRO_XOUT_H, GyroArray, 6U);
    if (status != I2C_Status::OK)
    {
        Recover();
        return;
    }
    sint16 rx = static_cast<sint16>(static_cast<uint16>(GyroArray[0]) << 8U | GyroArray[1]);
    sint16 ry = static_cast<sint16>(static_cast<uint16>(GyroArray[2]) << 8U | GyroArray[3]);
    sint16 rz = static_cast<sint16>(static_cast<uint16>(GyroArray[4]) << 8U | GyroArray[5]);
    gyro.GYRO_X = static_cast<sint16>((static_cast<sint32>(rx) * 100) / 131);
    gyro.GYRO_Y = static_cast<sint16>((static_cast<sint32>(ry) * 100) / 131);
    gyro.GYRO_Z = static_cast<sint16>((static_cast<sint32>(rz) * 100) / 131);
}

/* ── Calibrate ────────────────────────────────────────────────────── */
void MPU::Calibrate(ACCEL_t &biasAccel, GYRO_t &biasGyro)
{
    sint32 accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    sint32 gyro_x_sum  = 0, gyro_y_sum  = 0, gyro_z_sum  = 0;
    constexpr uint16 size = 5000U;

    for (uint16 i = 0U; i < size; i++)
    {
        MPU::GetAccelerometer(biasAccel);
        MPU::GetGyroscope(biasGyro);
        accel_x_sum += biasAccel.Accel_X;
        accel_y_sum += biasAccel.Accel_Y;
        accel_z_sum += biasAccel.Accel_Z;
        gyro_x_sum  += biasGyro.GYRO_X;
        gyro_y_sum  += biasGyro.GYRO_Y;
        gyro_z_sum  += biasGyro.GYRO_Z;
    }
    biasAccel.Accel_X = static_cast<sint16>(accel_x_sum / size);
    biasAccel.Accel_Y = static_cast<sint16>(accel_y_sum / size);
    biasAccel.Accel_Z = static_cast<sint16>(accel_z_sum / size);
    biasGyro.GYRO_X   = static_cast<sint16>(gyro_x_sum  / size);
    biasGyro.GYRO_Y   = static_cast<sint16>(gyro_y_sum  / size);
    biasGyro.GYRO_Z   = static_cast<sint16>(gyro_z_sum  / size);
}
