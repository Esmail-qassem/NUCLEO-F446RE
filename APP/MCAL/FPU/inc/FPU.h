#ifndef FPU_H_
#define FPU_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  FPU — Floating-Point Unit  (ARM Cortex-M4F / STM32F446RE)
 *
 *  The Cortex-M4F has a single-precision (SP) IEEE-754 FPU.
 *  It is DISABLED by default after reset — you must call FPU_Enable()
 *  before any floating-point instruction executes, otherwise a
 *  UsageFault (NOCP) is generated.
 *
 *  Architecture overview:
 *
 *   CPACR[23:20] ── coprocessor access: CP10 (FPU) + CP11 (must match)
 *   FPCCR        ── context save policy (lazy stacking / auto-stacking)
 *   FPSCR        ── status, rounding mode, exception flags (read via VMRS)
 *
 *  Lazy stacking (default recommended):
 *    The hardware reserves space for FP registers on the exception stack
 *    but only saves them if an FP instruction actually runs inside the ISR.
 *    This gives the best trade-off between context-switch speed and
 *    correctness for mixed FP/integer ISR code.
 *
 *  Exception flags in FPSCR:
 *    IDC — Input Denormal (flush-to-zero touched it)
 *    IXC — Inexact result
 *    UFC — Underflow
 *    OFC — Overflow
 *    DZC — Division by zero
 *    IOC — Invalid operation (NaN, inf, etc.)
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 *  Coprocessor Access Control Register
 *------------------------------------------------------------------*/
#define FPU_CPACR   (*((volatile uint32*)0xE000ED88U))

#define FPU_CPACR_CP10_Pos  20U
#define FPU_CPACR_CP11_Pos  22U
/* Access level values for each 2-bit field */
#define FPU_ACCESS_DENIED       0U   /* Any FP instruction → fault    */
#define FPU_ACCESS_PRIVILEGED   1U   /* Only privileged thread access */
#define FPU_ACCESS_FULL         3U   /* Full access from any thread   */

/*------------------------------------------------------------------
 *  FP Context Control Register
 *------------------------------------------------------------------*/
#define FPU_FPCCR   (*((volatile uint32*)0xE000EF34U))

#define FPU_FPCCR_ASPEN   (1U << 31U)  /* Auto state preservation enable */
#define FPU_FPCCR_LSPEN   (1U << 30U)  /* Lazy state preservation enable  */

/*------------------------------------------------------------------
 *  FP Context Address Register (address of stacked FP frame)
 *------------------------------------------------------------------*/
#define FPU_FPCAR   (*((volatile uint32*)0xE000EF38U))

/*------------------------------------------------------------------
 *  FP Default Status Control Register (reset value of FPSCR for new threads)
 *------------------------------------------------------------------*/
#define FPU_FPDSCR  (*((volatile uint32*)0xE000EF3CU))

/*------------------------------------------------------------------
 *  FPSCR bit masks (accessed via VMRS/VMSR, not memory)
 *------------------------------------------------------------------*/
#define FPU_FPSCR_IOC   (1U <<  0U)   /* Invalid operation       */
#define FPU_FPSCR_DZC   (1U <<  1U)   /* Division by zero        */
#define FPU_FPSCR_OFC   (1U <<  2U)   /* Overflow                */
#define FPU_FPSCR_UFC   (1U <<  3U)   /* Underflow               */
#define FPU_FPSCR_IXC   (1U <<  4U)   /* Inexact                 */
#define FPU_FPSCR_IDC   (1U <<  7U)   /* Input denormal          */
#define FPU_FPSCR_FZ    (1U << 24U)   /* Flush-to-zero mode      */
#define FPU_FPSCR_DN    (1U << 25U)   /* Default NaN mode        */
#define FPU_FPSCR_RMode_Pos  22U      /* Rounding mode [23:22]   */

/*------------------------------------------------------------------
 *  Enumerations
 *------------------------------------------------------------------*/

/** Rounding mode (IEEE-754 standard modes) */
typedef enum {
    FPU_ROUND_NEAREST  = 0,   /* Round to nearest, ties to even (RN) — default */
    FPU_ROUND_PLUS_INF = 1,   /* Round toward +infinity (RP)                   */
    FPU_ROUND_MINUS_INF= 2,   /* Round toward −infinity (RM)                   */
    FPU_ROUND_ZERO     = 3    /* Round toward zero / truncate (RZ)             */
} FPU_RoundMode_t;

/** Exception flag bitmask — OR these together for FPU_ClearExceptions() */
typedef enum {
    FPU_EX_INVALID   = FPU_FPSCR_IOC,
    FPU_EX_DIVZERO   = FPU_FPSCR_DZC,
    FPU_EX_OVERFLOW  = FPU_FPSCR_OFC,
    FPU_EX_UNDERFLOW = FPU_FPSCR_UFC,
    FPU_EX_INEXACT   = FPU_FPSCR_IXC,
    FPU_EX_DENORMAL  = FPU_FPSCR_IDC,
    FPU_EX_ALL       = 0x9FU
} FPU_Exception_t;

/*------------------------------------------------------------------
 *  Public API
 *------------------------------------------------------------------*/

/**
 * FPU_Enable — Grant full access to the FPU for both privileged and
 * unprivileged code.  Call this once, early in SystemInit() or main(),
 * before any floating-point operation.
 *
 * Also enables lazy stacking (LSPEN=1, ASPEN=1) — the recommended
 * setting for RTOS and ISR-intensive code.
 */
void FPU_Enable(void);

/**
 * FPU_Disable — Deny all access to the FPU.  Any subsequent FP
 * instruction will trigger a UsageFault (NOCP).
 */
void FPU_Disable(void);

/**
 * FPU_SetAccess — Fine-grained access control.
 * @param level  FPU_ACCESS_DENIED / FPU_ACCESS_PRIVILEGED / FPU_ACCESS_FULL
 */
void FPU_SetAccess(uint8 level);

/**
 * FPU_EnableLazyStacking — Recommended for RTOS: the CPU reserves stack
 * space for FP context on exception entry but only writes the registers
 * if an FP instruction runs inside the ISR.  This avoids the ~17-cycle
 * FP push overhead on ISRs that don't use the FPU.
 */
void FPU_EnableLazyStacking(void);

/**
 * FPU_DisableLazyStacking — Always save the full FP context on exception
 * entry.  Deterministic but slower.  Rarely needed.
 */
void FPU_DisableLazyStacking(void);

/**
 * FPU_SetRoundMode — Set the IEEE-754 rounding mode.
 * Default after reset: FPU_ROUND_NEAREST.
 */
void FPU_SetRoundMode(FPU_RoundMode_t mode);

/** FPU_GetRoundMode — Read the current rounding mode. */
FPU_RoundMode_t FPU_GetRoundMode(void);

/**
 * FPU_SetFlushToZero — When enabled, denormalised results are flushed
 * to zero instead of computing the full denormal.  Faster on many
 * signal-processing workloads but not strictly IEEE-754 compliant.
 */
void FPU_SetFlushToZero(uint8 enable);

/**
 * FPU_SetDefaultNaN — When enabled, any NaN result (from any operation)
 * is replaced by the canonical quiet NaN (0x7FC00000).  Simplifies
 * NaN propagation checks in embedded code.
 */
void FPU_SetDefaultNaN(uint8 enable);

/**
 * FPU_GetExceptions — Return the currently set exception flags (bitwise OR
 * of FPU_Exception_t values).
 */
uint32 FPU_GetExceptions(void);

/**
 * FPU_ClearExceptions — Clear the specified exception flags.
 * @param mask  Bitwise OR of FPU_Exception_t values, or FPU_EX_ALL.
 */
void FPU_ClearExceptions(uint32 mask);

/**
 * FPU_GetFPSCR — Read the raw FPSCR register value.
 * (Uses the VMRS instruction — requires the FPU to be enabled first.)
 */
uint32 FPU_GetFPSCR(void);

/**
 * FPU_SetFPSCR — Write the raw FPSCR register value.
 */
void FPU_SetFPSCR(uint32 value);

#endif /* FPU_H_ */
