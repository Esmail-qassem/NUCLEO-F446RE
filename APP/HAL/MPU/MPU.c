#include "MPU.h"
#include "I2C.h"
#include "App_Config.h"

static void MPU_Recover(void)
{
    I2C_Init(I2C1_PORT, &i2c1_cfg);  /* reset stuck I2C peripheral */
    MPU_Init();                        /* wake MPU after peripheral reset */
}

void MPU_Init(void)
{
    I2C_WriteRegister(I2C1_PORT, MPU_ADDR, REG_PWR, 0);
}
sint16 MPU_GetTemp(void)
{
    uint8 TempArray[2];
    I2C_Status_t Status = I2C_ReadRegisters(I2C1_PORT,MPU_ADDR,REG_TEMP_H, TempArray,2);
    if(Status != I2C_OK)
    {
        MPU_Recover();
        return 0;
    }
    sint16 raw     = (sint16)((uint16)TempArray[0] << 8 | TempArray[1]);
    sint32 temp_c  = ((sint32)raw * 100) / 340 + 3653;  /* °C × 100 */
    return (sint16)temp_c;
}


void MPU_GetAccelerometer(ACCEL_t* accel)
{
    uint8 AccelArray[6] = {0};
    I2C_Status_t Status = I2C_ReadRegisters(I2C1_PORT, MPU_ADDR, ACCEL_XOUT_H, AccelArray, 6);
    if(Status != I2C_OK)
    {
        MPU_Recover();
        return;
    }
    sint16 rx = (sint16)((uint16)AccelArray[0] << 8 | AccelArray[1]);
    sint16 ry = (sint16)((uint16)AccelArray[2] << 8 | AccelArray[3]);
    sint16 rz = (sint16)((uint16)AccelArray[4] << 8 | AccelArray[5]);
    accel->Accel_X = (sint16)(((sint32)rx * 100) / 16384); /* g × 100 */
    accel->Accel_Y = (sint16)(((sint32)ry * 100) / 16384);
    accel->Accel_Z = (sint16)(((sint32)rz * 100) / 16384);
}



void MPU_GetGyroscope(GYRO_t* gyro)
{
    uint8 GyroArray[6] = {0};
    I2C_Status_t Status = I2C_ReadRegisters(I2C1_PORT, MPU_ADDR, GYRO_XOUT_H, GyroArray, 6);
    if(Status != I2C_OK)
    {
        MPU_Recover();
        return;
    }
    sint16 rx = (sint16)((uint16)GyroArray[0] << 8 | GyroArray[1]);
    sint16 ry = (sint16)((uint16)GyroArray[2] << 8 | GyroArray[3]);
    sint16 rz = (sint16)((uint16)GyroArray[4] << 8 | GyroArray[5]);
    gyro->GYRO_X = (sint16)(((sint32)rx * 100) / 131); /* deg/s × 100 */
    gyro->GYRO_Y = (sint16)(((sint32)ry * 100) / 131);
    gyro->GYRO_Z = (sint16)(((sint32)rz * 100) / 131);
}

void MPU_Calibrate(ACCEL_t* accel, GYRO_t* gyro)
{
    sint32 accel_x_sum = 0, accel_y_sum = 0, accel_z_sum = 0;
    sint32 gyro_x_sum = 0, gyro_y_sum = 0, gyro_z_sum = 0;
    uint16 size = 5000;
    for(uint16 i=0;i<size;i++)
    {
        MPU_GetAccelerometer(accel);
        MPU_GetGyroscope(gyro);
        accel_x_sum += accel->Accel_X;
        accel_y_sum += accel->Accel_Y;
        accel_z_sum += accel->Accel_Z;
        gyro_x_sum += gyro->GYRO_X;
        gyro_y_sum += gyro->GYRO_Y;
        gyro_z_sum += gyro->GYRO_Z;
    }
    accel->Accel_X = accel_x_sum / size;
    accel->Accel_Y = accel_y_sum / size;
    accel->Accel_Z = accel_z_sum / size;
    gyro->GYRO_X = gyro_x_sum / size;
    gyro->GYRO_Y = gyro_y_sum / size;
    gyro->GYRO_Z = gyro_z_sum / size;


}