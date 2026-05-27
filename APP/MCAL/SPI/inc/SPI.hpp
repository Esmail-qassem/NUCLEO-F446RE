/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : SPI                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Base addresses ───────────────────────────────────────────────── */
constexpr uint32 SPI1_BASE_ADDR = 0x40013000UL;
constexpr uint32 SPI2_BASE_ADDR = 0x40003800UL;
constexpr uint32 SPI3_BASE_ADDR = 0x40003C00UL;
constexpr uint32 DMA2_BASE_ADDR = 0x40026400UL;

/* ── SPI register map ─────────────────────────────────────────────── */
struct SPI_RegDef_t
{
    volatile uint32 CR1;      /* 0x00 */
    volatile uint32 CR2;      /* 0x04 */
    volatile uint32 SR;       /* 0x08 */
    volatile uint32 DR;       /* 0x0C */
    volatile uint32 CRCPR;    /* 0x10 */
    volatile uint32 RXCRCR;   /* 0x14 */
    volatile uint32 TXCRCR;   /* 0x18 */
    volatile uint32 I2SCFGR;  /* 0x1C */
    volatile uint32 I2SPR;    /* 0x20 */
};

/* ── DMA stream register map ──────────────────────────────────────── */
struct DMA_Stream_t
{
    volatile uint32 CR;    /* 0x00 */
    volatile uint32 NDTR;  /* 0x04 */
    volatile uint32 PAR;   /* 0x08 */
    volatile uint32 M0AR;  /* 0x0C */
    volatile uint32 M1AR;  /* 0x10 */
    volatile uint32 FCR;   /* 0x14 */
};

/* ── Peripheral pointers ──────────────────────────────────────────── */
#define SPI1_REG  (reinterpret_cast<SPI_RegDef_t*>(SPI1_BASE_ADDR))
#define SPI2_REG  (reinterpret_cast<SPI_RegDef_t*>(SPI2_BASE_ADDR))
#define SPI3_REG  (reinterpret_cast<SPI_RegDef_t*>(SPI3_BASE_ADDR))

#define DMA2_Stream2_REG  (reinterpret_cast<DMA_Stream_t*>(DMA2_BASE_ADDR + 0x040U))
#define DMA2_Stream3_REG  (reinterpret_cast<DMA_Stream_t*>(DMA2_BASE_ADDR + 0x058U))

/* ── CR1 bit masks ────────────────────────────────────────────────── */
constexpr uint32 SPI_CR1_CPHA     = (1U <<  0U);
constexpr uint32 SPI_CR1_CPOL     = (1U <<  1U);
constexpr uint32 SPI_CR1_MSTR     = (1U <<  2U);
constexpr uint8  SPI_CR1_BR_Pos   = 3U;
constexpr uint32 SPI_CR1_SPE      = (1U <<  6U);
constexpr uint32 SPI_CR1_LSBFIRST = (1U <<  7U);
constexpr uint32 SPI_CR1_SSI      = (1U <<  8U);
constexpr uint32 SPI_CR1_SSM      = (1U <<  9U);
constexpr uint32 SPI_CR1_RXONLY   = (1U << 10U);
constexpr uint32 SPI_CR1_DFF      = (1U << 11U);
constexpr uint32 SPI_CR1_CRCNEXT  = (1U << 12U);
constexpr uint32 SPI_CR1_CRCEN    = (1U << 13U);
constexpr uint32 SPI_CR1_BIDIOE   = (1U << 14U);
constexpr uint32 SPI_CR1_BIDIMODE = (1U << 15U);

/* ── CR2 bit masks ────────────────────────────────────────────────── */
constexpr uint32 SPI_CR2_RXDMAEN = (1U << 0U);
constexpr uint32 SPI_CR2_TXDMAEN = (1U << 1U);
constexpr uint32 SPI_CR2_SSOE    = (1U << 2U);
constexpr uint32 SPI_CR2_ERRIE   = (1U << 5U);
constexpr uint32 SPI_CR2_RXNEIE  = (1U << 6U);
constexpr uint32 SPI_CR2_TXEIE   = (1U << 7U);

/* ── SR bit masks ─────────────────────────────────────────────────── */
constexpr uint32 SPI_SR_RXNE = (1U << 0U);
constexpr uint32 SPI_SR_TXE  = (1U << 1U);
constexpr uint32 SPI_SR_OVR  = (1U << 6U);
constexpr uint32 SPI_SR_BSY  = (1U << 7U);

/* ── DMA CR bits ──────────────────────────────────────────────────── */
constexpr uint32 DMA_SxCR_EN   = (1U <<  0U);
constexpr uint8  DMA_SxCR_DIR_Pos = 6U;
constexpr uint32 DMA_SxCR_MINC = (1U << 10U);
constexpr uint32 DMA_SxCR_TCIE = (1U <<  4U);
constexpr uint8  DMA_SxCR_PL_Pos  = 16U;
constexpr uint32 DMA_DIR_PERIPH_TO_MEM = (0U << 6U);
constexpr uint32 DMA_DIR_MEM_TO_PERIPH = (1U << 6U);
constexpr uint32 DMA_PL_HIGH           = (2U << 16U);

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class SPI_Mode : uint8
{
    MODE_0 = 0,
    MODE_1,
    MODE_2,
    MODE_3
};

enum class SPI_DataSize : uint8
{
    DATASIZE_8  = 0,
    DATASIZE_16 = 1
};

enum class SPI_Direction : uint8
{
    FULL_DUPLEX    = 0,
    HALF_DUPLEX_TX = 1,
    HALF_DUPLEX_RX = 2,
    RX_ONLY        = 3
};

/* ── Callback type ────────────────────────────────────────────────── */
using SPI_Callback_t = void(*)(SPI_RegDef_t *SPIx);

/* ── Configuration structure ──────────────────────────────────────── */
struct SPI_Config_t
{
    SPI_RegDef_t  *Instance;
    uint32         prescaler;
    SPI_Mode       mode;
    SPI_Direction  direction;
    uint8          master;
    SPI_DataSize   dataSize;
    uint8          lsbFirst;
    uint8          softwareNSS;
    uint8          crcEnable;
    uint16         crcPolynomial;
};

/* ── IRQ handlers ─────────────────────────────────────────────────── */
extern "C" {
    void SPI1_IRQHandler(void);
    void SPI2_IRQHandler(void);
    void SPI3_IRQHandler(void);
}

/* ── SPI Driver Class ─────────────────────────────────────────────── */
class SPI
{
public:
    static void Init           (const SPI_Config_t &cfg);
    static void DeInit         (SPI_RegDef_t *SPIx);

    static int  TransmitBlocking  (SPI_RegDef_t *SPIx, const void *txbuf, uint32 words);
    static int  ReceiveBlocking   (SPI_RegDef_t *SPIx, void *rxbuf, uint32 words);
    static int  TransceiveBlocking(SPI_RegDef_t *SPIx, const void *txbuf, void *rxbuf, uint32 words);

    static int  Transmit_IT    (SPI_RegDef_t *SPIx, const void *txbuf, uint32 words);
    static int  Receive_IT     (SPI_RegDef_t *SPIx, void *rxbuf, uint32 words);
    static int  Transceive_IT  (SPI_RegDef_t *SPIx, const void *txbuf, void *rxbuf, uint32 words);

    static void ConfigDMATX    (SPI_RegDef_t *SPIx, DMA_Stream_t *stream, const void *mem_addr, uint32 len);
    static void ConfigDMARX    (SPI_RegDef_t *SPIx, DMA_Stream_t *stream, void *mem_addr, uint32 len);

    static void RegisterTxCallback   (SPI_RegDef_t *SPIx, SPI_Callback_t cb);
    static void RegisterRxCallback   (SPI_RegDef_t *SPIx, SPI_Callback_t cb);
    static void RegisterErrorCallback(SPI_RegDef_t *SPIx, SPI_Callback_t cb);

    /* Weak default callbacks */
    static void TxCompleteCallback(SPI_RegDef_t *SPIx);
    static void RxCompleteCallback(SPI_RegDef_t *SPIx);
    static void ErrorCallback     (SPI_RegDef_t *SPIx);

private:
    static uint8 GetIndex(SPI_RegDef_t *SPIx);
    static void  IRQ_Handler(SPI_RegDef_t *SPIx);
    SPI() = delete;

    friend void SPI1_IRQHandler(void);
    friend void SPI2_IRQHandler(void);
    friend void SPI3_IRQHandler(void);
};
