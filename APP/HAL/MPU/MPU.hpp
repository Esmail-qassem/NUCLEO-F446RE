/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : MPU HAL (MPU6050)                                      */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── MPU6050 register addresses ───────────────────────────────────── */
constexpr uint8 MPU_ADDR     = 0x68U;
constexpr uint8 REG_PWR      = 0x6BU;
constexpr uint8 REG_WHO      = 0x75U;
constexpr uint8 REG_TEMP_H   = 0x41U;
constexpr uint8 GYRO_XOUT_H  = 0x43U;
constexpr uint8 GYRO_YOUT_H  = 0x45U;
constexpr uint8 GYRO_ZOUT_H  = 0x47U;
constexpr uint8 ACCEL_XOUT_H = 0x3BU;
constexpr uint8 ACCEL_YOUT_H = 0x3DU;
constexpr uint8 ACCEL_ZOUT_H = 0x3FU;

/* ── Data structures ──────────────────────────────────────────────── */
struct ACCEL_t
{
    sint16 Accel_X;
    sint16 Accel_Y;
    sint16 Accel_Z;
};

struct GYRO_t
{
    sint16 GYRO_X;
    sint16 GYRO_Y;
    sint16 GYRO_Z;
};

/* ── MPU Driver Class ─────────────────────────────────────────────── */
class MPU
{
public:
    static void   Init             (void);
    static void   GetAccelerometer (ACCEL_t &accel);
    static void   GetGyroscope     (GYRO_t &gyro);
    static sint16 GetTemp          (void);
    static void   Calibrate        (ACCEL_t &biasAccel, GYRO_t &biasGyro);

private:
    static void Recover(void);
    MPU() = delete;
};
