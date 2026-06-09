#ifndef MPU_H_
#define MPU_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  MPU — Memory Protection Unit  (ARM Cortex-M4 / STM32F446RE)
 *
 *  The Cortex-M4 MPU has 8 programmable regions (0–7).
 *  Each region defines: base address, size, access permissions,
 *  memory type, and execute-never.
 *
 *  Every memory access made by the CPU is checked against active
 *  regions. A violation triggers a MemManage fault.
 *
 *  Overlap rule: higher region number wins over lower.
 *
 *  Register map (System Control Space):
 *    MPU_TYPE  0xE000ED90  — read-only, reports number of regions
 *    MPU_CTRL  0xE000ED94  — enable/disable MPU, background region
 *    MPU_RNR   0xE000ED98  — select which region (0–7) to configure
 *    MPU_RBAR  0xE000ED9C  — base address of selected region
 *    MPU_RASR  0xE000EDA0  — size, permissions, type, enable
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 *  Register Definitions
 *------------------------------------------------------------------*/
#define MPU_TYPE    (*((volatile uint32*)0xE000ED90U))
#define MPU_CTRL    (*((volatile uint32*)0xE000ED94U))
#define MPU_RNR     (*((volatile uint32*)0xE000ED98U))
#define MPU_RBAR    (*((volatile uint32*)0xE000ED9CU))
#define MPU_RASR    (*((volatile uint32*)0xE000EDA0U))

/* SCB — MemManage Fault Address Register (address that caused the fault) */
#define MPU_MMFAR   (*((volatile uint32*)0xE000ED34U))

/* SCB — Configurable Fault Status Register (tells you what kind of fault) */
#define MPU_CFSR    (*((volatile uint32*)0xE000ED28U))

/*------------------------------------------------------------------
 *  MPU_CTRL bit positions
 *------------------------------------------------------------------*/
#define MPU_CTRL_ENABLE_Pos      0U   /* 1 = MPU enabled                          */
#define MPU_CTRL_HFNMIENA_Pos    1U   /* 1 = MPU active during HardFault/NMI/FAULTMASK */
#define MPU_CTRL_PRIVDEFENA_Pos  2U   /* 1 = privileged code can access unmapped regions */

/*------------------------------------------------------------------
 *  MPU_RASR bit positions
 *------------------------------------------------------------------*/
#define MPU_RASR_ENABLE_Pos   0U    /* Region enable                        */
#define MPU_RASR_SIZE_Pos     1U    /* Region size [5:1] — log2 encoded     */
#define MPU_RASR_SRD_Pos      8U    /* Sub-region disable [15:8]            */
#define MPU_RASR_B_Pos        16U   /* Bufferable                           */
#define MPU_RASR_C_Pos        17U   /* Cacheable                            */
#define MPU_RASR_S_Pos        18U   /* Shareable                            */
#define MPU_RASR_TEX_Pos      19U   /* Type extension [21:19]               */
#define MPU_RASR_AP_Pos       24U   /* Access permission [26:24]            */
#define MPU_RASR_XN_Pos       28U   /* Execute Never                        */

/*------------------------------------------------------------------
 *  CFSR — MemManage fault status bits (within CFSR [7:0])
 *------------------------------------------------------------------*/
#define MPU_CFSR_IACCVIOL   (1U << 0U)  /* Instruction access violation        */
#define MPU_CFSR_DACCVIOL   (1U << 1U)  /* Data access violation               */
#define MPU_CFSR_MSTKERR    (1U << 4U)  /* Stacking error on exception entry   */
#define MPU_CFSR_MUNSTKERR  (1U << 3U)  /* Unstacking error on exception exit  */
#define MPU_CFSR_MMARVALID  (1U << 7U)  /* MMFAR holds the faulting address    */

/*------------------------------------------------------------------
 *  Enumerations
 *------------------------------------------------------------------*/

typedef enum
{
    MPU_REGION_0 = 0,
    MPU_REGION_1,
    MPU_REGION_2,
    MPU_REGION_3,
    MPU_REGION_4,
    MPU_REGION_5,
    MPU_REGION_6,
    MPU_REGION_7
} MPU_Region_t;

/* SIZE field encoding: actual size = 2^(N+1) bytes */
typedef enum
{
    MPU_SIZE_32B   = 4U,
    MPU_SIZE_64B   = 5U,
    MPU_SIZE_128B  = 6U,
    MPU_SIZE_256B  = 7U,
    MPU_SIZE_512B  = 8U,
    MPU_SIZE_1KB   = 9U,
    MPU_SIZE_2KB   = 10U,
    MPU_SIZE_4KB   = 11U,
    MPU_SIZE_8KB   = 12U,
    MPU_SIZE_16KB  = 13U,
    MPU_SIZE_32KB  = 14U,
    MPU_SIZE_64KB  = 15U,
    MPU_SIZE_128KB = 16U,
    MPU_SIZE_256KB = 17U,
    MPU_SIZE_512KB = 18U,
    MPU_SIZE_1MB   = 19U
} MPU_Size_t;

/* AP[2:0] — access permission field */
typedef enum
{
    MPU_AP_NO_ACCESS    = 0U,  /* Privileged: none   | Unprivileged: none  */
    MPU_AP_PRIV_RW      = 1U,  /* Privileged: RW     | Unprivileged: none  */
    MPU_AP_PRIV_RW_UN_R = 2U,  /* Privileged: RW     | Unprivileged: R     */
    MPU_AP_FULL_ACCESS  = 3U,  /* Privileged: RW     | Unprivileged: RW    */
    MPU_AP_PRIV_R       = 5U,  /* Privileged: R      | Unprivileged: none  */
    MPU_AP_READ_ONLY    = 6U   /* Privileged: R      | Unprivileged: R     */
} MPU_AP_t;

/* Memory type — TEX, C, B, S combined presets (most common cases) */
typedef enum
{
    MPU_MEM_STRONGLY_ORDERED = 0U,  /* TEX=0 C=0 B=0 S=0 — peripherals / SCB */
    MPU_MEM_DEVICE_SHARED    = 1U,  /* TEX=0 C=0 B=1 S=1 — shared device mem  */
    MPU_MEM_NORMAL_WT        = 2U,  /* TEX=0 C=1 B=0 S=0 — normal write-through (flash) */
    MPU_MEM_NORMAL_WB        = 3U,  /* TEX=0 C=1 B=1 S=0 — normal write-back (SRAM)     */
    MPU_MEM_NORMAL_NOCACHE   = 4U   /* TEX=1 C=0 B=0 S=0 — normal non-cacheable          */
} MPU_MemType_t;

/*------------------------------------------------------------------
 *  Region configuration struct
 *------------------------------------------------------------------*/
typedef struct
{
    MPU_Region_t  region;
    uint32        base_addr;   /* must be aligned to size */
    MPU_Size_t    size;
    MPU_AP_t      ap;
    MPU_MemType_t mem_type;
    uint8         xn;          /* 1 = Execute Never, 0 = executable */
    uint8         srd;         /* Sub-region disable mask (8 bits)   */
} MPU_RegionConfig_t;

/*------------------------------------------------------------------
 *  Public API
 *------------------------------------------------------------------*/

/* Enable the MPU.
 * privDefEna = 1: privileged code can still access unmapped regions
 *              0: any unmapped access → MemManage fault              */
void MPU_Enable(uint8 privDefEna);

/* Disable the MPU entirely — all memory accessible without checks */
void MPU_Disable(void);

/* Configure one region from a config struct, then enable the region */
void MPU_ConfigRegion(const MPU_RegionConfig_t *cfg);

/* Enable a previously configured region */
void MPU_EnableRegion(MPU_Region_t region);

/* Disable a region without erasing its configuration */
void MPU_DisableRegion(MPU_Region_t region);

/* Read the faulting address from MMFAR (call inside MemManage_Handler) */
uint32 MPU_GetFaultAddress(void);

/* Read CFSR MemManage bits (call inside MemManage_Handler) */
uint32 MPU_GetFaultStatus(void);

#endif /* MPU_H_ */
