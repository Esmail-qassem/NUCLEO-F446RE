#ifndef I2C_H_
#define I2C_H_

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/*-----------------------------*/
#define I2C1_BASE 0x40005400   
#define I2C2_BASE 0x40005800   
#define I2C3_BASE 0x40005C00   

#define I2C_CR1(base)         *((volatile uint32*)(base + 0x0))
#define I2C_CR2(base)         *((volatile uint32*)(base + 0x4))
#define I2C_OAR1(base)        *((volatile uint32*)(base + 0x8))
#define I2C_OAR2(base)        *((volatile uint32*)(base + 0xC))
#define I2C_DR(base)          *((volatile uint32*)(base + 0x10))
#define I2C_SR1(base)         *((volatile uint32*)(base + 0x14))
#define I2C_SR2(base)         *((volatile uint32*)(base + 0x18))
#define I2C_CCR(base)         *((volatile uint32*)(base + 0x1C))
#define I2C_TRISE(base)       *((volatile uint32*)(base + 0x20))

/*-----------------------------*/
#define I2C_TIMEOUT_MAX  10000
#define I2C_READ  1
#define I2C_WRITE 0
#define PCLK1 16000000
/*-----------------------------*/
/*         ENUMS/TYPES         */
/*-----------------------------*/
typedef enum {
    I2C_OK = 0,
    I2C_ERROR,
    I2C_BUSY,
    I2C_TIMEOUT
} I2C_Status_t;
typedef enum {
    I2C_IDLE,
    I2C_BUSY_TX,
    I2C_BUSY_RX
} I2C_State_t;

typedef enum {
    I2C1_PORT,
    I2C2_PORT,
    I2C3_PORT,
} I2C_Port_t;



typedef enum
{
    I2C_POLLING,
    I2C_INTERRUPT
}
I2C_Intruppt_t;

/* Configuration structure */
typedef struct {
    uint32 ClockSpeed;       // e.g. 100000 for 100kHz
    uint8 OwnAddress;        // Device address (if slave)
    uint8 Acknowledgement;   // ENABLE / DISABLE
    uint8 DutyCycle;         // for Fast mode (if used)
  //  I2C_Intruppt_t I2C_INT;

} I2C_Config_t;

/* Initialization and configuration */
void I2C_Init(I2C_Port_t port, I2C_Config_t *config);
void I2C_DeInit(I2C_Port_t port);

/* Basic operations */
I2C_Status_t I2C_Start(I2C_Port_t port);
I2C_Status_t I2C_Stop(I2C_Port_t port);
I2C_Status_t I2C_SendAddress(I2C_Port_t port, uint8 address, uint8 direction);
I2C_Status_t I2C_SendData(I2C_Port_t port, uint8 data);
I2C_Status_t I2C_ReceiveData(I2C_Port_t port, uint8 *data, uint8 ack);

/* High-level blocking APIs */
I2C_Status_t I2C_MasterTransmit(I2C_Port_t port, uint8 slave_addr, uint8 *data, uint16 size, uint8 repeated_start);
I2C_Status_t I2C_MasterReceive(I2C_Port_t port, uint8 slave_addr, uint8 *data, uint16 size, uint8 repeated_start);

/* interrupt/event/error handling */
void I2C_MasterTransmit_IT(I2C_Port_t port, uint8 slave_addr, uint8 *data, uint16 size);


uint8 I2C_ReadStatus(uint32 base);





#endif