#include "DMA.hpp"

#ifndef DMA_H_
#define DMA_H_













#endif 









#ifndef DMA_STM32F4_H
#define DMA_STM32F4_H

#include "STD_TYPES.h"

/* ---------- Basic types ---------- */
typedef void (*dma_callback_t)(void *context);

/* ---------- DMA register map for a stream (32-bit accesses) ---------- */
typedef struct {
    volatile uint32 CR;     /* 0x00 stream x configuration register */
    volatile uint32 NDTR;   /* 0x04 number of data register */
    volatile uint32 PAR;    /* 0x08 peripheral address register */
    volatile uint32 M0AR;   /* 0x0C memory 0 address register */
    volatile uint32 M1AR;   /* 0x10 memory 1 address register (double buffer) */
    volatile uint32 FCR;    /* 0x14 FIFO control register */
} DMA_Stream_RegDef_t;

/* DMA controller register block (only what we need) */
typedef struct {
    volatile uint32 LISR;   /* 0x00 low interrupt status register */
    volatile uint32 HISR;   /* 0x04 high interrupt status register */
    volatile uint32 LIFCR;  /* 0x08 low interrupt flag clear register */
    volatile uint32 HIFCR;  /* 0x0C high interrupt flag clear register */
    /* Streams follow - for each DMA controller there are 8 streams */
    DMA_Stream_RegDef_t STREAM[8]; /* 0x10 ... */
} DMA_RegDef_t;

/* ---------- Base addresses (STM32F4 series typical) ---------- */
#define DMA1_BASE ((uint32)0x40026000U)
#define DMA2_BASE ((uint32)0x40026400U)

#define DMA1 ((DMA_RegDef_t *)DMA1_BASE)
#define DMA2 ((DMA_RegDef_t *)DMA2_BASE)

/* ---------- Bit masks for DMA_SxCR register (CR) ---------- */
/* Control register masks (selected fields used often) */
#define DMA_SxCR_EN            (1U << 0)
#define DMA_SxCR_TCIE          (1U << 4)   /* Transfer complete interrupt enable */
#define DMA_SxCR_HTIE          (1U << 3)   /* Half transfer interrupt enable */
#define DMA_SxCR_TEIE          (1U << 2)   /* Transfer error interrupt enable */

#define DMA_SxCR_DIR_Pos       6U
#define DMA_SxCR_DIR_Msk       (3U << DMA_SxCR_DIR_Pos) /* 00: per->mem, 01: mem->per, 10: mem->mem */
#define DMA_SxCR_DIR_MEM2PER   (1U << DMA_SxCR_DIR_Pos)
#define DMA_SxCR_DIR_PER2MEM   (0U << DMA_SxCR_DIR_Pos)
#define DMA_SxCR_DIR_MEM2MEM   (2U << DMA_SxCR_DIR_Pos)

#define DMA_SxCR_MINC          (1U << 10)  /* Memory increment mode */
#define DMA_SxCR_PINC          (1U << 9)   /* Peripheral increment mode */
#define DMA_SxCR_MSIZE_Pos     13U
#define DMA_SxCR_MSIZE_8BIT    (0U << DMA_SxCR_MSIZE_Pos)
#define DMA_SxCR_MSIZE_16BIT   (1U << DMA_SxCR_MSIZE_Pos)
#define DMA_SxCR_MSIZE_32BIT   (2U << DMA_SxCR_MSIZE_Pos)
#define DMA_SxCR_PSIZE_Pos     11U
#define DMA_SxCR_PSIZE_8BIT    (0U << DMA_SxCR_PSIZE_Pos)
#define DMA_SxCR_PSIZE_16BIT   (1U << DMA_SxCR_PSIZE_Pos)
#define DMA_SxCR_PSIZE_32BIT   (2U << DMA_SxCR_PSIZE_Pos)

#define DMA_SxCR_PL_Pos        16U
#define DMA_SxCR_PL_LOW        (0U << DMA_SxCR_PL_Pos)
#define DMA_SxCR_PL_MEDIUM     (1U << DMA_SxCR_PL_Pos)
#define DMA_SxCR_PL_HIGH       (2U << DMA_SxCR_PL_Pos)
#define DMA_SxCR_PL_VERYHIGH   (3U << DMA_SxCR_PL_Pos)

/* FIFO control register (FCR) */
#define DMA_SxFCR_DMDIS        (1U << 2)
#define DMA_SxFCR_FTH_Pos      0U
#define DMA_SxFCR_FTH_1QUART   (0U << DMA_SxFCR_FTH_Pos)
#define DMA_SxFCR_FTH_1HALF    (1U << DMA_SxFCR_FTH_Pos)
#define DMA_SxFCR_FTH_3QUART   (2U << DMA_SxFCR_FTH_Pos)
#define DMA_SxFCR_FTH_FULL     (3U << DMA_SxFCR_FTH_Pos)

/* ---------- Interrupt flags masks (for LISR/HISR / LIFCR/HIFCR) ---------- */
/* Flags are per-stream groups; to keep the header compact, user will call helper functions which compute masks */

/* ---------- Driver API ---------- */

/* Stream selection: 0..7 */
typedef enum {
    DMA_STREAM_0 = 0,
    DMA_STREAM_1,
    DMA_STREAM_2,
    DMA_STREAM_3,
    DMA_STREAM_4,
    DMA_STREAM_5,
    DMA_STREAM_6,
    DMA_STREAM_7
} dma_stream_t;

/* DMA controller selection */
typedef enum {
    DMA_CONTROLLER_1 = 1,
    DMA_CONTROLLER_2 = 2
} dma_controller_t;

/* Transfer direction */
typedef enum {
    DMA_DIR_PERIPH_TO_MEM = 0,
    DMA_DIR_MEM_TO_PERIPH = 1,
    DMA_DIR_MEM_TO_MEM     = 2
} dma_dir_t;

/* Transfer config struct */
typedef struct {
    dma_controller_t controller; /* DMA1 or DMA2 */
    dma_stream_t stream;         /* stream index 0..7 */
    uint32 channel;            /* channel number (0..7) - set according to peripheral mapping */
    dma_dir_t direction;         /* transfer direction */
    uint32 periph_addr;        /* peripheral address */
    uint32 mem0_addr;          /* memory address */
    uint32 data_length;        /* number of items */
    uint32 periph_size;        /* DMA_SxCR_PSIZE_x */
    uint32 mem_size;           /* DMA_SxCR_MSIZE_x */
    uint32 priority;           /* DMA_SxCR_PL_x */
    uint8 periph_inc;          /* 0/1 */
    uint8 mem_inc;             /* 0/1 */
    uint8 circular_mode;       /* 0/1 */
    uint8 use_fifo;            /* 0/1 */
} dma_config_t;

/* Initialize driver (optional) */
void DMA_DriverInit(void);

/* Configure a stream (stops stream if enabled) */
int DMA_ConfigStream(const dma_config_t *cfg);

/* Start transfer (enables stream). Returns 0 on success */
int DMA_Start(const dma_config_t *cfg);

/* Abort a transfer (disables stream) */
void DMA_Abort(dma_controller_t controller, dma_stream_t stream);

/* Register ISR callback for transfer complete (and context) */
void DMA_RegisterCallback(dma_controller_t controller, dma_stream_t stream, dma_callback_t cb, void *context);

/* Internal: call from IRQ handler or provide the IRQ handler mapping */
void DMA_HandleIRQ(dma_controller_t controller, dma_stream_t stream);

/* Example helper to compute stream register pointer */
DMA_Stream_RegDef_t *DMA_GetStreamReg(dma_controller_t controller, dma_stream_t stream);

#endif /* DMA_STM32F4_H */
