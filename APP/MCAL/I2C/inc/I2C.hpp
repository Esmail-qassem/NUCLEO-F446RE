/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : I2C                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 I2C1_BASE_ADDR = 0x40005400UL;
constexpr uint32 I2C2_BASE_ADDR = 0x40005800UL;
constexpr uint32 I2C3_BASE_ADDR = 0x40005C00UL;

/* ── Register layout ──────────────────────────────────────────────── */
struct I2C_RegDef_t
{
    volatile uint32 CR1;    /* 0x00 */
    volatile uint32 CR2;    /* 0x04 */
    volatile uint32 OAR1;   /* 0x08 */
    volatile uint32 OAR2;   /* 0x0C */
    volatile uint32 DR;     /* 0x10 */
    volatile uint32 SR1;    /* 0x14 */
    volatile uint32 SR2;    /* 0x18 */
    volatile uint32 CCR;    /* 0x1C */
    volatile uint32 TRISE;  /* 0x20 */
};

/* ── SR1 bit masks ────────────────────────────────────────────────── */
constexpr uint32 I2C_SR1_SB   = (1u <<  0u);
constexpr uint32 I2C_SR1_ADDR = (1u <<  1u);
constexpr uint32 I2C_SR1_BTF  = (1u <<  2u);
constexpr uint32 I2C_SR1_RXNE = (1u <<  6u);
constexpr uint32 I2C_SR1_TXE  = (1u <<  7u);
constexpr uint32 I2C_SR1_AF   = (1u << 10u);

/* ── CR1 bit masks ────────────────────────────────────────────────── */
constexpr uint32 I2C_CR1_PE        = (1u <<  0u);
constexpr uint32 I2C_CR1_ENGC      = (1u <<  6u);
constexpr uint32 I2C_CR1_NOSTRETCH = (1u <<  7u);
constexpr uint32 I2C_CR1_START     = (1u <<  8u);
constexpr uint32 I2C_CR1_STOP      = (1u <<  9u);
constexpr uint32 I2C_CR1_ACK       = (1u << 10u);

/* ── CR2 bit masks ────────────────────────────────────────────────── */
constexpr uint32 I2C_CR2_ITERREN = (1u <<  8u);
constexpr uint32 I2C_CR2_ITEVTEN = (1u <<  9u);
constexpr uint32 I2C_CR2_ITBUFEN = (1u << 10u);

/* ── CCR bit masks ────────────────────────────────────────────────── */
constexpr uint32 I2C_CCR_FS   = (1u << 15u);
constexpr uint32 I2C_CCR_DUTY = (1u << 14u);

/* ── Constants ────────────────────────────────────────────────────── */
constexpr uint32 I2C_TIMEOUT_MAX = 10000u;
constexpr uint8  I2C_READ  = 1u;
constexpr uint8  I2C_WRITE = 0u;

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class I2C_Status : uint8 { OK = 0, ERROR, BUSY, TIMEOUT };
enum class I2C_State  : uint8 { IDLE = 0, BUSY_TX, BUSY_RX };

enum class I2C_Port : uint8
{
    I2C1 = 0,
    I2C2,
    I2C3,
    COUNT
};

enum class I2C_TransferMode   : uint8 { POLLING = 0, INTERRUPT };
enum class I2C_AddressingMode : uint8 { ADDR_7BIT = 0, ADDR_10BIT = 1 };
enum class I2C_DutyCycle      : uint8 { DUTY_2 = 0, DUTY_16_9 = 1 };

/* ── Callback ─────────────────────────────────────────────────────── */
using I2C_Callback_t = void(*)(I2C_Port port);

/* ── Config struct ────────────────────────────────────────────────── */
struct I2C_Config_t
{
    uint32              PCLK1_Hz;
    uint32              ClockSpeed;
    I2C_DutyCycle       DutyCycle;
    I2C_AddressingMode  AddressingMode;
    uint16              OwnAddress;
    uint8               Acknowledgement;
    uint8               GeneralCallMode;
    uint8               NoStretchMode;
    uint8               DualAddressMode;
    uint8               OwnAddress2;
    I2C_TransferMode    TransferMode;
};

/* ── ISR declarations ─────────────────────────────────────────────── */
extern "C" {
    void I2C1_EV_IRQHandler(void);
    void I2C1_ER_IRQHandler(void);
    void I2C2_EV_IRQHandler(void);
    void I2C2_ER_IRQHandler(void);
    void I2C3_EV_IRQHandler(void);
    void I2C3_ER_IRQHandler(void);
}

/* ── I2C Driver Class ─────────────────────────────────────────────── */
class I2C
{
public:
    static void        Init    (I2C_Port port, const I2C_Config_t &config);
    static void        DeInit  (I2C_Port port);

    static I2C_Status  Start       (I2C_Port port);
    static I2C_Status  Stop        (I2C_Port port);
    static I2C_Status  SendAddress (I2C_Port port, uint16 address, uint8 direction);
    static I2C_Status  SendData    (I2C_Port port, uint8 data);
    static I2C_Status  ReceiveData (I2C_Port port, uint8 &data, uint8 ack);

    static I2C_Status  MasterTransmit (I2C_Port port, uint16 addr, uint8 *data, uint16 size, uint8 repStart);
    static I2C_Status  MasterReceive  (I2C_Port port, uint16 addr, uint8 *data, uint16 size, uint8 repStart);

    static I2C_Status  MasterTransmit_IT (I2C_Port port, uint16 addr, uint8 *data, uint16 size);
    static I2C_Status  MasterReceive_IT  (I2C_Port port, uint16 addr, uint8 *data, uint16 size);

    static I2C_Status  WriteRegister  (I2C_Port port, uint8 slaveAddr, uint8 reg, uint8 value);
    static I2C_Status  ReadRegister   (I2C_Port port, uint8 slaveAddr, uint8 reg, uint8 &value);
    static I2C_Status  ReadRegisters  (I2C_Port port, uint8 slaveAddr, uint8 reg, uint8 *buf, uint16 len);

    static void       RegisterTxCallback  (I2C_Port port, I2C_Callback_t cb);
    static void       RegisterRxCallback  (I2C_Port port, I2C_Callback_t cb);
    static void       RegisterErrCallback (I2C_Port port, I2C_Callback_t cb);

    static I2C_State   GetState  (I2C_Port port);
    static uint8       ReadStatus(uint32 base);

    /* used by ISRs — kept public */
    static I2C_RegDef_t* GetReg(I2C_Port port);

private:
    I2C() = delete;
};
