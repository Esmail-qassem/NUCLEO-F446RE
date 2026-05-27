#include "I2C.hpp"
/* Legacy shims */
#define I2C1_PORT  I2C_Port::I2C1
#define I2C2_PORT  I2C_Port::I2C2
#define I2C3_PORT  I2C_Port::I2C3
#define I2C_PORT_COUNT static_cast<uint8>(I2C_Port::COUNT)
#define I2C_OK      I2C_Status::OK
#define I2C_ERROR   I2C_Status::ERROR
#define I2C_BUSY    I2C_Status::BUSY
#define I2C_TIMEOUT I2C_Status::TIMEOUT
#define I2C_IDLE    I2C_State::IDLE
#define I2C_BUSY_TX I2C_State::BUSY_TX
#define I2C_BUSY_RX I2C_State::BUSY_RX
#define I2C_POLLING   I2C_TransferMode::POLLING
#define I2C_INTERRUPT I2C_TransferMode::INTERRUPT
#define I2C_ADDR_7BIT  I2C_AddressingMode::ADDR_7BIT
#define I2C_ADDR_10BIT I2C_AddressingMode::ADDR_10BIT
#define I2C_DUTY_2    I2C_DutyCycle::DUTY_2
#define I2C_DUTY_16_9 I2C_DutyCycle::DUTY_16_9
#define I2C_Init(p,c)                      I2C::Init(p, *(c))
#define I2C_DeInit(p)                      I2C::DeInit(p)
#define I2C_Start(p)                       I2C::Start(p)
#define I2C_Stop(p)                        I2C::Stop(p)
#define I2C_SendAddress(p,a,d)             I2C::SendAddress(p,a,d)
#define I2C_SendData(p,d)                  I2C::SendData(p,d)
#define I2C_ReceiveData(p,d,a)             I2C::ReceiveData(p,*(d),a)
#define I2C_MasterTransmit(p,a,b,s,r)     I2C::MasterTransmit(p,a,b,s,r)
#define I2C_MasterReceive(p,a,b,s,r)      I2C::MasterReceive(p,a,b,s,r)
#define I2C_MasterTransmit_IT(p,a,b,s)    I2C::MasterTransmit_IT(p,a,b,s)
#define I2C_MasterReceive_IT(p,a,b,s)     I2C::MasterReceive_IT(p,a,b,s)
#define I2C_WriteRegister(p,sa,r,v)        I2C::WriteRegister(p,sa,r,v)
#define I2C_ReadRegister(p,sa,r,v)         I2C::ReadRegister(p,sa,r,*(v))
#define I2C_ReadRegisters(p,sa,r,b,l)     I2C::ReadRegisters(p,sa,r,b,l)
#define I2C_RegisterTxCallback(p,cb)      I2C::RegisterTxCallback(p,cb)
#define I2C_RegisterRxCallback(p,cb)      I2C::RegisterRxCallback(p,cb)
#define I2C_RegisterErrorCallback(p,cb)   I2C::RegisterErrCallback(p,cb)
#define I2C_GetState(p)                    I2C::GetState(p)
#define I2C_ReadStatus(b)                  I2C::ReadStatus(b)

#ifndef I2C_H_
#define I2C_H_

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/*------------------------------------------------------------------
 *  Register base addresses
 *------------------------------------------------------------------*/
#define I2C1_BASE  0x40005400U
#define I2C2_BASE  0x40005800U
#define I2C3_BASE  0x40005C00U

/*------------------------------------------------------------------
 *  Register access macros
 *------------------------------------------------------------------*/
#define I2C_CR1(base)    *((volatile uint32*)(base + 0x00U))
#define I2C_CR2(base)    *((volatile uint32*)(base + 0x04U))
#define I2C_OAR1(base)   *((volatile uint32*)(base + 0x08U))
#define I2C_OAR2(base)   *((volatile uint32*)(base + 0x0CU))
#define I2C_DR(base)     *((volatile uint32*)(base + 0x10U))
#define I2C_SR1(base)    *((volatile uint32*)(base + 0x14U))
#define I2C_SR2(base)    *((volatile uint32*)(base + 0x18U))
#define I2C_CCR(base)    *((volatile uint32*)(base + 0x1CU))
#define I2C_TRISE(base)  *((volatile uint32*)(base + 0x20U))

/*------------------------------------------------------------------
 *  SR1 bit masks
 *------------------------------------------------------------------*/
#define I2C_SR1_SB      (1U <<  0)   /* Start bit sent            */
#define I2C_SR1_ADDR    (1U <<  1)   /* Address sent / matched    */
#define I2C_SR1_BTF     (1U <<  2)   /* Byte transfer finished    */
#define I2C_SR1_RXNE    (1U <<  6)   /* RX data register not empty*/
#define I2C_SR1_TXE     (1U <<  7)   /* TX data register empty    */
#define I2C_SR1_BERR    (1U <<  8)   /* Bus error                 */
#define I2C_SR1_ARLO    (1U <<  9)   /* Arbitration lost          */
#define I2C_SR1_AF      (1U << 10)   /* Acknowledge failure       */
#define I2C_SR1_OVR     (1U << 11)   /* Overrun / Underrun        */

/*------------------------------------------------------------------
 *  CR1 bit masks
 *------------------------------------------------------------------*/
#define I2C_CR1_PE        (1U <<  0)  /* Peripheral enable         */
#define I2C_CR1_SMBUS     (1U <<  1)  /* SMBus mode                */
#define I2C_CR1_ENGC      (1U <<  6)  /* General call enable       */
#define I2C_CR1_NOSTRETCH (1U <<  7)  /* Clock stretching disable  */
#define I2C_CR1_START     (1U <<  8)  /* Start generation          */
#define I2C_CR1_STOP      (1U <<  9)  /* Stop generation           */
#define I2C_CR1_ACK      (1U << 10)  /* Acknowledge enable        */

/*------------------------------------------------------------------
 *  CR2 bit masks
 *------------------------------------------------------------------*/
#define I2C_CR2_ITERREN  (1U <<  8)  /* Error interrupt enable     */
#define I2C_CR2_ITEVTEN  (1U <<  9)  /* Event interrupt enable     */
#define I2C_CR2_ITBUFEN  (1U << 10)  /* Buffer interrupt enable    */

/*------------------------------------------------------------------
 *  CCR bit masks
 *------------------------------------------------------------------*/
#define I2C_CCR_FS       (1U << 15)  /* Fast mode select          */
#define I2C_CCR_DUTY     (1U << 14)  /* Fast mode duty cycle      */

/*------------------------------------------------------------------
 *  Constants
 *------------------------------------------------------------------*/
#define I2C_TIMEOUT_MAX  10000U
#define I2C_READ         1U
#define I2C_WRITE        0U

/*------------------------------------------------------------------
 *  Enumerations
 *------------------------------------------------------------------*/
typedef enum {
    I2C_OK = 0,
    I2C_ERROR,
    I2C_BUSY,
    I2C_TIMEOUT
} I2C_Status_t;

typedef enum {
    I2C_IDLE = 0,
    I2C_BUSY_TX,
    I2C_BUSY_RX
} I2C_State_t;

typedef enum {
    I2C1_PORT = 0,
    I2C2_PORT,
    I2C3_PORT,
    I2C_PORT_COUNT
} I2C_Port_t;

typedef enum {
    I2C_POLLING = 0,
    I2C_INTERRUPT
} I2C_TransferMode_t;

typedef enum {
    I2C_ADDR_7BIT  = 0,
    I2C_ADDR_10BIT = 1
} I2C_AddressingMode_t;

typedef enum {
    I2C_DUTY_2    = 0,   /* Tlow/Thigh = 2    (CCR[14]=0) – default FM  */
    I2C_DUTY_16_9 = 1    /* Tlow/Thigh = 16/9 (CCR[14]=1) – FM+ duty    */
} I2C_DutyCycle_t;

/*------------------------------------------------------------------
 *  Callback type
 *------------------------------------------------------------------*/
typedef void (*I2C_Callback_t)(I2C_Port_t port);

/*------------------------------------------------------------------
 *  Configuration structure
 *------------------------------------------------------------------*/
typedef struct {
    uint32               PCLK1_Hz;         /* APB1 clock in Hz (e.g. 16000000)        */
    uint32               ClockSpeed;       /* Bus speed in Hz (≤100 000 or ≤400 000)  */
    I2C_DutyCycle_t      DutyCycle;        /* Fast-mode duty cycle                     */
    I2C_AddressingMode_t AddressingMode;   /* I2C_ADDR_7BIT or I2C_ADDR_10BIT         */
    uint16               OwnAddress;       /* Own address (7-bit: bits[6:0], 10-bit: bits[9:0]) */
    uint8                Acknowledgement;  /* 1 = ACK enabled, 0 = NACK               */
    uint8                GeneralCallMode;  /* 1 = respond to general-call address      */
    uint8                NoStretchMode;    /* 1 = disable clock stretching (slave)     */
    uint8                DualAddressMode;  /* 1 = enable second own address            */
    uint8                OwnAddress2;      /* Second 7-bit own address                 */
    I2C_TransferMode_t   TransferMode;     /* I2C_POLLING or I2C_INTERRUPT             */
} I2C_Config_t;

/*------------------------------------------------------------------
 *  Initialization
 *------------------------------------------------------------------*/
void         I2C_Init(I2C_Port_t port, const I2C_Config_t *config);
void         I2C_DeInit(I2C_Port_t port);

/*------------------------------------------------------------------
 *  Low-level primitives (usable directly for custom sequences)
 *------------------------------------------------------------------*/
I2C_Status_t I2C_Start(I2C_Port_t port);
I2C_Status_t I2C_Stop(I2C_Port_t port);
I2C_Status_t I2C_SendAddress(I2C_Port_t port, uint16 address, uint8 direction);
I2C_Status_t I2C_SendData(I2C_Port_t port, uint8 data);
I2C_Status_t I2C_ReceiveData(I2C_Port_t port, uint8 *data, uint8 ack);

/*------------------------------------------------------------------
 *  Blocking (polling) transfers
 *------------------------------------------------------------------*/
I2C_Status_t I2C_MasterTransmit(I2C_Port_t port, uint16 slave_addr,
                                  uint8 *data, uint16 size, uint8 repeated_start);
I2C_Status_t I2C_MasterReceive(I2C_Port_t port, uint16 slave_addr,
                                 uint8 *data, uint16 size, uint8 repeated_start);

/*------------------------------------------------------------------
 *  Non-blocking (interrupt-driven) transfers
 *  Caller must enable the relevant NVIC lines before using these.
 *  The registered callback fires when the transfer completes.
 *------------------------------------------------------------------*/
I2C_Status_t I2C_MasterTransmit_IT(I2C_Port_t port, uint16 slave_addr,
                                     uint8 *data, uint16 size);
I2C_Status_t I2C_MasterReceive_IT(I2C_Port_t port, uint16 slave_addr,
                                    uint8 *data, uint16 size);

/*------------------------------------------------------------------
 *  Register-access helpers (sensor / EEPROM friendly)
 *
 *  Write one byte to a device register:
 *      START | addr+W | reg | value | STOP
 *
 *  Read one byte from a device register:
 *      START | addr+W | reg | Sr | addr+R | data | STOP
 *
 *  Read multiple consecutive registers (burst read):
 *      START | addr+W | reg | Sr | addr+R | data[0..len-1] | STOP
 *------------------------------------------------------------------*/
I2C_Status_t I2C_WriteRegister(I2C_Port_t port, uint8 slave_addr,
                                 uint8 reg, uint8 value);
I2C_Status_t I2C_ReadRegister(I2C_Port_t port, uint8 slave_addr,
                                uint8 reg, uint8 *value);
I2C_Status_t I2C_ReadRegisters(I2C_Port_t port, uint8 slave_addr,
                                 uint8 reg, uint8 *buf, uint16 len);

/*------------------------------------------------------------------
 *  Callback registration
 *------------------------------------------------------------------*/
void I2C_RegisterTxCallback(I2C_Port_t port, I2C_Callback_t cb);
void I2C_RegisterRxCallback(I2C_Port_t port, I2C_Callback_t cb);
void I2C_RegisterErrorCallback(I2C_Port_t port, I2C_Callback_t cb);

/*------------------------------------------------------------------
 *  Status helpers
 *------------------------------------------------------------------*/
I2C_State_t  I2C_GetState(I2C_Port_t port);
uint8        I2C_ReadStatus(uint32 base);

#endif /* I2C_H_ */
