#include "I2C.h"

/*
 * Usage examples:
 *
 *   // Polling, 100 kHz, 7-bit addressing
 *   I2C_Config_t cfg = {
 *       .PCLK1_Hz        = 16000000,
 *       .ClockSpeed      = 100000,
 *       .DutyCycle       = I2C_DUTY_2,
 *       .AddressingMode  = I2C_ADDR_7BIT,
 *       .OwnAddress      = 0x00,
 *       .Acknowledgement = 1,
 *       .GeneralCallMode = 0,
 *       .NoStretchMode   = 0,
 *       .DualAddressMode = 0,
 *       .TransferMode    = I2C_POLLING,
 *   };
 *   I2C_Init(I2C1_PORT, &cfg);
 *   I2C_MasterTransmit(I2C1_PORT, 0x27, buf, 1, 0);
 *
 *   // Sensor register read (MPU-6050 accel X high byte)
 *   uint8 val;
 *   I2C_ReadRegister(I2C1_PORT, 0x68, 0x3B, &val);
 */

/*------------------------------------------------------------------
 *  Base address lookup table
 *------------------------------------------------------------------*/
static const uint32 I2C_BaseAddr[I2C_PORT_COUNT] = {
    I2C1_BASE,
    I2C2_BASE,
    I2C3_BASE
};

static uint32 I2C_GetBase(I2C_Port_t port)
{
    if ((uint8)port >= (uint8)I2C_PORT_COUNT) return 0U;
    return I2C_BaseAddr[(uint8)port];
}

/*------------------------------------------------------------------
 *  Per-port runtime handle (interrupt state machine)
 *------------------------------------------------------------------*/
typedef struct {
    volatile I2C_State_t state;
    uint8               *buffer;
    uint16               size;
    uint16               index;
    uint16               address;    /* pre-shifted address with R/W bit */
    I2C_Callback_t       tx_cb;
    I2C_Callback_t       rx_cb;
    I2C_Callback_t       err_cb;
} I2C_Handle_t;

static I2C_Handle_t I2C_Handle[I2C_PORT_COUNT];

/*------------------------------------------------------------------
 *  Internal: disable EV/BUF/ERR interrupts
 *------------------------------------------------------------------*/
static void I2C_DisableInterrupts(uint32 base)
{
    I2C_CR2(base) &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
}

/*------------------------------------------------------------------
 *  I2C_Init
 *------------------------------------------------------------------*/
void I2C_Init(I2C_Port_t port, const I2C_Config_t *config)
{
    uint32 base = I2C_GetBase(port);
    if (base == 0U || config == NULL) return;

    /* 1. Disable peripheral while configuring */
    I2C_CR1(base) &= ~I2C_CR1_PE;

    /* 2. Set FREQ bits (APB1 clock in MHz) */
    uint32 freq_MHz = config->PCLK1_Hz / 1000000U;
    I2C_CR2(base) = (I2C_CR2(base) & ~0x3FU) | (freq_MHz & 0x3FU);

    /* 3. Clock control register (CCR) and TRISE */
    uint32 ccr = 0U;
    if (config->ClockSpeed <= 100000U) {
        /* Standard mode: CCR = PCLK1 / (2 * Fscl) */
        uint32 val = config->PCLK1_Hz / (2U * config->ClockSpeed);
        if (val < 4U) val = 4U;
        ccr = val & 0x0FFFU;                 /* F/S=0, DUTY=0 */
        /* TRISE = Tr_max(1000 ns) / Tpclk1 + 1 = freq_MHz + 1 */
        I2C_TRISE(base) = freq_MHz + 1U;
    } else {
        /* Fast mode (up to 400 kHz) */
        ccr |= I2C_CCR_FS;
        uint32 val;
        if (config->DutyCycle == I2C_DUTY_16_9) {
            /* Tlow/Thigh = 16/9 → CCR = PCLK1 / (25 * Fscl) */
            ccr |= I2C_CCR_DUTY;
            val = config->PCLK1_Hz / (25U * config->ClockSpeed);
        } else {
            /* Tlow/Thigh = 2 → CCR = PCLK1 / (3 * Fscl) */
            val = config->PCLK1_Hz / (3U * config->ClockSpeed);
        }
        if (val == 0U) val = 1U;
        ccr |= (val & 0x0FFFU);
        /* TRISE for FM: Tr_max = 300 ns → ceil(300 / (1/PCLK1 * 1e9)) + 1 */
        I2C_TRISE(base) = ((freq_MHz * 300U) / 1000U) + 1U;
    }
    I2C_CCR(base) = ccr;

    /* 4. Own address */
    if (config->AddressingMode == I2C_ADDR_10BIT) {
        /* Bit 15 = 1 for 10-bit mode */
        I2C_OAR1(base) = (1U << 15) | (config->OwnAddress & 0x3FFU);
    } else {
        /* Bit 14 must be kept 1 (reserved, always write 1) */
        I2C_OAR1(base) = (1U << 14) | ((config->OwnAddress & 0x7FU) << 1U);
    }

    /* 5. Dual address */
    if (config->DualAddressMode) {
        I2C_OAR2(base) = ((config->OwnAddress2 & 0x7FU) << 1U) | 0x01U;
    } else {
        I2C_OAR2(base) = 0U;
    }

    /* 6. CR1 options */
    uint32 cr1 = I2C_CR1_PE;
    if (config->Acknowledgement)  cr1 |= I2C_CR1_ACK;
    if (config->GeneralCallMode)  cr1 |= I2C_CR1_ENGC;
    if (config->NoStretchMode)    cr1 |= I2C_CR1_NOSTRETCH;
    I2C_CR1(base) = cr1;
}

/*------------------------------------------------------------------
 *  I2C_DeInit
 *------------------------------------------------------------------*/
void I2C_DeInit(I2C_Port_t port)
{
    uint32 base = I2C_GetBase(port);
    if (base == 0U) return;
    I2C_CR1(base) &= ~I2C_CR1_PE;
    I2C_DisableInterrupts(base);
    I2C_Handle[port].state = I2C_IDLE;
}

/*------------------------------------------------------------------
 *  I2C_Start
 *------------------------------------------------------------------*/
I2C_Status_t I2C_Start(I2C_Port_t port)
{
    uint32 base    = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    I2C_CR1(base) |= I2C_CR1_START;
    while (!(I2C_SR1(base) & I2C_SR1_SB)) {
        if (--timeout == 0U) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  I2C_Stop
 *------------------------------------------------------------------*/
I2C_Status_t I2C_Stop(I2C_Port_t port)
{
    uint32 base = I2C_GetBase(port);
    I2C_CR1(base) |= I2C_CR1_STOP;
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  I2C_SendAddress
 *  address: raw 7-bit or 10-bit value (driver shifts and adds R/W)
 *------------------------------------------------------------------*/
I2C_Status_t I2C_SendAddress(I2C_Port_t port, uint16 address, uint8 direction)
{
    uint32 base    = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    I2C_DR(base) = ((uint32)address << 1U) | (direction & 0x01U);

    while (!(I2C_SR1(base) & I2C_SR1_ADDR)) {
        if (I2C_SR1(base) & I2C_SR1_AF) {
            I2C_CR1(base) |= I2C_CR1_STOP;
            return I2C_ERROR;             /* NACK from slave */
        }
        if (--timeout == 0U) return I2C_TIMEOUT;
    }

    /* Clear ADDR by reading SR1 then SR2 */
    (void)I2C_SR1(base);
    (void)I2C_SR2(base);
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  I2C_SendData
 *------------------------------------------------------------------*/
I2C_Status_t I2C_SendData(I2C_Port_t port, uint8 data)
{
    uint32 base    = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    I2C_DR(base) = data;
    while (!(I2C_SR1(base) & I2C_SR1_TXE)) {
        if (--timeout == 0U) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  I2C_ReceiveData
 *  ack=1 → ACK the byte (more bytes to follow)
 *  ack=0 → NACK the byte (last byte)
 *------------------------------------------------------------------*/
I2C_Status_t I2C_ReceiveData(I2C_Port_t port, uint8 *data, uint8 ack)
{
    uint32 base    = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    if (ack)
        I2C_CR1(base) |= I2C_CR1_ACK;
    else
        I2C_CR1(base) &= ~I2C_CR1_ACK;

    while (!(I2C_SR1(base) & I2C_SR1_RXNE)) {
        if (--timeout == 0U) return I2C_TIMEOUT;
    }
    *data = (uint8)I2C_DR(base);
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  I2C_MasterTransmit  (blocking)
 *------------------------------------------------------------------*/
I2C_Status_t I2C_MasterTransmit(I2C_Port_t port, uint16 slave_addr,
                                  uint8 *data, uint16 size, uint8 repeated_start)
{
    uint32       base   = I2C_GetBase(port);
    I2C_Status_t status;

    status = I2C_Start(port);
    if (status != I2C_OK) return status;

    status = I2C_SendAddress(port, slave_addr, I2C_WRITE);
    if (status != I2C_OK) return status;

    for (uint16 i = 0U; i < size; i++) {
        status = I2C_SendData(port, data[i]);
        if (status != I2C_OK) return status;
    }

    /* Wait for BTF (all bytes shifted out) */
    uint32 timeout = I2C_TIMEOUT_MAX;
    while (!(I2C_SR1(base) & I2C_SR1_BTF)) {
        if (--timeout == 0U) return I2C_TIMEOUT;
    }

    if (!repeated_start) {
        I2C_Stop(port);
    }
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  I2C_MasterReceive  (blocking)
 *------------------------------------------------------------------*/
I2C_Status_t I2C_MasterReceive(I2C_Port_t port, uint16 slave_addr,
                                 uint8 *data, uint16 size, uint8 repeated_start)
{
    I2C_Status_t status;

    status = I2C_Start(port);
    if (status != I2C_OK) return status;

    status = I2C_SendAddress(port, slave_addr, I2C_READ);
    if (status != I2C_OK) return status;

    for (uint16 i = 0U; i < size; i++) {
        uint8 ack = (i < (size - 1U)) ? 1U : 0U;
        status = I2C_ReceiveData(port, &data[i], ack);
        if (status != I2C_OK) return status;
    }

    if (!repeated_start) {
        I2C_Stop(port);
    }
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  Register helpers — common sensor / EEPROM patterns
 *------------------------------------------------------------------*/

/* Write single register: START | addr+W | reg | value | STOP */
I2C_Status_t I2C_WriteRegister(I2C_Port_t port, uint8 slave_addr,
                                 uint8 reg, uint8 value)
{
    uint8 buf[2] = { reg, value };
    return I2C_MasterTransmit(port, (uint16)slave_addr, buf, 2U, 0U);
}

/* Read single register: START | addr+W | reg | Sr | addr+R | data | STOP */
I2C_Status_t I2C_ReadRegister(I2C_Port_t port, uint8 slave_addr,
                                uint8 reg, uint8 *value)
{
    I2C_Status_t status;
    status = I2C_MasterTransmit(port, (uint16)slave_addr, &reg, 1U, 1U);
    if (status != I2C_OK) return status;
    return I2C_MasterReceive(port, (uint16)slave_addr, value, 1U, 0U);
}

/* Burst read: START | addr+W | reg | Sr | addr+R | data[0..len-1] | STOP */
I2C_Status_t I2C_ReadRegisters(I2C_Port_t port, uint8 slave_addr,
                                 uint8 reg, uint8 *buf, uint16 len)
{
    I2C_Status_t status;
    status = I2C_MasterTransmit(port, (uint16)slave_addr, &reg, 1U, 1U);
    if (status != I2C_OK) return status;
    return I2C_MasterReceive(port, (uint16)slave_addr, buf, len, 0U);
}

/*------------------------------------------------------------------
 *  Callback registration
 *------------------------------------------------------------------*/
void I2C_RegisterTxCallback(I2C_Port_t port, I2C_Callback_t cb)
{
    if ((uint8)port < (uint8)I2C_PORT_COUNT) I2C_Handle[port].tx_cb = cb;
}

void I2C_RegisterRxCallback(I2C_Port_t port, I2C_Callback_t cb)
{
    if ((uint8)port < (uint8)I2C_PORT_COUNT) I2C_Handle[port].rx_cb = cb;
}

void I2C_RegisterErrorCallback(I2C_Port_t port, I2C_Callback_t cb)
{
    if ((uint8)port < (uint8)I2C_PORT_COUNT) I2C_Handle[port].err_cb = cb;
}

/*------------------------------------------------------------------
 *  State query
 *------------------------------------------------------------------*/
I2C_State_t I2C_GetState(I2C_Port_t port)
{
    if ((uint8)port >= (uint8)I2C_PORT_COUNT) return I2C_IDLE;
    return I2C_Handle[port].state;
}

uint8 I2C_ReadStatus(uint32 base)
{
    return (uint8)I2C_SR1(base);
}

/*------------------------------------------------------------------
 *  Interrupt-driven transmit (non-blocking)
 *  Caller must have enabled the corresponding NVIC line.
 *------------------------------------------------------------------*/
I2C_Status_t I2C_MasterTransmit_IT(I2C_Port_t port, uint16 slave_addr,
                                     uint8 *data, uint16 size)
{
    uint32 base = I2C_GetBase(port);
    if (base == 0U || data == NULL || size == 0U) return I2C_ERROR;
    if (I2C_Handle[port].state != I2C_IDLE)       return I2C_BUSY;

    I2C_Handle_t *h = &I2C_Handle[port];
    h->state   = I2C_BUSY_TX;
    h->buffer  = data;
    h->size    = size;
    h->index   = 0U;
    h->address = ((uint16)slave_addr << 1U) | I2C_WRITE;

    I2C_CR2(base) |= (I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    I2C_CR1(base) |= I2C_CR1_START;
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  Interrupt-driven receive (non-blocking)
 *------------------------------------------------------------------*/
I2C_Status_t I2C_MasterReceive_IT(I2C_Port_t port, uint16 slave_addr,
                                    uint8 *data, uint16 size)
{
    uint32 base = I2C_GetBase(port);
    if (base == 0U || data == NULL || size == 0U) return I2C_ERROR;
    if (I2C_Handle[port].state != I2C_IDLE)       return I2C_BUSY;

    I2C_Handle_t *h = &I2C_Handle[port];
    h->state   = I2C_BUSY_RX;
    h->buffer  = data;
    h->size    = size;
    h->index   = 0U;
    h->address = ((uint16)slave_addr << 1U) | I2C_READ;

    I2C_CR1(base) |= I2C_CR1_ACK;   /* ACK bytes as they arrive */
    I2C_CR2(base) |= (I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    I2C_CR1(base) |= I2C_CR1_START;
    return I2C_OK;
}

/*------------------------------------------------------------------
 *  Internal: shared EV IRQ body (called from all three port handlers)
 *------------------------------------------------------------------*/
static void I2C_EV_Handler(I2C_Port_t port)
{
    uint32        base = I2C_GetBase(port);
    uint32        sr1  = I2C_SR1(base);
    I2C_Handle_t *h   = &I2C_Handle[port];

    /* SB: START sent → write address byte */
    if (sr1 & I2C_SR1_SB) {
        I2C_DR(base) = h->address;
        return;
    }

    /* ADDR: address phase done → clear by reading SR1 then SR2 */
    if (sr1 & I2C_SR1_ADDR) {
        (void)I2C_SR1(base);
        (void)I2C_SR2(base);
        return;
    }

    /* TXE: TX register empty → write next byte */
    if ((sr1 & I2C_SR1_TXE) && (h->state == I2C_BUSY_TX)) {
        if (h->index < h->size) {
            I2C_DR(base) = h->buffer[h->index++];
        }
        return;
    }

    /* BTF after last TX byte → STOP + notify */
    if ((sr1 & I2C_SR1_BTF) && (h->state == I2C_BUSY_TX)
                              && (h->index >= h->size)) {
        I2C_CR1(base) |= I2C_CR1_STOP;
        I2C_DisableInterrupts(base);
        h->state = I2C_IDLE;
        if (h->tx_cb) h->tx_cb(port);
        return;
    }

    /* RXNE: RX register has data */
    if ((sr1 & I2C_SR1_RXNE) && (h->state == I2C_BUSY_RX)) {
        if (h->index < h->size) {
            /* Send NACK + STOP before reading the last byte */
            if (h->index == h->size - 1U) {
                I2C_CR1(base) &= ~I2C_CR1_ACK;
                I2C_CR1(base) |=  I2C_CR1_STOP;
            }
            h->buffer[h->index++] = (uint8)I2C_DR(base);

            if (h->index >= h->size) {
                I2C_DisableInterrupts(base);
                h->state = I2C_IDLE;
                if (h->rx_cb) h->rx_cb(port);
            }
        }
        return;
    }

    I2C_DisableInterrupts(base);
}

/*------------------------------------------------------------------
 *  Internal: shared ER IRQ body
 *------------------------------------------------------------------*/
static void I2C_ER_Handler(I2C_Port_t port)
{
    uint32 base = I2C_GetBase(port);
    I2C_SR1(base) = 0U;              /* clear all error flags */
    I2C_Handle[port].state = I2C_IDLE;
    I2C_DisableInterrupts(base);
    if (I2C_Handle[port].err_cb) I2C_Handle[port].err_cb(port);
}

/*------------------------------------------------------------------
 *  IRQ handlers — all three ports delegate to shared bodies
 *------------------------------------------------------------------*/
void I2C1_EV_IRQHandler(void) { I2C_EV_Handler(I2C1_PORT); }
void I2C1_ER_IRQHandler(void) { I2C_ER_Handler(I2C1_PORT); }

void I2C2_EV_IRQHandler(void) { I2C_EV_Handler(I2C2_PORT); }
void I2C2_ER_IRQHandler(void) { I2C_ER_Handler(I2C2_PORT); }

void I2C3_EV_IRQHandler(void) { I2C_EV_Handler(I2C3_PORT); }
void I2C3_ER_IRQHandler(void) { I2C_ER_Handler(I2C3_PORT); }
