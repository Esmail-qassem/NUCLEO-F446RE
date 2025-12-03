#include "spi.h"

/* ---------- Helper: map prescaler value (2..256) to CR1 BR bits ---------- */
static uint32 prescaler_to_br_bits(uint32 prescaler)
{
    switch (prescaler) {
        case 2:   return (0U << SPI_CR1_BR_Pos);
        case 4:   return (1U << SPI_CR1_BR_Pos);
        case 8:   return (2U << SPI_CR1_BR_Pos);
        case 16:  return (3U << SPI_CR1_BR_Pos);
        case 32:  return (4U << SPI_CR1_BR_Pos);
        case 64:  return (5U << SPI_CR1_BR_Pos);
        case 128: return (6U << SPI_CR1_BR_Pos);
        case 256: return (7U << SPI_CR1_BR_Pos);
        default:  return (3U << SPI_CR1_BR_Pos); /* default divide by 16 */
    }
}
void static SPI_Enable(SPI_TypeDef *SPIx)
{
    if (!SPIx) return;
    SPIx->CR1 |= SPI_CR1_SPE;
}
/* ---------- Public API implementations ---------- */

void SPI_Init(const SPI_Config_t *cfg)
{
    if (!cfg || !cfg->Instance) return;

    SPI_TypeDef *SPIx = cfg->Instance;

    /* disable SPI while configuring */
    SPIx->CR1 &= ~SPI_CR1_SPE;

    /* build CR1 and CR2 */
    uint32 cr1 = 0;
    uint32 cr2 = 0;

    /* master/slave */
    if (cfg->master) cr1 |= SPI_CR1_MSTR;

    /* prescaler */
    cr1 |= prescaler_to_br_bits(cfg->prescaler);

    /* mode (CPOL/CPHA) */
    switch (cfg->mode) {
        case SPI_MODE_0: break;
        case SPI_MODE_1: cr1 |= SPI_CR1_CPHA; break;
        case SPI_MODE_2: cr1 |= SPI_CR1_CPOL; break;
        case SPI_MODE_3: cr1 |= (SPI_CR1_CPOL | SPI_CR1_CPHA); break;
    }

    /* data size */
    if (cfg->dataSize == SPI_DATASIZE_16) cr1 |= SPI_CR1_DFF;

    /* first bit */
    if (cfg->lsbFirst) cr1 |= SPI_CR1_LSBFIRST;

    /* software NSS */
    if (cfg->softwareNSS) cr1 |= (SPI_CR1_SSM | SPI_CR1_SSI);

    /* enable SSOE if hardware NSS and master (user must wire NSS pin) */
    if (!cfg->softwareNSS && cfg->master) cr2 |= SPI_CR2_SSOE;

    SPIx->CR1 = cr1;
    SPIx->CR2 = cr2;

    /* clear status by reading SR and DR */
    (void)SPIx->SR;
    (void)SPIx->DR;
    SPI_Enable(SPIx);
}


/* ---------- Blocking transmit (8/16-bit aware) ---------- */
int SPI_TransmitBlocking(SPI_TypeDef *SPIx, const void *txbuf, uint32 words)
{
    if (!SPIx || !txbuf) return -1;
    int is16 = (SPIx->CR1 & SPI_CR1_DFF) ? 1 : 0;

    if (is16) {
        const uint16 *p = (const uint16 *)txbuf;
        while (words--) {
            /* wait TXE */
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

    /* wait until not busy */
    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/* ---------- Blocking receive (sends dummy frames) ---------- */
int SPI_ReceiveBlocking(SPI_TypeDef *SPIx, void *rxbuf, uint32 words)
{
    if (!SPIx || !rxbuf) return -1;
    int is16 = (SPIx->CR1 & SPI_CR1_DFF) ? 1 : 0;

    if (is16) {
        uint16 *p = (uint16 *)rxbuf;
        while (words--) {
            /* send dummy */
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            SPIx->DR = 0xFFFF;

            /* wait for data */
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *p++ = (uint16)(SPIx->DR & 0xFFFF);
        }
    } else {
        uint8 *p = (uint8 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            *(volatile uint8 *)&SPIx->DR = 0xFF;

            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *p++ = (uint8)(SPIx->DR & 0xFF);
        }
    }

    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/* ---------- Blocking full-duplex transceive ---------- */
int SPI_TransceiveBlocking(SPI_TypeDef *SPIx, const void *txbuf, void *rxbuf, uint32 words)
{
    if (!SPIx || !txbuf || !rxbuf) return -1;
    int is16 = (SPIx->CR1 & SPI_CR1_DFF) ? 1 : 0;

    if (is16) {
        const uint16 *tx = (const uint16 *)txbuf;
        uint16 *rx = (uint16 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            SPIx->DR = *tx++;
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *rx++ = (uint16)(SPIx->DR & 0xFFFF);
        }
    } else {
        const uint8 *tx = (const uint8 *)txbuf;
        uint8 *rx = (uint8 *)rxbuf;
        while (words--) {
            while (!(SPIx->SR & SPI_SR_TXE)) {
                if (SPIx->SR & SPI_SR_OVR) return -2;
            }
            *(volatile uint8 *)&SPIx->DR = *tx++;
            while (!(SPIx->SR & SPI_SR_RXNE)) {
                if (SPIx->SR & SPI_SR_OVR) return -3;
            }
            *rx++ = (uint8)(SPIx->DR & 0xFF);
        }
    }

    while (SPIx->SR & SPI_SR_BSY) {}
    return 0;
}

/* ---------- Simple DMA support ---------- */

/* Configure DMA for SPI TX:
   - stream: pointer to DMA stream (e.g. DMA2_Stream3)
   - mem_addr: pointer to source buffer in memory
   - len: number of frames (bytes if 8-bit, halfwords if 16-bit) */
void SPI_ConfigDMATX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream, const void *mem_addr, uint32 len)
{
    if (!SPIx || !stream || !mem_addr || !len) return;

    /* disable stream before configure */
    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN) {}

    /* peripheral address = SPIx->DR */
    stream->PAR = (uint32)&(SPIx->DR);
    stream->M0AR = (uint32)mem_addr;
    stream->NDTR = (uint32)len;

    /* configure CR:
       - memory increment
       - direction memory->periph
       - high priority
       - peripheral & memory sizes set later if needed (we keep 8-bit default)
    */
    uint32 cr = 0;
    cr |= DMA_DIR_MEM_TO_PERIPH;
    cr |= DMA_SxCR_MINC;
    cr |= DMA_PL_HIGH;
    cr |= DMA_SxCR_TCIE; /* transfer complete interrupt if NVIC implemented by user */
    stream->CR = cr;

    /* enable SPI DMA request (TX) */
    SPIx->CR2 |= SPI_CR2_TXDMAEN;

    /* enable stream */
    stream->CR |= DMA_SxCR_EN;
}

/* Configure DMA for SPI RX:
   - stream: pointer to DMA stream (e.g. DMA2_Stream2)
   - mem_addr: pointer to destination buffer in memory
   - len: number of frames */
void SPI_ConfigDMARX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream, void *mem_addr, uint32 len)
{
    if (!SPIx || !stream || !mem_addr || !len) return;

    stream->CR &= ~DMA_SxCR_EN;
    while (stream->CR & DMA_SxCR_EN) {}

    stream->PAR = (uint32)&(SPIx->DR);
    stream->M0AR = (uint32)mem_addr;
    stream->NDTR = (uint32)len;

    uint32 cr = 0;
    cr |= DMA_DIR_PERIPH_TO_MEM;
    cr |= DMA_SxCR_MINC;
    cr |= DMA_PL_HIGH;
    cr |= DMA_SxCR_TCIE;
    stream->CR = cr;

    /* enable SPI DMA request (RX) */
    SPIx->CR2 |= SPI_CR2_RXDMAEN;

    stream->CR |= DMA_SxCR_EN;
}

/* ---------- Weak callbacks (user may override) ---------- */
// __attribute__((weak)) void SPI_DMA_TxCompleteCallback(SPI_TypeDef *SPIx) { (void)SPIx; }
// __attribute__((weak)) void SPI_DMA_RxCompleteCallback(SPI_TypeDef *SPIx) { (void)SPIx; }
// __attribute__((weak)) void SPI_ErrorCallback(SPI_TypeDef *SPIx) { (void)SPIx; }

/* ---------- Example minimal IRQ handlers the user can adapt ----------
   These would normally be installed in your vector table. They must check
   the DMA interrupt flags and call the callbacks. Implementing full NVIC/
   DMA ISR logic requires reading DMA_LISR/HISR and clearing flags in LIFCR/HIFCR.
   Here we only provide a tiny skeleton; adapt to your project's NVIC setup.
--------------------------------------------------------------------------*/

void SPI1_IRQHandler(void)
{
    /* simple error check: OVR */
    if (SPI1->SR & SPI_SR_OVR) {
        /* clear OVR by read DR and SR */
        (void)SPI1->DR;
        (void)SPI1->SR;
        SPI_ErrorCallback(SPI1);
    }
}
void SPI2_IRQHandler                   (void) 
{



}
void SPI3_IRQHandler                   (void) 
{


    
}

/* DMA stream handlers skeleton (real interrupts must inspect LISR/HISR flags)
   Provide proper handlers depending on your linker and vector table. */

void DMA2_Stream3_IRQHandler(void)
{
    /* TX complete for stream3: user should clear flags in LIFCR/HIFCR here */
    SPI_DMA_TxCompleteCallback(SPI1); /* commonly used for SPI1 TX */
}

void DMA2_Stream2_IRQHandler(void)
{
    /* RX complete for stream2 */
    SPI_DMA_RxCompleteCallback(SPI1);
}
