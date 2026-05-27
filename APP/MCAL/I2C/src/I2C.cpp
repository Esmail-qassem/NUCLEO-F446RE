/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : I2C                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "I2C.hpp"

static constexpr uint32 I2C_BASE_TABLE[3] = {
    I2C1_BASE_ADDR, I2C2_BASE_ADDR, I2C3_BASE_ADDR
};

/* ── Per-port handle ──────────────────────────────────────────────── */
struct I2C_Handle_t
{
    volatile I2C_State state    = I2C_State::IDLE;
    uint8             *buffer   = nullptr;
    uint16             size     = 0u;
    uint16             index    = 0u;
    uint16             address  = 0u;
    I2C_Callback_t     tx_cb   = nullptr;
    I2C_Callback_t     rx_cb   = nullptr;
    I2C_Callback_t     err_cb  = nullptr;
};

static I2C_Handle_t s_handle[3];

static void I2C_DisableInterrupts(I2C_RegDef_t *reg)
{
    reg->CR2 &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
}

/* ── GetReg ───────────────────────────────────────────────────────── */
I2C_RegDef_t* I2C::GetReg(I2C_Port port)
{
    if (static_cast<uint8>(port) >= static_cast<uint8>(I2C_Port::COUNT)) return nullptr;
    return reinterpret_cast<I2C_RegDef_t*>(I2C_BASE_TABLE[static_cast<uint8>(port)]);
}

/* ── Init ─────────────────────────────────────────────────────────── */
void I2C::Init(I2C_Port port, const I2C_Config_t &config)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return;

    reg->CR1 &= ~I2C_CR1_PE;

    uint32 freq_MHz = config.PCLK1_Hz / 1000000u;
    reg->CR2 = (reg->CR2 & ~0x3Fu) | (freq_MHz & 0x3Fu);

    uint32 ccr = 0u;
    if (config.ClockSpeed <= 100000u) {
        uint32 val = config.PCLK1_Hz / (2u * config.ClockSpeed);
        if (val < 4u) val = 4u;
        ccr = val & 0x0FFFu;
        reg->TRISE = freq_MHz + 1u;
    } else {
        ccr |= I2C_CCR_FS;
        uint32 val;
        if (config.DutyCycle == I2C_DutyCycle::DUTY_16_9) {
            ccr |= I2C_CCR_DUTY;
            val = config.PCLK1_Hz / (25u * config.ClockSpeed);
        } else {
            val = config.PCLK1_Hz / (3u * config.ClockSpeed);
        }
        if (val == 0u) val = 1u;
        ccr |= (val & 0x0FFFu);
        reg->TRISE = ((freq_MHz * 300u) / 1000u) + 1u;
    }
    reg->CCR = ccr;

    if (config.AddressingMode == I2C_AddressingMode::ADDR_10BIT) {
        reg->OAR1 = (1u << 15u) | (static_cast<uint32>(config.OwnAddress) & 0x3FFu);
    } else {
        reg->OAR1 = (1u << 14u) | (static_cast<uint32>(config.OwnAddress & 0x7Fu) << 1u);
    }

    reg->OAR2 = config.DualAddressMode
        ? (static_cast<uint32>(config.OwnAddress2 & 0x7Fu) << 1u) | 0x01u
        : 0u;

    uint32 cr1 = I2C_CR1_PE;
    if (config.Acknowledgement) cr1 |= I2C_CR1_ACK;
    if (config.GeneralCallMode) cr1 |= I2C_CR1_ENGC;
    if (config.NoStretchMode)   cr1 |= I2C_CR1_NOSTRETCH;
    reg->CR1 = cr1;
}

void I2C::DeInit(I2C_Port port)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return;
    reg->CR1 &= ~I2C_CR1_PE;
    I2C_DisableInterrupts(reg);
    s_handle[static_cast<uint8>(port)].state = I2C_State::IDLE;
}

/* ── Primitives ───────────────────────────────────────────────────── */
I2C_Status I2C::Start(I2C_Port port)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return I2C_Status::ERROR;
    uint32 to = I2C_TIMEOUT_MAX;
    reg->CR1 |= I2C_CR1_START;
    while (!(reg->SR1 & I2C_SR1_SB)) { if (--to == 0u) return I2C_Status::TIMEOUT; }
    return I2C_Status::OK;
}

I2C_Status I2C::Stop(I2C_Port port)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return I2C_Status::ERROR;
    reg->CR1 |= I2C_CR1_STOP;
    return I2C_Status::OK;
}

I2C_Status I2C::SendAddress(I2C_Port port, uint16 address, uint8 direction)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return I2C_Status::ERROR;
    uint32 to = I2C_TIMEOUT_MAX;
    reg->DR = (static_cast<uint32>(address) << 1u) | (direction & 0x01u);
    while (!(reg->SR1 & I2C_SR1_ADDR)) {
        if (reg->SR1 & I2C_SR1_AF) { reg->CR1 |= I2C_CR1_STOP; return I2C_Status::ERROR; }
        if (--to == 0u) return I2C_Status::TIMEOUT;
    }
    (void)reg->SR1; (void)reg->SR2;
    return I2C_Status::OK;
}

I2C_Status I2C::SendData(I2C_Port port, uint8 data)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return I2C_Status::ERROR;
    uint32 to = I2C_TIMEOUT_MAX;
    reg->DR = data;
    while (!(reg->SR1 & I2C_SR1_TXE)) { if (--to == 0u) return I2C_Status::TIMEOUT; }
    return I2C_Status::OK;
}

I2C_Status I2C::ReceiveData(I2C_Port port, uint8 &data, uint8 ack)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return I2C_Status::ERROR;
    uint32 to = I2C_TIMEOUT_MAX;
    if (ack) reg->CR1 |= I2C_CR1_ACK;
    else     reg->CR1 &= ~I2C_CR1_ACK;
    while (!(reg->SR1 & I2C_SR1_RXNE)) { if (--to == 0u) return I2C_Status::TIMEOUT; }
    data = static_cast<uint8>(reg->DR);
    return I2C_Status::OK;
}

/* ── Blocking transfers ───────────────────────────────────────────── */
I2C_Status I2C::MasterTransmit(I2C_Port port, uint16 addr, uint8 *data, uint16 size, uint8 repStart)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg) return I2C_Status::ERROR;
    I2C_Status st;
    st = Start(port);                  if (st != I2C_Status::OK) return st;
    st = SendAddress(port, addr, I2C_WRITE); if (st != I2C_Status::OK) return st;
    for (uint16 i = 0u; i < size; i++) { st = SendData(port, data[i]); if (st != I2C_Status::OK) return st; }
    uint32 to = I2C_TIMEOUT_MAX;
    while (!(reg->SR1 & I2C_SR1_BTF)) { if (--to == 0u) return I2C_Status::TIMEOUT; }
    if (!repStart) Stop(port);
    return I2C_Status::OK;
}

I2C_Status I2C::MasterReceive(I2C_Port port, uint16 addr, uint8 *data, uint16 size, uint8 repStart)
{
    I2C_Status st;
    st = Start(port);                 if (st != I2C_Status::OK) return st;
    st = SendAddress(port, addr, I2C_READ); if (st != I2C_Status::OK) return st;
    for (uint16 i = 0u; i < size; i++) {
        uint8 ack = (i < static_cast<uint16>(size - 1u)) ? 1u : 0u;
        st = ReceiveData(port, data[i], ack);
        if (st != I2C_Status::OK) return st;
    }
    if (!repStart) Stop(port);
    return I2C_Status::OK;
}

/* ── Register helpers ─────────────────────────────────────────────── */
I2C_Status I2C::WriteRegister(I2C_Port port, uint8 sa, uint8 reg, uint8 value)
{
    uint8 buf[2] = { reg, value };
    return MasterTransmit(port, static_cast<uint16>(sa), buf, 2u, 0u);
}
I2C_Status I2C::ReadRegister(I2C_Port port, uint8 sa, uint8 reg, uint8 &value)
{
    I2C_Status st = MasterTransmit(port, static_cast<uint16>(sa), &reg, 1u, 1u);
    if (st != I2C_Status::OK) return st;
    return MasterReceive(port, static_cast<uint16>(sa), &value, 1u, 0u);
}
I2C_Status I2C::ReadRegisters(I2C_Port port, uint8 sa, uint8 reg, uint8 *buf, uint16 len)
{
    I2C_Status st = MasterTransmit(port, static_cast<uint16>(sa), &reg, 1u, 1u);
    if (st != I2C_Status::OK) return st;
    return MasterReceive(port, static_cast<uint16>(sa), buf, len, 0u);
}

/* ── Callbacks / State ────────────────────────────────────────────── */
void I2C::RegisterTxCallback (I2C_Port p, I2C_Callback_t cb) { if (static_cast<uint8>(p)<3u) s_handle[static_cast<uint8>(p)].tx_cb  = cb; }
void I2C::RegisterRxCallback (I2C_Port p, I2C_Callback_t cb) { if (static_cast<uint8>(p)<3u) s_handle[static_cast<uint8>(p)].rx_cb  = cb; }
void I2C::RegisterErrCallback(I2C_Port p, I2C_Callback_t cb) { if (static_cast<uint8>(p)<3u) s_handle[static_cast<uint8>(p)].err_cb = cb; }
I2C_State I2C::GetState(I2C_Port p) { if (static_cast<uint8>(p)>=3u) return I2C_State::IDLE; return s_handle[static_cast<uint8>(p)].state; }
uint8 I2C::ReadStatus(uint32 base) { return static_cast<uint8>(reinterpret_cast<I2C_RegDef_t*>(base)->SR1); }

/* ── IT transfers ─────────────────────────────────────────────────── */
I2C_Status I2C::MasterTransmit_IT(I2C_Port port, uint16 addr, uint8 *data, uint16 size)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg || !data || size == 0u) return I2C_Status::ERROR;
    I2C_Handle_t &h = s_handle[static_cast<uint8>(port)];
    if (h.state != I2C_State::IDLE) return I2C_Status::BUSY;
    h = { I2C_State::BUSY_TX, data, size, 0u,
          static_cast<uint16>((static_cast<uint16>(addr) << 1u) | I2C_WRITE),
          h.tx_cb, h.rx_cb, h.err_cb };
    reg->CR2 |= (I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    reg->CR1 |= I2C_CR1_START;
    return I2C_Status::OK;
}

I2C_Status I2C::MasterReceive_IT(I2C_Port port, uint16 addr, uint8 *data, uint16 size)
{
    I2C_RegDef_t *reg = GetReg(port);
    if (!reg || !data || size == 0u) return I2C_Status::ERROR;
    I2C_Handle_t &h = s_handle[static_cast<uint8>(port)];
    if (h.state != I2C_State::IDLE) return I2C_Status::BUSY;
    h = { I2C_State::BUSY_RX, data, size, 0u,
          static_cast<uint16>((static_cast<uint16>(addr) << 1u) | I2C_READ),
          h.tx_cb, h.rx_cb, h.err_cb };
    reg->CR1 |= I2C_CR1_ACK;
    reg->CR2 |= (I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    reg->CR1 |= I2C_CR1_START;
    return I2C_Status::OK;
}

/* ── IRQ bodies ───────────────────────────────────────────────────── */
static void I2C_EV_Body(I2C_Port port)
{
    I2C_RegDef_t *reg = I2C::GetReg(port);
    if (!reg) return;
    I2C_Handle_t &h = s_handle[static_cast<uint8>(port)];
    uint32 sr1 = reg->SR1;

    if (sr1 & I2C_SR1_SB) { reg->DR = h.address; return; }
    if (sr1 & I2C_SR1_ADDR) { (void)reg->SR1; (void)reg->SR2; return; }
    if ((sr1 & I2C_SR1_TXE) && (h.state == I2C_State::BUSY_TX)) {
        if (h.index < h.size) { reg->DR = h.buffer[h.index++]; }
        return;
    }
    if ((sr1 & I2C_SR1_BTF) && (h.state == I2C_State::BUSY_TX) && (h.index >= h.size)) {
        reg->CR1 |= I2C_CR1_STOP;
        I2C_DisableInterrupts(reg);
        h.state = I2C_State::IDLE;
        if (h.tx_cb) h.tx_cb(port);
        return;
    }
    if ((sr1 & I2C_SR1_RXNE) && (h.state == I2C_State::BUSY_RX)) {
        if (h.index < h.size) {
            if (h.index == static_cast<uint16>(h.size - 1u)) {
                reg->CR1 &= ~I2C_CR1_ACK;
                reg->CR1 |=  I2C_CR1_STOP;
            }
            h.buffer[h.index++] = static_cast<uint8>(reg->DR);
            if (h.index >= h.size) {
                I2C_DisableInterrupts(reg);
                h.state = I2C_State::IDLE;
                if (h.rx_cb) h.rx_cb(port);
            }
        }
        return;
    }
    I2C_DisableInterrupts(reg);
}

static void I2C_ER_Body(I2C_Port port)
{
    I2C_RegDef_t *reg = I2C::GetReg(port);
    if (!reg) return;
    reg->SR1 = 0u;
    I2C_Handle_t &h = s_handle[static_cast<uint8>(port)];
    h.state = I2C_State::IDLE;
    I2C_DisableInterrupts(reg);
    if (h.err_cb) h.err_cb(port);
}

extern "C" void I2C1_EV_IRQHandler(void) { I2C_EV_Body(I2C_Port::I2C1); }
extern "C" void I2C1_ER_IRQHandler(void) { I2C_ER_Body(I2C_Port::I2C1); }
extern "C" void I2C2_EV_IRQHandler(void) { I2C_EV_Body(I2C_Port::I2C2); }
extern "C" void I2C2_ER_IRQHandler(void) { I2C_ER_Body(I2C_Port::I2C2); }
extern "C" void I2C3_EV_IRQHandler(void) { I2C_EV_Body(I2C_Port::I2C3); }
extern "C" void I2C3_ER_IRQHandler(void) { I2C_ER_Body(I2C_Port::I2C3); }
