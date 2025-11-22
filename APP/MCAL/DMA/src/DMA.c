#include "DMA.h"


/* If CMSIS is available, include NVIC prototypes */
// #include "stm32f4xx.h"  /* Uncomment if using CMSIS device header */

typedef struct {
    dma_callback_t cb;
    void *context;
} dma_cb_entry_t;

/* Store callback entries for 2 controllers x 8 streams */
static dma_cb_entry_t g_dma_cbs[2][8];

/* ---------- Helpers ---------- */

DMA_Stream_RegDef_t *DMA_GetStreamReg(dma_controller_t controller, dma_stream_t stream)
{
    DMA_RegDef_t *dma = (controller == DMA_CONTROLLER_1) ? DMA1 : DMA2;
    return &dma->STREAM[(uint32)stream];
}

/* Interrupt flag helpers:
   Flags layout: each stream has 4 flags (TCIF, HTIF, TEIF, DMEIF, FEIF) but their positions are in LISR/HISR.
   We'll compute the LIFCR/HIFCR mask for clearing and check LISR/HISR for reading.
   For brevity we implement only Transfer Complete and Transfer Error handling for needed stream groups.
*/

/* Stream -> which status register (LISR/HISR) and which bit positions:
   Streams 0..3 -> LISR / LIFCR fields; Streams 4..7 -> HISR / HIFCR fields.
   Each stream uses 6 bits of status starting at a stream-specific offset.
   We'll compute offsets as in RM: for stream n (0..7), the status bits start at 6*(n%4) in LISR or HISR.
*/

static inline uint32 dma_status_offset_for_stream(uint32 stream)
{
    uint32 idx = stream % 4;
    return idx * 6U;
}

/* Read status bits for a stream */
static uint32 DMA_ReadStatus(dma_controller_t controller, dma_stream_t stream)
{
    DMA_RegDef_t *dma = (controller == DMA_CONTROLLER_1) ? DMA1 : DMA2;
    uint32 s = (uint32)stream;
    uint32 status;
    if (s <= 3) {
        status = dma->LISR;
    } else {
        status = dma->HISR;
    }
    uint32 off = dma_status_offset_for_stream(s);
    return (status >> off) & 0x3FU; /* 6 bits */
}

/* Clear status flags for a stream */
static void DMA_ClearFlag(dma_controller_t controller, dma_stream_t stream, uint32 mask6bit)
{
    DMA_RegDef_t *dma = (controller == DMA_CONTROLLER_1) ? DMA1 : DMA2;
    uint32 s = (uint32)stream;
    uint32 off = dma_status_offset_for_stream(s);
    uint32 clear_mask = (mask6bit & 0x3FU) << off;
    if (s <= 3) {
        dma->LIFCR = clear_mask;
    } else {
        dma->HIFCR = clear_mask;
    }
}

/* Map stream status 6-bit field: bits are [FEIF, DMEIF, TEIF, HTIF, TCIF, ???] depending on RM.
   We only need to detect TCIF (transfer complete) and TEIF (transfer error): positions vary but
   typical ordering (from RM0316) is: (bit5)FEIF,(4)DMEIF,(3)TEIF,(2)HTIF,(1)TCIF,(0)???.
   To be robust, user should consult reference manual for exact mapping. We'll check typical TCIF mask (1<<1) and TEIF (1<<3).
*/

/* ---------- Public API ---------- */

void DMA_DriverInit(void)
{
    /* Zero callbacks */
    for (int c = 0; c < 2; ++c)
        for (int s = 0; s < 8; ++s) {
            g_dma_cbs[c][s].cb = NULL;
            g_dma_cbs[c][s].context = NULL;
        }
}

/* Configure a stream: stream must be disabled before calling */
int DMA_ConfigStream(const dma_config_t *cfg)
{
    if (!cfg) return -1;
    DMA_Stream_RegDef_t *sreg = DMA_GetStreamReg(cfg->controller, cfg->stream);

    /* If stream enabled, abort */
    if (sreg->CR & DMA_SxCR_EN) {
        return -2; /* stream enabled - must abort/disable first */
    }

    /* Clear CR and FCR (clean slate) */
    sreg->CR = 0;
    sreg->FCR = 0x00000021; /* default reset value: DMA_SxFCR empty? (implementation dependent) */
    /* Set FIFO if user wants */
    if (cfg->use_fifo) {
        sreg->FCR |= DMA_SxFCR_DMDIS; /* disable direct mode (use FIFO) */
        sreg->FCR |= DMA_SxFCR_FTH_1HALF; /* choose threshold - change as needed */
    } else {
        /* enable direct mode: clear DMDIS (0) */
        sreg->FCR &= ~DMA_SxFCR_DMDIS;
    }

    /* Set addresses and NDTR */
    sreg->PAR = cfg->periph_addr;
    sreg->M0AR = cfg->mem0_addr;
    sreg->NDTR = cfg->data_length;

    /* Build CR value */
    uint32 cr = 0;
    /* channel selection: bits 27:25 in CR (CHSEL) - set by shifting */
    cr |= ((cfg->channel & 7U) << 25);

    /* direction */
    switch (cfg->direction) {
        case DMA_DIR_PERIPH_TO_MEM: cr |= DMA_SxCR_DIR_PER2MEM; break;
        case DMA_DIR_MEM_TO_PERIPH: cr |= DMA_SxCR_DIR_MEM2PER; break;
        case DMA_DIR_MEM_TO_MEM:    cr |= DMA_SxCR_DIR_MEM2MEM; break;
    }

    if (cfg->mem_inc) cr |= DMA_SxCR_MINC;
    if (cfg->periph_inc) cr |= DMA_SxCR_PINC;

    cr |= (cfg->mem_size & (3U << DMA_SxCR_MSIZE_Pos)); /* expects one of MSIZE macros */
    cr |= (cfg->periph_size & (3U << DMA_SxCR_PSIZE_Pos));

    cr |= (cfg->priority & (3U << DMA_SxCR_PL_Pos)); /* PL bits */

    if (cfg->circular_mode) cr |= (1U << 8); /* CIRC bit */

    /* enable transfer complete and error interrupts by default */
    cr |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;

    sreg->CR = cr;

    return 0;
}

int DMA_Start(const dma_config_t *cfg)
{
    if (!cfg) return -1;
    DMA_Stream_RegDef_t *sreg = DMA_GetStreamReg(cfg->controller, cfg->stream);

    /* Ensure stream is disabled */
    if (sreg->CR & DMA_SxCR_EN) {
        return -2; /* already enabled */
    }

    /* reload NDTR and addresses in case changed */
    sreg->PAR = cfg->periph_addr;
    sreg->M0AR = cfg->mem0_addr;
    sreg->NDTR = cfg->data_length;

    /* Clear any pending flags for this stream */
    DMA_ClearFlag(cfg->controller, cfg->stream, 0x3FU);

    /* Enable the stream (word access) */
    sreg->CR |= DMA_SxCR_EN;

    /* Enable NVIC IRQ for that stream (user may prefer to enable globally elsewhere).
       IRQ mapping depends on controller+stream. For brevity we show a pattern - user must enable actual IRQ in startup or call NVIC_EnableIRQ with correct IRQn.
    */
    /* Example (uncomment if CMSIS present):
       if (cfg->controller == DMA_CONTROLLER_1) {
           switch (cfg->stream) {
               case DMA_STREAM_0: NVIC_EnableIRQ(DMA1_Stream0_IRQn); break;
               case DMA_STREAM_1: NVIC_EnableIRQ(DMA1_Stream1_IRQn); break;
               ...
           }
       } else {
           // DMA2_StreamX_IRQn
       }
    */

    return 0;
}

void DMA_Abort(dma_controller_t controller, dma_stream_t stream)
{
    DMA_Stream_RegDef_t *sreg = DMA_GetStreamReg(controller, stream);
    /* Clear EN and wait until it reads 0 */
    sreg->CR &= ~DMA_SxCR_EN;
    /* Wait for EN to clear (simple spin) */
    while (sreg->CR & DMA_SxCR_EN) { __asm volatile("nop"); }
    /* Clear flags */
    DMA_ClearFlag(controller, stream, 0x3FU);
}

/* Register callback for stream (transfer complete) */
void DMA_RegisterCallback(dma_controller_t controller, dma_stream_t stream, dma_callback_t cb, void *context)
{
    if (!cb) return;
    g_dma_cbs[(controller == DMA_CONTROLLER_1) ? 0 : 1][(uint32)stream].cb = cb;
    g_dma_cbs[(controller == DMA_CONTROLLER_1) ? 0 : 1][(uint32)stream].context = context;
}

/* This function should be called from the actual IRQ handler for the stream.
   Alternatively, implement global IRQ handlers that call this helper with the right params.
*/
void DMA_HandleIRQ(dma_controller_t controller, dma_stream_t stream)
{
    uint32 status = DMA_ReadStatus(controller, stream);

    /* Typical mapping - check RM for your part. We'll interpret:
       TCIF bit ~ (1<<1)
       TEIF bit ~ (1<<3)
    */
    const uint32 TCIF = (1U << 1);
    const uint32 TEIF = (1U << 3);

    if (status & TCIF) {
        /* Clear TCIF and other flags */
        DMA_ClearFlag(controller, stream, (1U<<1) | (1U<<2) | (1U<<3) | (1U<<4) | (1U<<5));
        /* call callback if registered */
        dma_cb_entry_t *entry = &g_dma_cbs[(controller == DMA_CONTROLLER_1) ? 0 : 1][(uint32)stream];
        if (entry->cb) entry->cb(entry->context);
    }

    if (status & TEIF) {
        /* handle transfer error (clear and callback maybe) */
        DMA_ClearFlag(controller, stream, (1U<<1) | (1U<<2) | (1U<<3) | (1U<<4) | (1U<<5));
        /* Optionally call callback or error handler */
        dma_cb_entry_t *entry = &g_dma_cbs[(controller == DMA_CONTROLLER_1) ? 0 : 1][(uint32)stream];
        if (entry->cb) entry->cb(entry->context); /* user callback should check whether transfer complete or error via status if needed */
    }
}
