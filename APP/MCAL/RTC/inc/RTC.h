#ifndef RTC_H_
#define RTC_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  RTC — Real-Time Clock  (STM32F446RE)
 *
 *  The RTC lives in the backup domain (VBAT island) and keeps
 *  running even when VDD is removed, as long as VBAT is supplied.
 *
 *  Block diagram:
 *
 *   [LSE 32.768kHz]──┐
 *   [LSI  ~32kHz]────┼──► RTC_CLK ──► ÷(PREDIV_A+1) ──► ÷(PREDIV_S+1)
 *   [HSE÷31]─────────┘                (async,  7-bit)    (sync,  15-bit)
 *                                           │                   │
 *                                           └──────── 1 Hz ─────┘
 *                                                      │
 *                                               Calendar / Alarm
 *                                               Wakeup timer
 *                                               Timestamp
 *
 *  Typical prescalers for 1 Hz calendar tick:
 *    LSE (32 768 Hz): PREDIV_A=127, PREDIV_S=255  → 32768/128/256 = 1 Hz
 *    LSI (~32 000 Hz): PREDIV_A=99,  PREDIV_S=319  → 32000/100/320 = 1 Hz
 *
 *  Write-protection: WPR register must be unlocked (0xCA → 0x53) before
 *  any configuration write, and locked again after.
 *
 *  Shadow registers: TR/DR are read-safe after RSF is set (set by HW
 *  each second). Use BYPSHAD=1 in CR to read directly from counters.
 *
 *  All time and date fields are stored in BCD internally.
 *  This driver converts transparently — use plain decimal in your code.
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 *  Register base and map
 *------------------------------------------------------------------*/
#define RTC_BASE    0x40002800U

#define RTC_TR      (*((volatile uint32*)(RTC_BASE + 0x00U)))  /* Time            */
#define RTC_DR      (*((volatile uint32*)(RTC_BASE + 0x04U)))  /* Date            */
#define RTC_CR      (*((volatile uint32*)(RTC_BASE + 0x08U)))  /* Control         */
#define RTC_ISR     (*((volatile uint32*)(RTC_BASE + 0x0CU)))  /* Init/Status     */
#define RTC_PRER    (*((volatile uint32*)(RTC_BASE + 0x10U)))  /* Prescaler       */
#define RTC_WUTR    (*((volatile uint32*)(RTC_BASE + 0x14U)))  /* Wakeup timer    */
#define RTC_ALRMAR  (*((volatile uint32*)(RTC_BASE + 0x1CU)))  /* Alarm A         */
#define RTC_ALRMBR  (*((volatile uint32*)(RTC_BASE + 0x20U)))  /* Alarm B         */
#define RTC_WPR     (*((volatile uint32*)(RTC_BASE + 0x24U)))  /* Write protect   */
#define RTC_SSR     (*((volatile uint32*)(RTC_BASE + 0x28U)))  /* Sub-second      */
#define RTC_TSTR    (*((volatile uint32*)(RTC_BASE + 0x30U)))  /* Timestamp time  */
#define RTC_TSDR    (*((volatile uint32*)(RTC_BASE + 0x34U)))  /* Timestamp date  */
#define RTC_TAFCR   (*((volatile uint32*)(RTC_BASE + 0x40U)))  /* Tamper/AF ctrl  */

/* Backup registers (RTC_BKPxR, x = 0…19) */
#define RTC_BKP(n)  (*((volatile uint32*)(RTC_BASE + 0x50U + (uint32)(n)*4U)))

/*------------------------------------------------------------------
 *  RCC backup domain register (needed for RTC clock source)
 *------------------------------------------------------------------*/
#define RCC_BASE_RTC  0x40023800U
#define RCC_CSR_RTC (*((volatile uint32*)(RCC_BASE_RTC + 0x74U)))

/* RCC_BDCR bits */
#define RCC_BDCR_LSEON    (1U <<  0)
#define RCC_BDCR_LSERDY   (1U <<  1)
#define RCC_BDCR_RTCSEL_Pos  8U
#define RCC_BDCR_RTCEN    (1U << 15)
#define RCC_BDCR_BDRST    (1U << 16)

/* RCC_CSR bits (LSI) */
#define RCC_CSR_LSION     (1U <<  0)
#define RCC_CSR_LSIRDY    (1U <<  1)

/*------------------------------------------------------------------
 *  PWR register (for DBP — backup domain write access)
 *------------------------------------------------------------------*/
#define PWR_BASE_RTC  0x40007000U
#define PWR_CR_RTC   (*((volatile uint32*)(PWR_BASE_RTC + 0x00U)))
#define PWR_CR_DBP    (1U << 8)

/*------------------------------------------------------------------
 *  ISR bit masks
 *------------------------------------------------------------------*/
#define RTC_ISR_ALRAWF  (1U <<  0)  /* Alarm A write flag          */
#define RTC_ISR_ALRBWF  (1U <<  1)  /* Alarm B write flag          */
#define RTC_ISR_WUTWF   (1U <<  2)  /* Wakeup timer write flag     */
#define RTC_ISR_SHPF    (1U <<  3)  /* Shift operation pending     */
#define RTC_ISR_INITS   (1U <<  4)  /* RTC init status (1=calendar set) */
#define RTC_ISR_RSF     (1U <<  5)  /* Registers sync flag         */
#define RTC_ISR_INITF   (1U <<  6)  /* Init mode flag              */
#define RTC_ISR_INIT    (1U <<  7)  /* Init mode enable            */
#define RTC_ISR_ALRAF   (1U <<  8)  /* Alarm A flag                */
#define RTC_ISR_ALRBF   (1U <<  9)  /* Alarm B flag                */
#define RTC_ISR_WUTF    (1U << 10)  /* Wakeup timer flag           */
#define RTC_ISR_TSF     (1U << 11)  /* Timestamp flag              */
#define RTC_ISR_TSOVF   (1U << 12)  /* Timestamp overflow          */
#define RTC_ISR_TAMP1F  (1U << 13)  /* Tamper 1 flag               */

/*------------------------------------------------------------------
 *  CR bit masks
 *------------------------------------------------------------------*/
#define RTC_CR_TSEDGE   (1U <<  3)  /* Timestamp edge select       */
#define RTC_CR_REFCKON  (1U <<  4)  /* Ref clock detection enable  */
#define RTC_CR_BYPSHAD  (1U <<  5)  /* Bypass shadow registers     */
#define RTC_CR_FMT      (1U <<  6)  /* Hour format (0=24h, 1=12h)  */
#define RTC_CR_ALRAE    (1U <<  8)  /* Alarm A enable              */
#define RTC_CR_ALRBE    (1U <<  9)  /* Alarm B enable              */
#define RTC_CR_WUTE     (1U << 10)  /* Wakeup timer enable         */
#define RTC_CR_TSE      (1U << 11)  /* Timestamp enable            */
#define RTC_CR_ALRAIE   (1U << 12)  /* Alarm A interrupt enable    */
#define RTC_CR_ALRBIE   (1U << 13)  /* Alarm B interrupt enable    */
#define RTC_CR_WUTIE    (1U << 14)  /* Wakeup timer interrupt      */
#define RTC_CR_TSIE     (1U << 15)  /* Timestamp interrupt         */
#define RTC_CR_WUCKSEL_Pos  0U      /* Wakeup clock select [2:0]   */

/*------------------------------------------------------------------
 *  Enumerations
 *------------------------------------------------------------------*/

/** RTC clock source (written to RCC_BDCR RTCSEL[1:0]) */
typedef enum {
    RTC_CLK_NONE = 0,   /* No clock           */
    RTC_CLK_LSE  = 1,   /* LSE 32.768 kHz     */
    RTC_CLK_LSI  = 2,   /* LSI ~32 kHz        */
    RTC_CLK_HSE  = 3    /* HSE / 31           */
} RTC_ClockSrc_t;

/** Hour format */
typedef enum {
    RTC_HOURFORMAT_24 = 0,
    RTC_HOURFORMAT_12 = 1
} RTC_HourFormat_t;

/** AM / PM (only relevant in 12-hour mode) */
typedef enum {
    RTC_AM = 0,
    RTC_PM = 1
} RTC_AMPM_t;

/** Day of the week (RTC hardware uses 1=Monday…7=Sunday, 0 is forbidden) */
typedef enum {
    RTC_WEEKDAY_MONDAY    = 1,
    RTC_WEEKDAY_TUESDAY   = 2,
    RTC_WEEKDAY_WEDNESDAY = 3,
    RTC_WEEKDAY_THURSDAY  = 4,
    RTC_WEEKDAY_FRIDAY    = 5,
    RTC_WEEKDAY_SATURDAY  = 6,
    RTC_WEEKDAY_SUNDAY    = 7
} RTC_Weekday_t;

/** Wakeup clock selection (CR WUCKSEL[2:0]) */
typedef enum {
    RTC_WUCKSEL_RTCDIV16  = 0,
    RTC_WUCKSEL_RTCDIV8   = 1,
    RTC_WUCKSEL_RTCDIV4   = 2,
    RTC_WUCKSEL_RTCDIV2   = 3,
    RTC_WUCKSEL_CK_SPRE   = 4,   /* 1 Hz (ck_spre)           */
    RTC_WUCKSEL_CK_SPRE2  = 6    /* 1 Hz + 2^16 added to WUT */
} RTC_WakeupClk_t;

/** Which alarm */
typedef enum {
    RTC_ALARM_A = 0,
    RTC_ALARM_B = 1
} RTC_AlarmID_t;

/*------------------------------------------------------------------
 *  Data structures
 *------------------------------------------------------------------*/

/** Configuration passed to RTC_Init */
typedef struct {
    RTC_ClockSrc_t  clock_src;     /* LSE / LSI / HSE             */
    uint32          prediv_a;      /* Async prescaler (0–127)      */
    uint32          prediv_s;      /* Sync  prescaler (0–32767)    */
    RTC_HourFormat_t hour_format;  /* 24h or 12h                   */
} RTC_Config_t;

/** Time fields (use plain decimal — BCD handled internally) */
typedef struct {
    uint8       hours;     /* 0–23 (24h) or 1–12 (12h)  */
    uint8       minutes;   /* 0–59                       */
    uint8       seconds;   /* 0–59                       */
    RTC_AMPM_t  ampm;      /* AM or PM (12h mode only)   */
} RTC_Time_t;

/** Date fields */
typedef struct {
    uint8          date;       /* Day of month: 1–31    */
    uint8          month;      /* 1–12                  */
    uint8          year;       /* 0–99 (offset from 2000) */
    RTC_Weekday_t  weekday;    /* Monday=1 … Sunday=7   */
} RTC_Date_t;

/** Alarm configuration */
typedef struct {
    RTC_Time_t  time;
    uint8       date_or_day;  /* Day of month (1–31) or weekday (1–7) */
    uint8       use_weekday;  /* 0 = match date, 1 = match weekday    */
    uint8       mask_hours;   /* 1 = ignore hours in comparison       */
    uint8       mask_minutes; /* 1 = ignore minutes                   */
    uint8       mask_seconds; /* 1 = ignore seconds                   */
    uint8       mask_date;    /* 1 = ignore date/weekday              */
} RTC_Alarm_t;

/** Status returned by API functions */
typedef enum {
    RTC_OK = 0,
    RTC_ERROR,
    RTC_TIMEOUT
} RTC_Status_t;

/*------------------------------------------------------------------
 *  Initialization
 *------------------------------------------------------------------*/

/**
 * RTC_Init — Configure clock source, prescalers, and hour format.
 *
 * Prerequisite: PWR clock must be enabled (RCC_APB1 bit 28) so that
 *               the backup domain can be written (DBP bit).
 *
 * If the calendar is already set (INITS flag) this function skips
 * the time/date reset — call RTC_SetTime()/RTC_SetDate() explicitly
 * to update them.
 *
 * Typical config for 1 Hz with LSE:
 *   { RTC_CLK_LSE, 127, 255, RTC_HOURFORMAT_24 }
 * Typical config for 1 Hz with LSI:
 *   { RTC_CLK_LSI, 99, 319, RTC_HOURFORMAT_24 }
 */
RTC_Status_t RTC_Init(const RTC_Config_t *config);

/*------------------------------------------------------------------
 *  Time and date
 *------------------------------------------------------------------*/
RTC_Status_t RTC_SetTime(const RTC_Time_t *time);
RTC_Status_t RTC_GetTime(RTC_Time_t *time);

RTC_Status_t RTC_SetDate(const RTC_Date_t *date);
RTC_Status_t RTC_GetDate(RTC_Date_t *date);

/*------------------------------------------------------------------
 *  Alarms
 *------------------------------------------------------------------*/

/**
 * RTC_SetAlarm — Configure and enable Alarm A or B.
 * The corresponding EXTI line (17=AlarmA, 18=AlarmB) and NVIC must
 * be enabled by the caller to receive an interrupt.
 */
RTC_Status_t RTC_SetAlarm(RTC_AlarmID_t id, const RTC_Alarm_t *alarm,
                           uint8 interrupt_enable);

/** RTC_DisableAlarm — Disable Alarm A or B. */
void         RTC_DisableAlarm(RTC_AlarmID_t id);

/** RTC_ClearAlarmFlag — Clear the alarm flag in ISR. */
void         RTC_ClearAlarmFlag(RTC_AlarmID_t id);

/** RTC_GetAlarmFlag — Return 1 if the alarm flag is set. */
uint8        RTC_GetAlarmFlag(RTC_AlarmID_t id);

/*------------------------------------------------------------------
 *  Periodic wakeup timer
 *------------------------------------------------------------------*/

/**
 * RTC_SetWakeup — Configure the automatic wakeup timer.
 *
 * @param clk_sel  Clock source for the wakeup counter
 * @param period   Counter reload value (0 – 65535)
 *                 With RTC_WUCKSEL_CK_SPRE the period is in seconds.
 * @param interrupt_enable  1 = generate RTC_WKUP interrupt
 *
 * EXTI line 22 must be enabled as event/rising edge for the wakeup
 * interrupt to reach the NVIC.
 */
RTC_Status_t RTC_SetWakeup(RTC_WakeupClk_t clk_sel, uint16 period,
                             uint8 interrupt_enable);

/** RTC_DisableWakeup — Stop the wakeup timer. */
void  RTC_DisableWakeup(void);

/** RTC_ClearWakeupFlag — Clear WUTF in ISR. */
void  RTC_ClearWakeupFlag(void);

/** RTC_GetWakeupFlag — Return 1 if wakeup timer fired. */
uint8 RTC_GetWakeupFlag(void);

/*------------------------------------------------------------------
 *  Timestamp
 *------------------------------------------------------------------*/

/**
 * RTC_EnableTimestamp — Capture time/date on a rising/falling edge
 * on the RTC_AF1 pin (PC13 on NUCLEO-F446RE when configured as AF).
 * @param falling_edge  0 = rising edge, 1 = falling edge
 */
void RTC_EnableTimestamp(uint8 falling_edge, uint8 interrupt_enable);
void RTC_DisableTimestamp(void);

/** Read the captured timestamp into time/date structs. */
void RTC_GetTimestamp(RTC_Time_t *time, RTC_Date_t *date);
void RTC_ClearTimestampFlag(void);

/*------------------------------------------------------------------
 *  Backup registers (20 × 32-bit, survive reset and Standby)
 *------------------------------------------------------------------*/
void   RTC_WriteBackup(uint8 index, uint32 value);
uint32 RTC_ReadBackup(uint8 index);

/*------------------------------------------------------------------
 *  Status helpers
 *------------------------------------------------------------------*/
uint8 RTC_IsInitialized(void);   /* Returns 1 if calendar has been set  */
void  RTC_WaitSync(void);        /* Wait for shadow register sync (RSF)  */

#endif /* RTC_H_ */
