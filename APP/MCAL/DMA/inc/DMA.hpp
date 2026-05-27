/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : DMA                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 DMA1_BASE_ADDR = 0x40026000UL;
constexpr uint32 DMA2_BASE_ADDR = 0x40026400UL;

/* ── Stream register layout ───────────────────────────────────────── */
struct DMA_Stream_RegDef_t
{
    volatile uint32 CR;   /* 0x00 */
    volatile uint32 NDTR; /* 0x04 */
    volatile uint32 PAR;  /* 0x08 */
    volatile uint32 M0AR; /* 0x0C */
    volatile uint32 M1AR; /* 0x10 */
    volatile uint32 FCR;  /* 0x14 */
};

/* ── DMA controller register layout ──────────────────────────────── */
struct DMA_RegDef_t
{
    volatile uint32 LISR;  /* 0x00 */
    volatile uint32 HISR;  /* 0x04 */
    volatile uint32 LIFCR; /* 0x08 */
    volatile uint32 HIFCR; /* 0x0C */
    DMA_Stream_RegDef_t STREAM[8]; /* 0x10 ... */
};

#define DMA1_REGS (reinterpret_cast<DMA_RegDef_t*>(DMA1_BASE_ADDR))
#define DMA2_REGS (reinterpret_cast<DMA_RegDef_t*>(DMA2_BASE_ADDR))

/* ── CR bit masks ─────────────────────────────────────────────────── */
constexpr uint32 DMA_SxCR_EN     = (1u << 0u);
constexpr uint32 DMA_SxCR_TCIE   = (1u << 4u);
constexpr uint32 DMA_SxCR_HTIE   = (1u << 3u);
constexpr uint32 DMA_SxCR_TEIE   = (1u << 2u);
constexpr uint8  DMA_SxCR_DIR_Pos   = 6u;
constexpr uint32 DMA_SxCR_MEM2PER  = (1u << 6u);
constexpr uint32 DMA_SxCR_PER2MEM  = (0u << 6u);
constexpr uint32 DMA_SxCR_MEM2MEM  = (2u << 6u);
constexpr uint32 DMA_SxCR_MINC     = (1u << 10u);
constexpr uint32 DMA_SxCR_PINC     = (1u <<  9u);
constexpr uint8  DMA_SxCR_MSIZE_Pos = 13u;
constexpr uint32 DMA_SxCR_MSIZE_8BIT  = (0u << 13u);
constexpr uint32 DMA_SxCR_MSIZE_16BIT = (1u << 13u);
constexpr uint32 DMA_SxCR_MSIZE_32BIT = (2u << 13u);
constexpr uint8  DMA_SxCR_PSIZE_Pos = 11u;
constexpr uint32 DMA_SxCR_PSIZE_8BIT  = (0u << 11u);
constexpr uint32 DMA_SxCR_PSIZE_16BIT = (1u << 11u);
constexpr uint32 DMA_SxCR_PSIZE_32BIT = (2u << 11u);
constexpr uint8  DMA_SxCR_PL_Pos = 16u;
constexpr uint32 DMA_SxCR_PL_LOW      = (0u << 16u);
constexpr uint32 DMA_SxCR_PL_MEDIUM   = (1u << 16u);
constexpr uint32 DMA_SxCR_PL_HIGH     = (2u << 16u);
constexpr uint32 DMA_SxCR_PL_VERYHIGH = (3u << 16u);
constexpr uint32 DMA_SxFCR_DMDIS      = (1u <<  2u);

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class DMA_Stream : uint8
{
    S0=0, S1, S2, S3, S4, S5, S6, S7
};

enum class DMA_Controller : uint8
{
    DMA1 = 1u,
    DMA2 = 2u
};

enum class DMA_Direction : uint8
{
    PERIPH_TO_MEM = 0u,
    MEM_TO_PERIPH = 1u,
    MEM_TO_MEM    = 2u
};

using DMA_Callback_t = void(*)(void*);

struct DMA_Config_t
{
    DMA_Controller controller;
    DMA_Stream     stream;
    uint32         channel;
    DMA_Direction  direction;
    uint32         periph_addr;
    uint32         mem0_addr;
    uint32         data_length;
    uint32         periph_size;
    uint32         mem_size;
    uint32         priority;
    uint8          periph_inc;
    uint8          mem_inc;
    uint8          circular_mode;
    uint8          use_fifo;
};

/* ── DMA Driver Class ─────────────────────────────────────────────── */
class DMA
{
public:
    static void                  DriverInit      (void);
    static int                   ConfigStream    (const DMA_Config_t &cfg);
    static int                   Start           (const DMA_Config_t &cfg);
    static void                  Abort           (DMA_Controller ctrl, DMA_Stream stream);
    static void                  RegisterCallback(DMA_Controller ctrl, DMA_Stream stream, DMA_Callback_t cb, void *ctx);
    static void                  HandleIRQ       (DMA_Controller ctrl, DMA_Stream stream);
    static DMA_Stream_RegDef_t*  GetStreamReg    (DMA_Controller ctrl, DMA_Stream stream);
private:
    DMA() = delete;
};
