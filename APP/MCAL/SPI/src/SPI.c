#include "SPI.h"

/*
 * Usage examples:
 *
 *   // Master, mode 0, 8-bit, SPI1, prescaler /16, full duplex
 *   SPI_Config_t cfg = {
 *       .Instance    = SPI1,
 *       .prescaler   = 16,
 *       .mode        = SPI_MODE_0,
 *       .direction   = SPI_DIR_FULL_DUPLEX,
 *       .master      = 1,
 *       .dataSize    = SPI_DATASIZE_8,
 *       .lsbFirst    = 0,
 *       .softwareNSS = 1,
 *       .crcEnable   = 0,
 *   };
 *   SPI_Init(&cfg);
 *
 *   uint8 tx[] = { 0x9F };   // JEDEC ID command
 *   uint8 rx[3];
 *   SPI_TransceiveBlocking(SPI1, tx, rx, 1);
 */

/*------------------------------------------------------------------
 *  Internal: map prescaler value → CR1 BR[2:0]
 *------------------------------------------------------------------*/
static uint32 prescaler_to_br_bits(uint32 prescaler)
{
    switch (prescaler) {
        case   2: return (0U << SPI_CR1_BR_Pos);
        case   4: return (1U << SPI_CR1_BR_Pos);
        case   8: return (2U << SPI_CR1_BR_Pos);
        case  16: return (3U << SPI_CR1_BR_Pos);
        case  32: return (4U << SPI_CR1_BR_Pos);
        case  64: return (5U << SPI_CR1_BR_Pos);
        case 128: return (6U << SPI_CR1_BR_Pos);
        case 256: return (7U << SPI_CR1_BR_Pos);
        default:  return (3U << SPI_CR1_BR_Pos);  /* fallback: /16 */
    }
}

/*------------------------------------------------------------------
 *  Per-instance interrupt state machine
 *------------------------------------------------------------------*/
typedef enum {
    SPI_STATE_READY = 0,
    SPI_STATE_BUSY_TX,
    SPI_STATE_BUSY_RX,
    SPI_STATE_BUSY_TX_RX
} SPI_State_t;

typedef struct {
    volatile SPI_State_t state;
    const uint8         *txbuf;
    uint8               *rxbuf;
    uint32               total;
    uint32               tx_idx;
    uint32               rx_idx;
    uint8                is16;
    SPI_Callback_t       tx_cb;
    SPI_Callback_t       rx_cb;
    SPI_Callback_t       err_cb;
} SPI_Handle_t;

/* Three handles — index 0 = SPI1, 1 = SPI2, 2 = SPI3 */
static SPI_Handle_t SPI_Handle[3];

static uint8 SPI_GetIndex(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1) return 0U;
    if (SPIx == SPI2) return 1U;
    /* SPI3 */        return 2U;
}

/*------------------------------------------------------------------
 *  SPI_Init
 *------------------------------------------------------------------*/
void SPI_Init(const SPI_Config_t *cfg)
{
    if (!cfg || !cfg->Instance) return;

    SPI_TypeDef *SPIx = cfg->Instance;

    /* Disable SPI while configuring */
    SPIx->CR1 &= ~SPI_CR1_SPE;

    uint32 cr1 = 0U;
    uint32 cr2 = 0U;

    /* Master / slave */
    if (cfg->master) cr1 |= SPI_CR1_MSTR;

    /* Baud rate */
    cr1 |= prescaler_to_br_bits(cfg->prescaler);

    /* Clock mode (CPOL / CPHA) */
    switch (cfg->mode) {
        case SPI_MODE_0: break;
        case SPI_MODE_1: cr1 |= SPI_CR1_CPHA;                    break;
        case SPI_MODE_2: cr1 |= SPI_CR1_CPOL;                    break;
        case SPI_MODE_3: cr1 |= (SPI_CR1_CPOL | SPI_CR1_CPHA);   break;
    }

    /* Transfer direction */
    switch (cfg->direction) {
        case SPI_DIR_FULL_DUPLEX:    break;
        case SPI_DIR_HALF_DUPLEX_TX: cr1 |= (SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE); break;
        case SPI_DIR_HALF_DUPLEX_RX: cr1 |=  SPI_CR1_BIDIMODE;                   break;
        case SPI_DIR_RX_ONLY:        cr1 |=  SPI_CR1_RXONLY;                      break;
    }

    /* Data frame width */
    if (cfg->dataSize == SPI_DATASIZE_16) cr1 |= SPI_CR1_DFF;

    /* Bit order */
    if (cfg->lsbFirst) cr1 |= SPI_CR1_LSBFIRST;

    /* NSS management */
    if (cfg->softwareNSS) {
        cr1 |= (SPI_CR1_SSM | SPI_CR1_SSI);
    } else if (cfg->master) {
        cr2 |= SPI_CR2_SSOE;   /* drive NSS low automatically in master mode */
    }

    /* CRC */
    if (cfg->crcEnable) {
        cr1 |= SPI_CR1_CRCEN;
        SPIx->CRCPR = cfg->crcPolynomial ? cfg->crcPolynomial : 7U;
    }

    SPIx->CR1 = cr1;
    SPIx->CR2 = cr2;

    /* Flush status */
    (void)SPIx->SR;
    (void)SPIx->DR;

    /* Enable */
    SPIx->CR1 |= SPI_CR1_SPE;

    /* Reset handle state */
    SPI_Handle[SPI_GetIndex(SPIx)].state = SPI_STATE_READY;
}

/*------------------------------------------------------------------
 *  SPI_DeInit
 *------------------------------------------------------------------*/
void SPI_DeInit(SPI_TypeDef *SPIx)
{
    if (!SPIx) return;
    SPIx->CR1 &= ~SPI_CR1_SPE;
    SPIx->CR1  = 0U;
    SPIx->CR2  = 0U;
    SPI_Handle[SPI_GetIndex(SPIx)].state = SPI_STATE_READY;
}

/*------------------------------------------------------------------
 *  Blocking transmit (8/16-bit aware)
 *------------------------------------------------------------------*/
int SPI_TransmitBlocking(SPI_TypeDef *SPIx, const void *txbuf, uint32 words)
{
    if (!SPIx || !txbuf) return -1;
    int is16 = (SPIx->CR1 & SPI_CR1_DFF) ? 1 : 0;

    if (is16) {
        const uint16 *p = (const uint16 *)txbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            SPIx->DR = *p++;
        }
    } else {
        const uint8 *p = (const uint8 *)txbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            *(volatile uint8 *)&SPIx->DR = *p++;
        }
    }

    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/*------------------------------------------------------------------
 *  Blocking receive (sends dummy frames to clock data in)
 *------------------------------------------------------------------*/
int SPI_ReceiveBlocking(SPI_TypeDef *SPIx, void *rxbuf, uint32 words)
{
    if (!SPIx || !rxbuf) return -1;
    int is16 = (SPIx->CR1 & SPI_CR1_DFF) ? 1 : 0;

    if (is16) {
        uint16 *p = (uint16 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            SPIx->DR = 0xFFFFU;
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *p++ = (uint16)(SPIx->DR & 0xFFFFU);
        }
    } else {
        uint8 *p = (uint8 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            *(volatile uint8 *)&SPIx->DR = 0xFFU;
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *p++ = (uint8)(SPIx->DR & 0xFFU);
        }
    }

    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/*------------------------------------------------------------------
 *  Blocking full-duplex transceive
 *------------------------------------------------------------------*/
int SPI_TransceiveBlocking(SPI_TypeDef *SPIx, const void *txbuf,
                            void *rxbuf, uint32 words)
{
    if (!SPIx || !txbuf || !rxbuf) return -1;
    int is16 = (SPIx->CR1 & SPI_CR1_DFF) ? 1 : 0;

    if (is16) {
        const uint16 *tx = (const uint16 *)txbuf;
        uint16       *rx = (uint16 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            SPIx->DR = *tx++;
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *rx++ = (uint16)(SPIx->DR & 0xFFFFU);
        }
    } else {
        const uint8 *tx = (const uint8 *)txbuf;
        uint8       *rx = (uint8 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            *(volatile uint8 *)&SPIx->DR = *tx++;
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *rx++ = (uint8)(SPIx->DR & 0xFFU);
        }
    }

    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/*------------------------------------------------------------------
 *  Interrupt-driven transmit (non-blocking)
 *------------------------------------------------------------------*/
int SPI_Transmit_IT(SPI_TypeDef *SPIx, const void *txbuf, uint32 words)
{
    if (!SPIx || !txbuf || words == 0U) return -1;
    SPI_Handle_t *h = &SPI_Handle[SPI_GetIndex(SPIx)];
    if (h->state != SPI_STATE_READY) return -1;

    h->state  = SPI_STATE_BUSY_TX;
    h->txbuf  = (const uint8 *)txbuf;
    h->rxbuf  = NULL;
    h->total  = words;
    h->tx_idx = 0U;
    h->rx_idx = 0U;
    h->is16   = (SPIx->CR1 & SPI_CR1_DFF) ? 1U : 0U;

    SPIx->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_ERRIE);
    return 0;
}

/*------------------------------------------------------------------
 *  Interrupt-driven receive (non-blocking, sends dummy bytes)
 *------------------------------------------------------------------*/
int SPI_Receive_IT(SPI_TypeDef *SPIx, void *rxbuf, uint32 words)
{
    if (!SPIx || !rxbuf || words == 0U) return -1;
    SPI_Handle_t *h = &SPI_Handle[SPI_GetIndex(SPIx)];
    if (h->state != SPI_STATE_READY) return -1;

    h->state  = SPI_STATE_BUSY_RX;
    h->txbuf  = NULL;
    h->rxbuf  = (uint8 *)rxbuf;
    h->total  = words;
    h->tx_idx = 0U;
    h->rx_idx = 0U;
    h->is16   = (SPIx->CR1 & SPI_CR1_DFF) ? 1U : 0U;

    /* Prime the TX side with the first dummy byte to start the clock */
    if (h->is16)
        SPIx->DR = 0xFFFFU;
    else
        *(volatile uint8 *)&SPIx->DR = 0xFFU;
    h->tx_idx = 1U;

    SPIx->CR2 |= (SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE);
    return 0;
}

/*------------------------------------------------------------------
 *  Interrupt-driven full-duplex transceive (non-blocking)
 *------------------------------------------------------------------*/
int SPI_Transceive_IT(SPI_TypeDef *SPIx, const void *txbuf,
                       void *rxbuf, uint32 words)
{
    if (!SPIx || !txbuf || !rxbuf || words == 0U) return -1;
    SPI_Handle_t *h = &SPI_Handle[SPI_GetIndex(SPIx)];
    if (h->state != SPI_STATE_READY) return -1;

    h->state  = SPI_STATE_BUSY_TX_RX;
    h->txbuf  = (const uint8 *)txbuf;
    h->rxbuf  = (uint8 *)rxbuf;
    h->total  = words;
    h->tx_idx = 0U;
    h->rx_idx = 0U;
    h->is16   = (SPIx->CR1 & SPI_CR1_DFF) ? 1U : 0U;

    SPIx->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
    return 0;
}

/*------------------------------------------------------------------
 *  Callback registration
 *------------------------------------------------------------------*/
void SPI_RegisterTxCallback(SPI_TypeDef *SPIx, SPI_Callback_t cb)
{
    if (SPIx) SPI_Handle[SPI_GetIndex(SPIx)].tx_cb = cb;
}

void SPI_RegisterRxCallback(SPI_TypeDef *SPIx, SPI_Callback_t cb)
{
    if (SPIx) SPI_Handle[SPI_GetIndex(SPIx)].rx_cb = cb;
}

void SPI_RegisterErrorCallback(SPI_TypeDef *SPIx, SPI_Callback_t cb)
{
    if (SPIx) SPI_Handle[SPI_GetIndex(SPIx)].err_cb = cb;
}

/*------------------------------------------------------------------
 *  Weak default callbacks (override in application)
 *------------------------------------------------------------------*/
__attribute__((weak)) void SPI_TxCompleteCallback(SPI_TypeDef *SPIx) { (void)SPIx; }
__attribute__((weak)) void SPI_RxCompleteCallback(SPI_TypeDef *SPIx) { (void)SPIx; }
__attribute__((weak)) void SPI_ErrorCallback(SPI_TypeDef *SPIx)      { (void)SPIx; }

/*------------------------------------------------------------------
 *  Internal: shared IRQ body for all SPI instances
 *------------------------------------------------------------------*/
static void SPI_IRQ_Handler(SPI_TypeDef *SPIx)
{
    uint32        sr = SPIx->SR;
    SPI_Handle_t *h  = &SPI_Handle[SPI_GetIndex(SPIx)];

    /* ── Error ────────────────────────────────────────────────── */
    if (sr & SPI_SR_OVR) {
        (void)SPIx->DR;
        (void)SPIx->SR;
        SPIx->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
        h->state = SPI_STATE_READY;
        if (h->err_cb) h->err_cb(SPIx);
        else           SPI_ErrorCallback(SPIx);
        return;
    }

    /* ── TXE: TX register empty ───────────────────────────────── */
    if ((sr & SPI_SR_TXE) && (SPIx->CR2 & SPI_CR2_TXEIE)) {
        if (h->tx_idx < h->total) {
            /* send next byte/word */
            if (h->is16) {
                SPIx->DR = ((const uint16 *)h->txbuf)[h->tx_idx++];
            } else {
                *(volatile uint8 *)&SPIx->DR =
                    (h->txbuf != NULL) ? h->txbuf[h->tx_idx++]
                                       : (h->tx_idx++, 0xFFU);  /* dummy for RX mode */
            }
        } else {
            /* All bytes sent: disable TXE unless RX is still pending */
            SPIx->CR2 &= ~SPI_CR2_TXEIE;
            if (h->state == SPI_STATE_BUSY_TX) {
                /* TX-only: wait for BSY to clear then notify */
                while (SPIx->SR & SPI_SR_BSY) {}
                SPIx->CR2 &= ~SPI_CR2_ERRIE;
                h->state = SPI_STATE_READY;
                if (h->tx_cb) h->tx_cb(SPIx);
                else           SPI_TxCompleteCallback(SPIx);
            }
        }
    }

    /* ── RXNE: RX register not empty ─────────────────────────── */
    if ((sr & SPI_SR_RXNE) && (SPIx->CR2 & SPI_CR2_RXNEIE)) {
        if (h->rx_idx < h->total) {
            if (h->is16) {
                ((uint16 *)h->rxbuf)[h->rx_idx++] = (uint16)(SPIx->DR & 0xFFFFU);
            } else {
                h->rxbuf[h->rx_idx++] = (uint8)(SPIx->DR & 0xFFU);
            }

            /* Prime next dummy TX for pure RX mode */
            if ((h->state == SPI_STATE_BUSY_RX) && (h->tx_idx < h->total)) {
                if (h->is16)
                    SPIx->DR = 0xFFFFU;
                else
                    *(volatile uint8 *)&SPIx->DR = 0xFFU;
                h->tx_idx++;
            }

            if (h->rx_idx >= h->total) {
                /* All data received */
                SPIx->CR2 &= ~(SPI_CR2_RXNEIE | SPI_CR2_TXEIE | SPI_CR2_ERRIE);
                h->state = SPI_STATE_READY;
                if (h->rx_cb) h->rx_cb(SPIx);
                else           SPI_RxCompleteCallback(SPIx);
            }
        }
    }
}

/*------------------------------------------------------------------
 *  IRQ handlers — all three instances
 *------------------------------------------------------------------*/
void SPI1_IRQHandler(void) { SPI_IRQ_Handler(SPI1); }
void SPI2_IRQHandler(void) { SPI_IRQ_Handler(SPI2); }
void SPI3_IRQHandler(void) { SPI_IRQ_Handler(SPI3); }

/*------------------------------------------------------------------
 *  DMA helpers
 *  Note: Enable DMA2 clock via RCC before calling these.
 *------------------------------------------------------------------*/
void SPI_ConfigDMATX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream,
                     const void *mem_addr, uint32 len)
{
    if (!SPIx || !stream || !mem_addr || !len) return;

    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN) {}   /* wait for disable */

    stream->PAR  = (uint32)&(SPIx->DR);
    stream->M0AR = (uint32)mem_addr;
    stream->NDTR = len;
    stream->CR   = DMA_DIR_MEM_TO_PERIPH | DMA_SxCR_MINC | DMA_PL_HIGH | DMA_SxCR_TCIE;

    SPIx->CR2   |= SPI_CR2_TXDMAEN;
    stream->CR  |= DMA_SxCR_EN;
}

void SPI_ConfigDMARX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream,
                     void *mem_addr, uint32 len)
{
    if (!SPIx || !stream || !mem_addr || !len) return;

    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN) {}

    stream->PAR  = (uint32)&(SPIx->DR);
    stream->M0AR = (uint32)mem_addr;
    stream->NDTR = len;
    stream->CR   = DMA_DIR_PERIPH_TO_MEM | DMA_SxCR_MINC | DMA_PL_HIGH | DMA_SxCR_TCIE;

    SPIx->CR2   |= SPI_CR2_RXDMAEN;
    stream->CR  |= DMA_SxCR_EN;
}
