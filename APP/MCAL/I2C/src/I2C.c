#include "I2C.h"
/** I2C_Config_t cfg = {100000, 0x00, 1, 0};
    I2C_Init(I2C1_PORT, &cfg);
    I2C_MasterTransmit(I2C1_PORT, 0x27, &cmd, 1);
 */


/* Helper macro to get base address */
static uint32 I2C_GetBase(I2C_Port_t port) {
    switch(port)
    {
        case I2C1_PORT : return I2C1_BASE;break;
        case I2C2_PORT : return I2C2_BASE;break;
        case I2C3_PORT : return I2C3_BASE;break;
        default : break;
    }
}


void I2C_Init(I2C_Port_t port, I2C_Config_t *config)
{
    uint32 base = I2C_GetBase(port);

    /*-----------------------------*
     * 1. Disable I2C before config
     *-----------------------------*/
    CLEAR_BIT(I2C_CR1(base), 0);  // PE = 0

    /*-----------------------------*
     * 2. Peripheral input clock (PCLK1 = 36 MHz)
     *-----------------------------*/
    uint32 pclk1 = PCLK1;
    uint32 freq_range = pclk1 / 1000000;
    I2C_CR2(base) = (I2C_CR2(base) & ~0x3F) | (freq_range & 0x3F);

    /*-----------------------------*
     * 3. Configure clock control
     *-----------------------------*/
    uint16 ccr_value = 0;

    if (config->ClockSpeed <= 100000)   // Standard Mode (≤100 kHz)
    {
        CLEAR_BIT(I2C_CCR(base), 15);   // F/S = 0 → Standard mode
        ccr_value = pclk1 / (config->ClockSpeed * 2);
        if (ccr_value < 4) ccr_value = 4;
        I2C_CCR(base) = ccr_value;

        /* Rise time for Standard mode: TRISE = Freq_MHz + 1 */
        I2C_TRISE(base) = freq_range + 1;
    }
    else if(config->ClockSpeed <= 400000)                                // Fast Mode (400 kHz)
    {
        SET_BIT(I2C_CCR(base), 15);     // F/S = 1 → Fast mode
        CLEAR_BIT(I2C_CCR(base), 14);   // DUTY = 0 (Tlow/Thigh = 2)
        ccr_value = pclk1 / (3 * config->ClockSpeed);
        if (ccr_value == 0) ccr_value = 1;
        I2C_CCR(base) = (I2C_CCR(base) & ~0x0FFF) | (ccr_value & 0x0FFF);

        /* Rise time for Fast mode: TRISE = (Freq_MHz * 300ns) + 1 */
        I2C_TRISE(base) = (uint16)(((freq_range * 300) / 1000) + 1);
    }
    else
    {
        //do nothing
    }

    /*-----------------------------*
     * 4. Program own address
     *-----------------------------*/
    I2C_OAR1(base) = (config->OwnAddress & 0x7F) << 1;
    SET_BIT(I2C_OAR1(base), 14);  // Bit 14 must be kept 1

    /*-----------------------------*
     * 5. Acknowledge enable/disable
     *-----------------------------*/
    if (config->Acknowledgement)
        SET_BIT(I2C_CR1(base), 10);  // ACK = 1
    else
        CLEAR_BIT(I2C_CR1(base), 10); // ACK = 0

    /*-----------------------------*
     * 6. Enable I2C peripheral
     *-----------------------------*/
    SET_BIT(I2C_CR1(base), 0);  // PE = 1
}

void I2C_DeInit(I2C_Port_t port)
{
     uint32 base = I2C_GetBase(port);
    CLEAR_BIT(I2C_CR1(base), 0); // Disable
}
I2C_Status_t I2C_Start(I2C_Port_t port)
{
    uint32 base = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    /* Generate START */
    SET_BIT(I2C_CR1(base), 8); // START = 1

    /* Wait for SB (start bit set) */
    while (!(I2C_SR1(base) & (1 << 0))) {
        if (--timeout == 0) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

I2C_Status_t I2C_SendAddress(I2C_Port_t port, uint8 address, uint8 direction)
{
    uint32 base = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    /* Send address */
    I2C_DR(base) = (address << 1) | (direction & 0x01);

    /* Wait for ADDR flag */
    while (!(I2C_SR1(base) & (1 << 1))) {
        if (--timeout == 0) return I2C_TIMEOUT;
    }

    /* Clear ADDR by reading SR1 then SR2 */
    (void)I2C_SR1(base);
    (void)I2C_SR2(base);

    return I2C_OK;
}

/*-----------------------------------------------------------*/
I2C_Status_t I2C_SendData(I2C_Port_t port, uint8 data)
{
    uint32 base = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    /* Write data */
    I2C_DR(base) = data;

    /* Wait for TXE */
    while (!(I2C_SR1(base) & (1 << 7))) {
        if (--timeout == 0) return I2C_TIMEOUT;
    }

    return I2C_OK;
}

/*-----------------------------------------------------------*/
I2C_Status_t I2C_ReceiveData(I2C_Port_t port, uint8 *data, uint8 ack)
{
    uint32 base = I2C_GetBase(port);
    uint32 timeout = I2C_TIMEOUT_MAX;

    if (ack)
        SET_BIT(I2C_CR1(base), 10); // ACK = 1
    else
        CLEAR_BIT(I2C_CR1(base), 10);

    while (!(I2C_SR1(base) & (1 << 6))) { // RXNE
        if (--timeout == 0) return I2C_TIMEOUT;
    }

    *data = (uint8)I2C_DR(base);
    return I2C_OK;
}

/*-----------------------------------------------------------*/
I2C_Status_t I2C_Stop(I2C_Port_t port)
{
    uint32 base = I2C_GetBase(port);
    SET_BIT(I2C_CR1(base), 9);  // STOP = 1
    return I2C_OK;
}

/*-----------------------------------------------------------*/
I2C_Status_t I2C_MasterTransmit(I2C_Port_t port, uint8 slave_addr, uint8 *data, uint16 size, uint8 repeated_start)
{
    uint32 base = I2C_GetBase(port);
    I2C_Status_t status;

    // Generate START condition
    status = I2C_Start(port);
    if (status != I2C_OK) return status;

    // Send slave address with Write bit
    status = I2C_SendAddress(port, slave_addr, I2C_WRITE);
    if (status != I2C_OK) return status;

    // Send all bytes
    for (uint16 i = 0; i < size; i++) {
        status = I2C_SendData(port, data[i]);
        if (status != I2C_OK) return status;
    }

    // Wait for BTF (Byte Transfer Finished)
    uint32 timeout = I2C_TIMEOUT_MAX;
    while (!(I2C_SR1(base) & (1 << 2))) { // BTF
        if (--timeout == 0) return I2C_TIMEOUT;
    }

    // Generate STOP only if repeated_start == 0
    if (!repeated_start)
        I2C_Stop(port);

    return I2C_OK;
}
/*-----------------------------------------------------------*/

uint8 I2C_ReadStatus(uint32 base)
{
    return I2C_SR1(base);
}

/*-----------------------------------------------------------*/
I2C_Status_t I2C_MasterReceive(I2C_Port_t port, uint8 slave_addr, uint8 *data, uint16 size, uint8 repeated_start)
{
    uint32 base = I2C_GetBase(port);
    I2C_Status_t status;

    // Generate START condition
    status = I2C_Start(port);
    if (status != I2C_OK) return status;

    // Send slave address with Read bit
    status = I2C_SendAddress(port, slave_addr, I2C_READ);
    if (status != I2C_OK) return status;

    // Receive bytes
    for (uint16 i = 0; i < size; i++) {
        uint8 ack = (i < (size - 1)) ? 1 : 0;
        status = I2C_ReceiveData(port, &data[i], ack);
        if (status != I2C_OK) return status;
    }

    // Generate STOP only if repeated_start == 0
    if (!repeated_start)
        I2C_Stop(port);

    return I2C_OK;
}

void static I2C_DisableInterrupts(uint32 base)
{

    CLEAR_BIT(I2C_CR2(base), 9);  // ITBUFEN
    CLEAR_BIT(I2C_CR2(base), 10); // ITEVTEN
    CLEAR_BIT(I2C_CR2(base), 8);  // ITERREN
}

static volatile I2C_State_t I2C1_State = I2C_IDLE;

static uint8 *I2C1_TxBuffer;
static uint16 I2C1_TxSize;
static uint16 I2C1_TxIndex;
static uint8 I2C1_Address;
void I2C_MasterTransmit_IT(I2C_Port_t port, uint8 slave_addr, uint8 *data, uint16 size)
{
    uint32 base = I2C_GetBase(port);

    I2C1_State     = I2C_BUSY_TX;
    I2C1_TxBuffer  = data;
    I2C1_TxSize    = size;
    I2C1_TxIndex   = 0;
    I2C1_Address   = slave_addr << 1;  // write mode

    // Enable interrupts
    SET_BIT(I2C_CR2(base), 9);  // ITBUFEN (TXE, RXNE)
    SET_BIT(I2C_CR2(base), 10); // ITEVTEN (EV)
    SET_BIT(I2C_CR2(base), 8);  // ITERREN (ERR)

    // Start condition
    SET_BIT(I2C_CR1(base), 8);
}

void I2C1_EV_IRQHandler(void)
{
    uint32 base = I2C1_BASE;
    uint32 sr1 = I2C_SR1(base);

    // 1) START condition sent (SB)
    if (sr1 & (1<<0)) {
        I2C_DR(base) = I2C1_Address;   // Send address
        return;
    }

    // 2) Address sent (ADDR)
    if (sr1 & (1<<1)) {
        (void)I2C_SR1(base);
        (void)I2C_SR2(base);
        return;
    }

    // 3) TXE: time to send next byte
    if ((sr1 & (1<<7)) && (I2C1_State == I2C_BUSY_TX)) {
        if (I2C1_TxIndex < I2C1_TxSize) {
            I2C_DR(base) = I2C1_TxBuffer[I2C1_TxIndex++];
        }
        return;
    }

    // 4) BTF: last byte transferred → STOP
    if ((sr1 & (1<<2)) && (I2C1_TxIndex >= I2C1_TxSize)) {
        SET_BIT(I2C_CR1(base), 9);  // STOP
        I2C1_State = I2C_IDLE;
        return;
    }
    I2C_DisableInterrupts(base);
}   
void I2C1_ER_IRQHandler(void)
{
    uint32 base = I2C1_BASE;

    // Clear all errors
    I2C_SR1(base) = 0;

    I2C1_State = I2C_IDLE;
I2C_DisableInterrupts(base);
}

void I2C2_EV_IRQHandler(void)
{     


}
void I2C2_ER_IRQHandler(void)
{     


}
void I2C3_EV_IRQHandler(void)
{     


}
void I2C3_ER_IRQHandler(void)
{     


}