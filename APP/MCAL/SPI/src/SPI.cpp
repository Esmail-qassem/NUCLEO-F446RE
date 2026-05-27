/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : SPI                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "SPI.hpp"

/* ── Internal state machine ───────────────────────────────────────── */
enum class SPI_State : uint8
{
    READY = 0,
    BUSY_TX,
    BUSY_RX,
    BUSY_TX_RX
};

struct SPI_Handle_t
{
    volatile SPI_State state;
    const uint8       *txbuf;
    uint8             *rxbuf;
    uint32             total;
    uint32             tx_idx;
    uint32             rx_idx;
    uint8              is16;
    SPI_Callback_t     tx_cb;
    SPI_Callback_t     rx_cb;
    SPI_Callback_t     err_cb;
};

static SPI_Handle_t SPI_Handle[3];

/* ── GetIndex ─────────────────────────────────────────────────────── */
uint8 SPI::GetIndex(SPI_RegDef_t *SPIx)
{
    if (SPIx == SPI1_REG) return 0U;
    if (SPIx == SPI2_REG) return 1U;
    return 2U;
}

/* ── prescaler_to_br_bits helper ─────────────────────────────────── */
static uint32 prescaler_to_br_bits(uint32 prescaler)
{
    switch (prescaler)
    {
        case   2U: return (0U << SPI_CR1_BR_Pos);
        case   4U: return (1U << SPI_CR1_BR_Pos);
        case   8U: return (2U << SPI_CR1_BR_Pos);
        case  16U: return (3U << SPI_CR1_BR_Pos);
        case  32U: return (4U << SPI_CR1_BR_Pos);
        case  64U: return (5U << SPI_CR1_BR_Pos);
        case 128U: return (6U << SPI_CR1_BR_Pos);
        case 256U: return (7U << SPI_CR1_BR_Pos);
        default:   return (3U << SPI_CR1_BR_Pos);
    }
}

/* ── Init ─────────────────────────────────────────────────────────── */
void SPI::Init(const SPI_Config_t &cfg)
{
    if (!cfg.Instance) return;
    SPI_RegDef_t *SPIx = cfg.Instance;

    SPIx->CR1 &= ~SPI_CR1_SPE;

    uint32 cr1 = 0U;
    uint32 cr2 = 0U;

    if (cfg.master) cr1 |= SPI_CR1_MSTR;
    cr1 |= prescaler_to_br_bits(cfg.prescaler);

    switch (cfg.mode)
    {
        case SPI_Mode::MODE_0: break;
        case SPI_Mode::MODE_1: cr1 |= SPI_CR1_CPHA;                     break;
        case SPI_Mode::MODE_2: cr1 |= SPI_CR1_CPOL;                     break;
        case SPI_Mode::MODE_3: cr1 |= (SPI_CR1_CPOL | SPI_CR1_CPHA);    break;
    }

    switch (cfg.direction)
    {
        case SPI_Direction::FULL_DUPLEX:    break;
        case SPI_Direction::HALF_DUPLEX_TX: cr1 |= (SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE); break;
        case SPI_Direction::HALF_DUPLEX_RX: cr1 |=  SPI_CR1_BIDIMODE;                   break;
        case SPI_Direction::RX_ONLY:        cr1 |=  SPI_CR1_RXONLY;                      break;
    }

    if (cfg.dataSize == SPI_DataSize::DATASIZE_16) cr1 |= SPI_CR1_DFF;
    if (cfg.lsbFirst) cr1 |= SPI_CR1_LSBFIRST;

    if (cfg.softwareNSS)
        cr1 |= (SPI_CR1_SSM | SPI_CR1_SSI);
    else if (cfg.master)
        cr2 |= SPI_CR2_SSOE;

    if (cfg.crcEnable)
    {
        cr1 |= SPI_CR1_CRCEN;
        SPIx->CRCPR = cfg.crcPolynomial ? static_cast<uint32>(cfg.crcPolynomial) : 7U;
    }

    SPIx->CR1 = cr1;
    SPIx->CR2 = cr2;
    (void)SPIx->SR;
    (void)SPIx->DR;
    SPIx->CR1 |= SPI_CR1_SPE;

    SPI_Handle[GetIndex(SPIx)].state = SPI_State::READY;
}

/* ── DeInit ───────────────────────────────────────────────────────── */
void SPI::DeInit(SPI_RegDef_t *SPIx)
{
    if (!SPIx) return;
    SPIx->CR1 &= ~SPI_CR1_SPE;
    SPIx->CR1  = 0U;
    SPIx->CR2  = 0U;
    SPI_Handle[GetIndex(SPIx)].state = SPI_State::READY;
}

/* ── TransmitBlocking ─────────────────────────────────────────────── */
int SPI::TransmitBlocking(SPI_RegDef_t *SPIx, const void *txbuf, uint32 words)
{
    if (!SPIx || !txbuf) return -1;
    bool is16 = (SPIx->CR1 & SPI_CR1_DFF) != 0U;

    if (is16)
    {
        const uint16 *p = reinterpret_cast<const uint16*>(txbuf);
        while (words--)
        {
            while (!(SPIx->SR & SPI_SR_TXE)) { if (SPIx->SR & SPI_SR_OVR) return -2; }
            SPIx->DR = *p++;
        }
    }
    else
    {
        const uint8 *p = reinterpret_cast<const uint8*>(txbuf);
        while (words--)
        {
            while (!(SPIx->SR & SPI_SR_TXE)) { if (SPIx->SR & SPI_SR_OVR) return -2; }
            *reinterpret_cast<volatile uint8*>(&SPIx->DR) = *p++;
        }
    }
    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/* ── ReceiveBlocking ──────────────────────────────────────────────── */
int SPI::ReceiveBlocking(SPI_RegDef_t *SPIx, void *rxbuf, uint32 words)
{
    if (!SPIx || !rxbuf) return -1;
    bool is16 = (SPIx->CR1 & SPI_CR1_DFF) != 0U;

    if (is16)
    {
        uint16 *p = reinterpret_cast<uint16*>(rxbuf);
        while (words--)
        {
            while (!(SPIx->SR & SPI_SR_TXE)) { if (SPIx->SR & SPI_SR_OVR) return -2; }
            SPIx->DR = 0xFFFFU;
            while (!(SPIx->SR & SPI_SR_RXNE)) { if (SPIx->SR & SPI_SR_OVR) return -3; }
            *p++ = static_cast<uint16>(SPIx->DR & 0xFFFFU);
        }
    }
    else
    {
        uint8 *p = reinterpret_cast<uint8*>(rxbuf);
        while (words--)
        {
            while (!(SPIx->SR & SPI_SR_TXE)) { if (SPIx->SR & SPI_SR_OVR) return -2; }
            *reinterpret_cast<volatile uint8*>(&SPIx->DR) = 0xFFU;
            while (!(SPIx->SR & SPI_SR_RXNE)) { if (SPIx->SR & SPI_SR_OVR) return -3; }
            *p++ = static_cast<uint8>(SPIx->DR & 0xFFU);
        }
    }
    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/* ── TransceiveBlocking ───────────────────────────────────────────── */
int SPI::TransceiveBlocking(SPI_RegDef_t *SPIx, const void *txbuf, void *rxbuf, uint32 words)
{
    if (!SPIx || !txbuf || !rxbuf) return -1;
    bool is16 = (SPIx->CR1 & SPI_CR1_DFF) != 0U;

    if (is16)
    {
        const uint16 *tx = reinterpret_cast<const uint16*>(txbuf);
        uint16       *rx = reinterpret_cast<uint16*>(rxbuf);
        while (words--)
        {
            while (!(SPIx->SR & SPI_SR_TXE)) { if (SPIx->SR & SPI_SR_OVR) return -2; }
            SPIx->DR = *tx++;
            while (!(SPIx->SR & SPI_SR_RXNE)) { if (SPIx->SR & SPI_SR_OVR) return -3; }
            *rx++ = static_cast<uint16>(SPIx->DR & 0xFFFFU);
        }
    }
    else
    {
        const uint8 *tx = reinterpret_cast<const uint8*>(txbuf);
        uint8       *rx = reinterpret_cast<uint8*>(rxbuf);
        while (words--)
        {
            while (!(SPIx->SR & SPI_SR_TXE)) { if (SPIx->SR & SPI_SR_OVR) return -2; }
            *reinterpret_cast<volatile uint8*>(&SPIx->DR) = *tx++;
            while (!(SPIx->SR & SPI_SR_RXNE)) { if (SPIx->SR & SPI_SR_OVR) return -3; }
            *rx++ = static_cast<uint8>(SPIx->DR & 0xFFU);
        }
    }
    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/* ── Transmit_IT ──────────────────────────────────────────────────── */
int SPI::Transmit_IT(SPI_RegDef_t *SPIx, const void *txbuf, uint32 words)
{
    if (!SPIx || !txbuf || words == 0U) return -1;
    SPI_Handle_t *h = &SPI_Handle[GetIndex(SPIx)];
    if (h->state != SPI_State::READY) return -1;
    h->state  = SPI_State::BUSY_TX;
    h->txbuf  = reinterpret_cast<const uint8*>(txbuf);
    h->rxbuf  = nullptr;
    h->total  = words; h->tx_idx = 0U; h->rx_idx = 0U;
    h->is16   = (SPIx->CR1 & SPI_CR1_DFF) ? 1U : 0U;
    SPIx->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_ERRIE);
    return 0;
}

/* ── Receive_IT ───────────────────────────────────────────────────── */
int SPI::Receive_IT(SPI_RegDef_t *SPIx, void *rxbuf, uint32 words)
{
    if (!SPIx || !rxbuf || words == 0U) return -1;
    SPI_Handle_t *h = &SPI_Handle[GetIndex(SPIx)];
    if (h->state != SPI_State::READY) return -1;
    h->state  = SPI_State::BUSY_RX;
    h->txbuf  = nullptr;
    h->rxbuf  = reinterpret_cast<uint8*>(rxbuf);
    h->total  = words; h->tx_idx = 0U; h->rx_idx = 0U;
    h->is16   = (SPIx->CR1 & SPI_CR1_DFF) ? 1U : 0U;
    if (h->is16) SPIx->DR = 0xFFFFU;
    else *reinterpret_cast<volatile uint8*>(&SPIx->DR) = 0xFFU;
    h->tx_idx = 1U;
    SPIx->CR2 |= (SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE);
    return 0;
}

/* ── Transceive_IT ────────────────────────────────────────────────── */
int SPI::Transceive_IT(SPI_RegDef_t *SPIx, const void *txbuf, void *rxbuf, uint32 words)
{
    if (!SPIx || !txbuf || !rxbuf || words == 0U) return -1;
    SPI_Handle_t *h = &SPI_Handle[GetIndex(SPIx)];
    if (h->state != SPI_State::READY) return -1;
    h->state  = SPI_State::BUSY_TX_RX;
    h->txbuf  = reinterpret_cast<const uint8*>(txbuf);
    h->rxbuf  = reinterpret_cast<uint8*>(rxbuf);
    h->total  = words; h->tx_idx = 0U; h->rx_idx = 0U;
    h->is16   = (SPIx->CR1 & SPI_CR1_DFF) ? 1U : 0U;
    SPIx->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    return 0;
}

/* ── Callback registration ────────────────────────────────────────── */
void SPI::RegisterTxCallback   (SPI_RegDef_t *SPIx, SPI_Callback_t cb) { if (SPIx) SPI_Handle[GetIndex(SPIx)].tx_cb  = cb; }
void SPI::RegisterRxCallback   (SPI_RegDef_t *SPIx, SPI_Callback_t cb) { if (SPIx) SPI_Handle[GetIndex(SPIx)].rx_cb  = cb; }
void SPI::RegisterErrorCallback(SPI_RegDef_t *SPIx, SPI_Callback_t cb) { if (SPIx) SPI_Handle[GetIndex(SPIx)].err_cb = cb; }

/* ── Weak default callbacks ───────────────────────────────────────── */
__attribute__((weak)) void SPI::TxCompleteCallback(SPI_RegDef_t *SPIx) { (void)SPIx; }
__attribute__((weak)) void SPI::RxCompleteCallback(SPI_RegDef_t *SPIx) { (void)SPIx; }
__attribute__((weak)) void SPI::ErrorCallback     (SPI_RegDef_t *SPIx) { (void)SPIx; }

/* ── IRQ_Handler ──────────────────────────────────────────────────── */
void SPI::IRQ_Handler(SPI_RegDef_t *SPIx)
{
    uint32        sr = SPIx->SR;
    SPI_Handle_t *h  = &SPI_Handle[GetIndex(SPIx)];

    if (sr & SPI_SR_OVR)
    {
        (void)SPIx->DR; (void)SPIx->SR;
        SPIx->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
        h->state = SPI_State::READY;
        if (h->err_cb) h->err_cb(SPIx); else SPI::ErrorCallback(SPIx);
        return;
    }

    if ((sr & SPI_SR_TXE) && (SPIx->CR2 & SPI_CR2_TXEIE))
    {
        if (h->tx_idx < h->total)
        {
            if (h->is16)
                SPIx->DR = reinterpret_cast<const uint16*>(h->txbuf)[h->tx_idx++];
            else
                *reinterpret_cast<volatile uint8*>(&SPIx->DR) =
                    (h->txbuf != nullptr) ? h->txbuf[h->tx_idx++]
                                          : (h->tx_idx++, static_cast<uint8>(0xFFU));
        }
        else
        {
            SPIx->CR2 &= ~SPI_CR2_TXEIE;
            if (h->state == SPI_State::BUSY_TX)
            {
                while (SPIx->SR & SPI_SR_BSY) {}
                SPIx->CR2 &= ~SPI_CR2_ERRIE;
                h->state = SPI_State::READY;
                if (h->tx_cb) h->tx_cb(SPIx); else SPI::TxCompleteCallback(SPIx);
            }
        }
    }

    if ((sr & SPI_SR_RXNE) && (SPIx->CR2 & SPI_CR2_RXNEIE))
    {
        if (h->rx_idx < h->total)
        {
            if (h->is16)
                reinterpret_cast<uint16*>(h->rxbuf)[h->rx_idx++] = static_cast<uint16>(SPIx->DR & 0xFFFFU);
            else
                h->rxbuf[h->rx_idx++] = static_cast<uint8>(SPIx->DR & 0xFFU);

            if ((h->state == SPI_State::BUSY_RX) && (h->tx_idx < h->total))
            {
                if (h->is16) SPIx->DR = 0xFFFFU;
                else *reinterpret_cast<volatile uint8*>(&SPIx->DR) = 0xFFU;
                h->tx_idx++;
            }

            if (h->rx_idx >= h->total)
            {
                SPIx->CR2 &= ~(SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE);
                h->state = SPI_State::READY;
                if (h->rx_cb) h->rx_cb(SPIx); else SPI::RxCompleteCallback(SPIx);
            }
        }
    }
}

/* ── IRQ handlers ─────────────────────────────────────────────────── */
extern "C" void SPI1_IRQHandler(void) { SPI::IRQ_Handler(SPI1_REG); }
extern "C" void SPI2_IRQHandler(void) { SPI::IRQ_Handler(SPI2_REG); }
extern "C" void SPI3_IRQHandler(void) { SPI::IRQ_Handler(SPI3_REG); }

/* ── DMA helpers ──────────────────────────────────────────────────── */
void SPI::ConfigDMATX(SPI_RegDef_t *SPIx, DMA_Stream_t *stream, const void *mem_addr, uint32 len)
{
    if (!SPIx || !stream || !mem_addr || !len) return;
    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN) {}
    stream->PAR  = reinterpret_cast<uint32>(&SPIx->DR);
    stream->M0AR = reinterpret_cast<uint32>(mem_addr);
    stream->NDTR = len;
    stream->CR   = DMA_DIR_MEM_TO_PERIPH | DMA_SxCR_MINC | DMA_PL_HIGH | DMA_SxCR_TCIE;
    SPIx->CR2   |= SPI_CR2_TXDMAEN;
    stream->CR  |= DMA_SxCR_EN;
}

void SPI::ConfigDMARX(SPI_RegDef_t *SPIx, DMA_Stream_t *stream, void *mem_addr, uint32 len)
{
    if (!SPIx || !stream || !mem_addr || !len) return;
    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN) {}
    stream->PAR  = reinterpret_cast<uint32>(&SPIx->DR);
    stream->M0AR = reinterpret_cast<uint32>(mem_addr);
    stream->NDTR = len;
    stream->CR   = DMA_DIR_PERIPH_TO_MEM | DMA_SxCR_MINC | DMA_PL_HIGH | DMA_SxCR_TCIE;
    SPIx->CR2   |= SPI_CR2_RXDMAEN;
    stream->CR  |= DMA_SxCR_EN;
}
