#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include "STD_TYPES.h"
#include "MPU.h"

/*------------------------------------------------------------------
 *  Firmware version (defined in App_Config.c)
 *------------------------------------------------------------------*/
#define FIRMWARE_VERSION_STR    "1.0.1"
extern const uint8 FIRMWARE_VERSION[];

/*------------------------------------------------------------------
 *  Sensor calibration bias (filled by APP_Init, read by SwM)
 *------------------------------------------------------------------*/
extern ACCEL_t Bias_accel_data;
extern GYRO_t  Bias_gyro_data;

/*------------------------------------------------------------------
 *  System bring-up
 *------------------------------------------------------------------*/
void APP_ClockInit(void);   /* called from SystemInit() before main */
void APP_Init(void);        /* called from main()                   */

#endif /* APP_CONFIG_H_ */
