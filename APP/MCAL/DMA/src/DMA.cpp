/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : DMA                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "DMA.hpp"

struct DMA_CB_Entry_t { DMA_Callback_t cb; void *ctx; };
static DMA_CB_Entry_t s_cbs[2][8];

static DMA_RegDef_t* GetDMARegs(DMA_Controller ctrl)
{
    return (ctrl == DMA_Controller::DMA1) ? DMA1_REGS : DMA2_REGS;
}

static uint32 StreamOffset(DMA_Stream s)
{
    return (static_cast<uint32>(s) % 4u) * 6u;
}

DMA_Stream_RegDef_t* DMA::GetStreamReg(DMA_Controller ctrl, DMA_Stream stream)
{
    return &GetDMARegs(ctrl)->STREAM[static_cast<uint8>(stream)];
}

static uint32 DMA_ReadStatus(DMA_Controller ctrl, DMA_Stream stream)
{
    DMA_RegDef_t *dma = GetDMARegs(ctrl);
    uint32 s = static_cast<uint32>(stream);
    uint32 status = (s <= 3u) ? dma->LISR : dma->HISR;
    return (status >> StreamOffset(stream)) & 0x3Fu;
}

static void DMA_ClearFlag(DMA_Controller ctrl, DMA_Stream stream, uint32 mask)
{
    DMA_RegDef_t *dma = GetDMARegs(ctrl);
    uint32 s   = static_cast<uint32>(stream);
    uint32 clr = (mask & 0x3Fu) << StreamOffset(stream);
    if (s <= 3u) dma->LIFCR = clr;
    else         dma->HIFCR = clr;
}

void DMA::DriverInit(void)
{
    for (int c = 0; c < 2; c++)
        for (int st = 0; st < 8; st++)
            s_cbs[c][st] = { nullptr, nullptr };
}

int DMA::ConfigStream(const DMA_Config_t &cfg)
{
    DMA_Stream_RegDef_t *sreg = GetStreamReg(cfg.controller, cfg.stream);
    if (sreg->CR & DMA_SxCR_EN) return -2;

    sreg->CR = 0u;
    sreg->FCR = cfg.use_fifo ? (DMA_SxFCR_DMDIS | (1u << 0u)) : 0u;

    sreg->PAR  = cfg.periph_addr;
    sreg->M0AR = cfg.mem0_addr;
    sreg->NDTR = cfg.data_length;

    uint32 cr = 0u;
    cr |= ((cfg.channel & 7u) << 25u);
    switch (cfg.direction) {
        case DMA_Direction::PERIPH_TO_MEM: cr |= DMA_SxCR_PER2MEM; break;
        case DMA_Direction::MEM_TO_PERIPH: cr |= DMA_SxCR_MEM2PER; break;
        case DMA_Direction::MEM_TO_MEM:    cr |= DMA_SxCR_MEM2MEM; break;
    }
    if (cfg.mem_inc)   cr |= DMA_SxCR_MINC;
    if (cfg.periph_inc) cr |= DMA_SxCR_PINC;
    cr |= (cfg.mem_size   & (3u << DMA_SxCR_MSIZE_Pos));
    cr |= (cfg.periph_size & (3u << DMA_SxCR_PSIZE_Pos));
    cr |= (cfg.priority   & (3u << DMA_SxCR_PL_Pos));
    if (cfg.circular_mode) cr |= (1u << 8u);
    cr |= (DMA_SxCR_TCIE | DMA_SxCR_TEIE);
    sreg->CR = cr;
    return 0;
}

int DMA::Start(const DMA_Config_t &cfg)
{
    DMA_Stream_RegDef_t *sreg = GetStreamReg(cfg.controller, cfg.stream);
    if (sreg->CR & DMA_SxCR_EN) return -2;
    sreg->PAR  = cfg.periph_addr;
    sreg->M0AR = cfg.mem0_addr;
    sreg->NDTR = cfg.data_length;
    DMA_ClearFlag(cfg.controller, cfg.stream, 0x3Fu);
    sreg->CR |= DMA_SxCR_EN;
    return 0;
}

void DMA::Abort(DMA_Controller ctrl, DMA_Stream stream)
{
    DMA_Stream_RegDef_t *sreg = GetStreamReg(ctrl, stream);
    sreg->CR &= ~DMA_SxCR_EN;
    while (sreg->CR & DMA_SxCR_EN) { __asm volatile("nop"); }
    DMA_ClearFlag(ctrl, stream, 0x3Fu);
}

void DMA::RegisterCallback(DMA_Controller ctrl, DMA_Stream stream, DMA_Callback_t cb, void *ctx)
{
    uint8 ci = (ctrl == DMA_Controller::DMA1) ? 0u : 1u;
    s_cbs[ci][static_cast<uint8>(stream)] = { cb, ctx };
}

void DMA::HandleIRQ(DMA_Controller ctrl, DMA_Stream stream)
{
    uint32 status = DMA_ReadStatus(ctrl, stream);
    constexpr uint32 TCIF = (1u << 1u);
    constexpr uint32 TEIF = (1u << 3u);
    if (status & (TCIF | TEIF)) {
        DMA_ClearFlag(ctrl, stream, 0x3Fu);
        uint8 ci = (ctrl == DMA_Controller::DMA1) ? 0u : 1u;
        auto &e = s_cbs[ci][static_cast<uint8>(stream)];
        if (e.cb) e.cb(e.ctx);
    }
}
