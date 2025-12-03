#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include "STD_TYPES.h"

/* --------- Base addresses (STM32F4 typical mapping) --------- */
#define SPI1_BASE         (0x40013000U)
#define SPI2_BASE         (0x40003800)
#define SPI3_BASE         (0x40003C00)
#define DMA2_BASE         (0x40026400)



/* --------- SPI register map (STM32F4 family) --------- */
typedef struct {
    uint32 CR1;      /* 0x00 */
    uint32 CR2;      /* 0x04 */
    uint32 SR;       /* 0x08 */
    uint32 DR;       /* 0x0C */
    uint32 CRCPR;    /* 0x10 */
    uint32 RXCRCR;   /* 0x14 */
    uint32 TXCRCR;   /* 0x18 */
    uint32 I2SCFGR;  /* 0x1C */
    uint32 I2SPR;    /* 0x20 */
} SPI_TypeDef;

/* --------- DMA stream register map (simplified) --------- */
typedef struct {
    uint32 CR;     /* 0x00 */
    uint32 NDTR;   /* 0x04 */
    uint32 PAR;    /* 0x08 */
    uint32 M0AR;   /* 0x0C */
    uint32 M1AR;   /* 0x10 */
    uint32 FCR;    /* 0x14 */
} DMA_Stream_TypeDef;

/* peripheral pointers */
#define SPI1 ((SPI_TypeDef *) SPI1_BASE)
#define SPI2 ((SPI_TypeDef *) SPI2_BASE)
#define SPI3 ((SPI_TypeDef *) SPI3_BASE)

/* DMA2 streams used (addresses per reference manual) */
#define DMA2_Stream3 ((DMA_Stream_TypeDef *)(DMA2_BASE + 0x018 + 0x10*3)) /* stream3 */
#define DMA2_Stream2 ((DMA_Stream_TypeDef *)(DMA2_BASE + 0x018 + 0x10*2)) /* stream2 */


/* --------- SPI register bit defines --------- */
/* CR1 */
#define SPI_CR1_CPHA         (1U << 0)
#define SPI_CR1_CPOL         (1U << 1)
#define SPI_CR1_MSTR         (1U << 2)
#define SPI_CR1_BR_Pos       3U
#define SPI_CR1_SPE          (1U << 6)
#define SPI_CR1_LSBFIRST     (1U << 7)
#define SPI_CR1_SSI          (1U << 8)
#define SPI_CR1_SSM          (1U << 9)
#define SPI_CR1_RXONLY       (1U << 10)
#define SPI_CR1_DFF          (1U << 11) /* 0=8bit,1=16bit */

/* CR2 */
#define SPI_CR2_SSOE         (1U << 2)
#define SPI_CR2_TXEIE        (1U << 7)
#define SPI_CR2_RXNEIE       (1U << 6)
#define SPI_CR2_TXDMAEN      (1U << 1)
#define SPI_CR2_RXDMAEN      (1U << 0)

/* SR */
#define SPI_SR_RXNE          (1U << 0)
#define SPI_SR_TXE           (1U << 1)
#define SPI_SR_OVR           (1U << 6)
#define SPI_SR_BSY           (1U << 7)

/* DMA stream CR bits used */
#define DMA_SxCR_EN          (1U << 0)
#define DMA_SxCR_DIR_Pos     6U
#define DMA_SxCR_MINC        (1U << 10)
#define DMA_SxCR_TCIE        (1U << 4)
#define DMA_SxCR_PL_Pos      16U
#define DMA_SxCR_MSIZE_Pos   13U
#define DMA_SxCR_PSIZE_Pos   11U

/* DMA direction values (in CR DIR bits) */
#define DMA_DIR_PERIPH_TO_MEM  (0U << DMA_SxCR_DIR_Pos)
#define DMA_DIR_MEM_TO_PERIPH  (1U << DMA_SxCR_DIR_Pos)
#define DMA_PL_HIGH            (2U << DMA_SxCR_PL_Pos)

/* --------- Public API --------- */

typedef enum {
    SPI_MODE_0 = 0, /* CPOL=0 CPHA=0 */
    SPI_MODE_1,
    SPI_MODE_2,
    SPI_MODE_3
} SPI_Mode_t;

typedef enum {
    SPI_DATASIZE_8 = 0,
    SPI_DATASIZE_16
} SPI_Datauint32;

typedef struct {
    SPI_TypeDef *Instance;     /* SPI1/SPI2/SPI3 */
    uint32 prescaler;        /* 2,4,8,16,32,64,128,256 (driver maps to BR bits) */
    SPI_Mode_t mode;
    uint8 master;            /* 1=master, 0=slave */
    SPI_Datauint32 dataSize;   /* 8 or 16 bits */
    uint8 lsbFirst;          /* 0=MSB first, 1=LSB first */
    uint8 softwareNSS;       /* 1 -> SSM/SSI set (master) */
} SPI_Config_t;

/* Initialization */
void SPI_Init(const SPI_Config_t *cfg);

/* Blocking transfers (words parameter = number of frames; frame=8 or 16 bits depending on config) */
int SPI_TransmitBlocking(SPI_TypeDef *SPIx, const void *txbuf, uint32 words);
int SPI_ReceiveBlocking(SPI_TypeDef *SPIx, void *rxbuf, uint32 words);
int SPI_TransceiveBlocking(SPI_TypeDef *SPIx, const void *txbuf, void *rxbuf, uint32 words);

/* DMA helpers (simple) */
/* Initializes DMA clocks and configures streams for the given SPI (uses DMA2_Stream3 for TX and DMA2_Stream2 for RX by default).
   If you need different streams change implementation. */
void SPI_DMA_EnableClocks(void);
void SPI_ConfigDMATX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream, const void *mem_addr, uint32 len);
void SPI_ConfigDMARX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream, void *mem_addr, uint32 len);

/* Weak callbacks you can override in your app */
//  void SPI_DMA_TxCompleteCallback(SPI_TypeDef *SPIx) __attribute__((weak));
//  void SPI_DMA_RxCompleteCallback(SPI_TypeDef *SPIx);__attribute__((weak));
//  void SPI_ErrorCallback(SPI_TypeDef *SPIx);__attribute__((weak));

#endif /* SPI_DRIVER_H */
