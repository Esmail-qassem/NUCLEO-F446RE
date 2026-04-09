#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  Base addresses
 *------------------------------------------------------------------*/
#define SPI1_BASE  (0x40013000U)
#define SPI2_BASE  (0x40003800U)
#define SPI3_BASE  (0x40003C00U)
#define DMA2_BASE  (0x40026400U)

/*------------------------------------------------------------------
 *  SPI register map
 *------------------------------------------------------------------*/
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

/*------------------------------------------------------------------
 *  DMA stream register map
 *------------------------------------------------------------------*/
typedef struct {
    uint32 CR;    /* 0x00 */
    uint32 NDTR;  /* 0x04 */
    uint32 PAR;   /* 0x08 */
    uint32 M0AR;  /* 0x0C */
    uint32 M1AR;  /* 0x10 */
    uint32 FCR;   /* 0x14 */
} DMA_Stream_TypeDef;

/*------------------------------------------------------------------
 *  Peripheral pointers
 *------------------------------------------------------------------*/
#define SPI1  ((SPI_TypeDef *) SPI1_BASE)
#define SPI2  ((SPI_TypeDef *) SPI2_BASE)
#define SPI3  ((SPI_TypeDef *) SPI3_BASE)

/* DMA2 streams (address = DMA2_BASE + 0x018 + stream_num * 0x018) */
#define DMA2_Stream2  ((DMA_Stream_TypeDef *)(DMA2_BASE + 0x040U))
#define DMA2_Stream3  ((DMA_Stream_TypeDef *)(DMA2_BASE + 0x058U))

/*------------------------------------------------------------------
 *  CR1 bit masks
 *------------------------------------------------------------------*/
#define SPI_CR1_CPHA      (1U <<  0)  /* Clock phase                         */
#define SPI_CR1_CPOL      (1U <<  1)  /* Clock polarity                      */
#define SPI_CR1_MSTR      (1U <<  2)  /* Master selection                    */
#define SPI_CR1_BR_Pos    3U          /* Baud rate control [5:3]             */
#define SPI_CR1_SPE       (1U <<  6)  /* SPI enable                          */
#define SPI_CR1_LSBFIRST  (1U <<  7)  /* Frame format (0=MSB first)          */
#define SPI_CR1_SSI       (1U <<  8)  /* Internal slave select               */
#define SPI_CR1_SSM       (1U <<  9)  /* Software slave management           */
#define SPI_CR1_RXONLY    (1U << 10)  /* Receive only mode                   */
#define SPI_CR1_DFF       (1U << 11)  /* Data frame format (0=8bit, 1=16bit) */
#define SPI_CR1_CRCNEXT   (1U << 12)  /* Transmit CRC next                   */
#define SPI_CR1_CRCEN     (1U << 13)  /* Hardware CRC calculation enable     */
#define SPI_CR1_BIDIOE    (1U << 14)  /* Output enable in bidirectional mode */
#define SPI_CR1_BIDIMODE  (1U << 15)  /* Bidirectional data mode enable      */

/*------------------------------------------------------------------
 *  CR2 bit masks
 *------------------------------------------------------------------*/
#define SPI_CR2_RXDMAEN  (1U << 0)   /* Rx DMA enable         */
#define SPI_CR2_TXDMAEN  (1U << 1)   /* Tx DMA enable         */
#define SPI_CR2_SSOE     (1U << 2)   /* SS output enable      */
#define SPI_CR2_ERRIE    (1U << 5)   /* Error interrupt       */
#define SPI_CR2_RXNEIE   (1U << 6)   /* RXNE interrupt        */
#define SPI_CR2_TXEIE    (1U << 7)   /* TXE interrupt         */

/*------------------------------------------------------------------
 *  SR bit masks
 *------------------------------------------------------------------*/
#define SPI_SR_RXNE  (1U << 0)   /* Receive buffer not empty */
#define SPI_SR_TXE   (1U << 1)   /* Transmit buffer empty    */
#define SPI_SR_OVR   (1U << 6)   /* Overrun flag             */
#define SPI_SR_BSY   (1U << 7)   /* Busy flag                */

/*------------------------------------------------------------------
 *  DMA stream CR bit masks (used by DMA helpers)
 *------------------------------------------------------------------*/
#define DMA_SxCR_EN       (1U <<  0)
#define DMA_SxCR_DIR_Pos  6U
#define DMA_SxCR_MINC     (1U << 10)
#define DMA_SxCR_TCIE     (1U <<  4)
#define DMA_SxCR_PL_Pos   16U

#define DMA_DIR_PERIPH_TO_MEM  (0U << DMA_SxCR_DIR_Pos)
#define DMA_DIR_MEM_TO_PERIPH  (1U << DMA_SxCR_DIR_Pos)
#define DMA_PL_HIGH            (2U << DMA_SxCR_PL_Pos)

/*------------------------------------------------------------------
 *  Enumerations
 *------------------------------------------------------------------*/

/* SPI clock mode (CPOL / CPHA) */
typedef enum {
    SPI_MODE_0 = 0,   /* CPOL=0, CPHA=0 — idle low,  sample on rising  */
    SPI_MODE_1,       /* CPOL=0, CPHA=1 — idle low,  sample on falling */
    SPI_MODE_2,       /* CPOL=1, CPHA=0 — idle high, sample on falling */
    SPI_MODE_3        /* CPOL=1, CPHA=1 — idle high, sample on rising  */
} SPI_Mode_t;

/* Data frame width */
typedef enum {
    SPI_DATASIZE_8  = 0,
    SPI_DATASIZE_16 = 1
} SPI_DataSize_t;

/* Transfer direction */
typedef enum {
    SPI_DIR_FULL_DUPLEX    = 0,  /* BIDIMODE=0, RXONLY=0              */
    SPI_DIR_HALF_DUPLEX_TX = 1,  /* BIDIMODE=1, BIDIOE=1              */
    SPI_DIR_HALF_DUPLEX_RX = 2,  /* BIDIMODE=1, BIDIOE=0              */
    SPI_DIR_RX_ONLY        = 3   /* BIDIMODE=0, RXONLY=1 (simplex RX) */
} SPI_Direction_t;

/*------------------------------------------------------------------
 *  Callback type
 *------------------------------------------------------------------*/
typedef void (*SPI_Callback_t)(SPI_TypeDef *SPIx);

/*------------------------------------------------------------------
 *  Configuration structure
 *------------------------------------------------------------------*/
typedef struct {
    SPI_TypeDef    *Instance;      /* SPI1 / SPI2 / SPI3                          */
    uint32          prescaler;     /* Clock divider: 2,4,8,16,32,64,128,256        */
    SPI_Mode_t      mode;          /* Clock mode 0-3                               */
    SPI_Direction_t direction;     /* Full/half duplex, or RX-only                 */
    uint8           master;        /* 1 = master, 0 = slave                        */
    SPI_DataSize_t  dataSize;      /* 8-bit or 16-bit frames                       */
    uint8           lsbFirst;      /* 0 = MSB first, 1 = LSB first                 */
    uint8           softwareNSS;   /* 1 = SSM/SSI (software NSS), 0 = hardware NSS */
    uint8           crcEnable;     /* 1 = enable hardware CRC                      */
    uint16          crcPolynomial; /* CRC polynomial (used only if crcEnable=1)    */
} SPI_Config_t;

/*------------------------------------------------------------------
 *  Initialization / De-initialization
 *------------------------------------------------------------------*/
void SPI_Init(const SPI_Config_t *cfg);
void SPI_DeInit(SPI_TypeDef *SPIx);

/*------------------------------------------------------------------
 *  Blocking (polling) transfers
 *  words = number of frames (each frame is 8 or 16 bits per dataSize)
 *  Return 0 on success, negative on error (-1 bad param, -2/-3 OVR)
 *------------------------------------------------------------------*/
int SPI_TransmitBlocking(SPI_TypeDef *SPIx, const void *txbuf, uint32 words);
int SPI_ReceiveBlocking(SPI_TypeDef *SPIx, void *rxbuf, uint32 words);
int SPI_TransceiveBlocking(SPI_TypeDef *SPIx, const void *txbuf,
                            void *rxbuf, uint32 words);

/*------------------------------------------------------------------
 *  Non-blocking (interrupt-driven) transfers
 *  Caller must enable the NVIC line for the chosen SPI instance.
 *  The registered callback fires when the transfer completes.
 *  Return 0 if transfer started, -1 if busy or bad params.
 *------------------------------------------------------------------*/
int SPI_Transmit_IT(SPI_TypeDef *SPIx, const void *txbuf, uint32 words);
int SPI_Receive_IT(SPI_TypeDef *SPIx, void *rxbuf, uint32 words);
int SPI_Transceive_IT(SPI_TypeDef *SPIx, const void *txbuf,
                       void *rxbuf, uint32 words);

/*------------------------------------------------------------------
 *  DMA helpers
 *  Note: caller is responsible for enabling DMA clocks via RCC
 *        before calling these functions.
 *------------------------------------------------------------------*/
void SPI_ConfigDMATX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream,
                     const void *mem_addr, uint32 len);
void SPI_ConfigDMARX(SPI_TypeDef *SPIx, DMA_Stream_TypeDef *stream,
                     void *mem_addr, uint32 len);

/*------------------------------------------------------------------
 *  Callback registration
 *  Register application callbacks instead of overriding weak functions.
 *------------------------------------------------------------------*/
void SPI_RegisterTxCallback(SPI_TypeDef *SPIx, SPI_Callback_t cb);
void SPI_RegisterRxCallback(SPI_TypeDef *SPIx, SPI_Callback_t cb);
void SPI_RegisterErrorCallback(SPI_TypeDef *SPIx, SPI_Callback_t cb);

/*------------------------------------------------------------------
 *  Weak default callbacks (override in application if needed)
 *------------------------------------------------------------------*/
void SPI_TxCompleteCallback(SPI_TypeDef *SPIx);
void SPI_RxCompleteCallback(SPI_TypeDef *SPIx);
void SPI_ErrorCallback(SPI_TypeDef *SPIx);

#endif /* SPI_DRIVER_H */
