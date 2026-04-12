#ifndef MPU_H
#define MPU_H
#include "STD_TYPES.h"

#define MPU_ADDR   0x68   /* AD0 pin low → 0x68, high → 0x69 */
#define REG_PWR    0x6B   /* Power management */
#define REG_WHO    0x75   /* WHO_AM_I — should return 0x68 */
#define REG_TEMP_H   0x41   /* Temperature data starts here */
#define GYRO_XOUT_H     0x43
#define GYRO_YOUT_H     0x45 
#define GYRO_ZOUT_H     0x47
#define ACCEL_XOUT_H   0x3B 
#define ACCEL_YOUT_H   0x3D 
#define ACCEL_ZOUT_H   0x3F 

typedef struct{
    sint16 Accel_X;    
    sint16 Accel_Y;
    sint16 Accel_Z;
}ACCEL_t ;
typedef struct{
    sint16 GYRO_X;    
    sint16 GYRO_Y;
    sint16 GYRO_Z;
}GYRO_t ;
void MPU_Init(void);
void MPU_GetAccelerometer(ACCEL_t* accel);
void MPU_GetGyroscope(GYRO_t* gyro);
sint16 MPU_GetTemp(void);
void MPU_Calibrate(ACCEL_t* accel, GYRO_t* gyro);

#endif