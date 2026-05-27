/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : RTC                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Register base ────────────────────────────────────────────────── */
constexpr uint32 RTC_BASE_ADDR = 0x40002800UL;

#define RTC_TR      (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x00U))
#define RTC_DR      (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x04U))
#define RTC_CR      (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x08U))
#define RTC_ISR     (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x0CU))
#define RTC_PRER    (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x10U))
#define RTC_WUTR    (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x14U))
#define RTC_ALRMAR  (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x1CU))
#define RTC_ALRMBR  (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x20U))
#define RTC_WPR     (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x24U))
#define RTC_SSR     (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x28U))
#define RTC_TSTR    (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x30U))
#define RTC_TSDR    (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x34U))
#define RTC_TAFCR   (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x40U))
#define RTC_BKP(n)  (*reinterpret_cast<volatile uint32*>(RTC_BASE_ADDR + 0x50U + static_cast<uint32>(n)*4U))

/* ── RCC backup domain ────────────────────────────────────────────── */
constexpr uint32 RCC_BASE_RTC_ADDR = 0x40023800UL;
#define RCC_BDCR    (*reinterpret_cast<volatile uint32*>(RCC_BASE_RTC_ADDR + 0x70U))
#define RCC_CSR_RTC (*reinterpret_cast<volatile uint32*>(RCC_BASE_RTC_ADDR + 0x74U))

constexpr uint32 RCC_BDCR_LSEON    = (1U <<  0U);
constexpr uint32 RCC_BDCR_LSERDY   = (1U <<  1U);
constexpr uint8  RCC_BDCR_RTCSEL_Pos = 8U;
constexpr uint32 RCC_BDCR_RTCEN    = (1U << 15U);
constexpr uint32 RCC_BDCR_BDRST    = (1U << 16U);
constexpr uint32 RCC_CSR_LSION     = (1U <<  0U);
constexpr uint32 RCC_CSR_LSIRDY    = (1U <<  1U);

/* ── PWR backup domain write enable ──────────────────────────────── */
constexpr uint32 PWR_BASE_RTC_ADDR = 0x40007000UL;
#define PWR_CR_RTC  (*reinterpret_cast<volatile uint32*>(PWR_BASE_RTC_ADDR + 0x00U))
constexpr uint32 PWR_CR_DBP = (1U << 8U);

/* ── ISR bit masks ────────────────────────────────────────────────── */
constexpr uint32 RTC_ISR_ALRAWF = (1U <<  0U);
constexpr uint32 RTC_ISR_ALRBWF = (1U <<  1U);
constexpr uint32 RTC_ISR_WUTWF  = (1U <<  2U);
constexpr uint32 RTC_ISR_SHPF   = (1U <<  3U);
constexpr uint32 RTC_ISR_INITS  = (1U <<  4U);
constexpr uint32 RTC_ISR_RSF    = (1U <<  5U);
constexpr uint32 RTC_ISR_INITF  = (1U <<  6U);
constexpr uint32 RTC_ISR_INIT   = (1U <<  7U);
constexpr uint32 RTC_ISR_ALRAF  = (1U <<  8U);
constexpr uint32 RTC_ISR_ALRBF  = (1U <<  9U);
constexpr uint32 RTC_ISR_WUTF   = (1U << 10U);
constexpr uint32 RTC_ISR_TSF    = (1U << 11U);
constexpr uint32 RTC_ISR_TSOVF  = (1U << 12U);
constexpr uint32 RTC_ISR_TAMP1F = (1U << 13U);

/* ── CR bit masks ─────────────────────────────────────────────────── */
constexpr uint32 RTC_CR_TSEDGE  = (1U <<  3U);
constexpr uint32 RTC_CR_REFCKON = (1U <<  4U);
constexpr uint32 RTC_CR_BYPSHAD = (1U <<  5U);
constexpr uint32 RTC_CR_FMT     = (1U <<  6U);
constexpr uint32 RTC_CR_ALRAE   = (1U <<  8U);
constexpr uint32 RTC_CR_ALRBE   = (1U <<  9U);
constexpr uint32 RTC_CR_WUTE    = (1U << 10U);
constexpr uint32 RTC_CR_TSE     = (1U << 11U);
constexpr uint32 RTC_CR_ALRAIE  = (1U << 12U);
constexpr uint32 RTC_CR_ALRBIE  = (1U << 13U);
constexpr uint32 RTC_CR_WUTIE   = (1U << 14U);
constexpr uint32 RTC_CR_TSIE    = (1U << 15U);

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class RTC_ClockSrc : uint8
{
    NONE = 0,
    LSE  = 1,
    LSI  = 2,
    HSE  = 3
};

enum class RTC_HourFormat : uint8
{
    H24 = 0,
    H12 = 1
};

enum class RTC_AMPM : uint8
{
    AM = 0,
    PM = 1
};

enum class RTC_Weekday : uint8
{
    MONDAY    = 1,
    TUESDAY   = 2,
    WEDNESDAY = 3,
    THURSDAY  = 4,
    FRIDAY    = 5,
    SATURDAY  = 6,
    SUNDAY    = 7
};

enum class RTC_WakeupClk : uint8
{
    RTCDIV16 = 0,
    RTCDIV8  = 1,
    RTCDIV4  = 2,
    RTCDIV2  = 3,
    CK_SPRE  = 4,
    CK_SPRE2 = 6
};

enum class RTC_AlarmID : uint8
{
    A = 0,
    B = 1
};

enum class RTC_Status : uint8
{
    OK      = 0,
    ERROR,
    TIMEOUT
};

/* ── Data structures ──────────────────────────────────────────────── */
struct RTC_Config_t
{
    RTC_ClockSrc  clock_src;
    uint32        prediv_a;
    uint32        prediv_s;
    RTC_HourFormat hour_format;
};

struct RTC_Time_t
{
    uint8    hours;
    uint8    minutes;
    uint8    seconds;
    RTC_AMPM ampm;
};

struct RTC_Date_t
{
    uint8       date;
    uint8       month;
    uint8       year;
    RTC_Weekday weekday;
};

struct RTC_Alarm_t
{
    RTC_Time_t time;
    uint8      date_or_day;
    uint8      use_weekday;
    uint8      mask_hours;
    uint8      mask_minutes;
    uint8      mask_seconds;
    uint8      mask_date;
};

/* ── RTC Driver Class ─────────────────────────────────────────────── */
class RTC_Driver
{
public:
    static RTC_Status Init             (const RTC_Config_t &config);
    static RTC_Status SetTime          (const RTC_Time_t &time);
    static RTC_Status GetTime          (RTC_Time_t &time);
    static RTC_Status SetDate          (const RTC_Date_t &date);
    static RTC_Status GetDate          (RTC_Date_t &date);
    static RTC_Status SetAlarm         (RTC_AlarmID id, const RTC_Alarm_t &alarm, uint8 interrupt_enable);
    static void       DisableAlarm     (RTC_AlarmID id);
    static void       ClearAlarmFlag   (RTC_AlarmID id);
    static uint8      GetAlarmFlag     (RTC_AlarmID id);
    static RTC_Status SetWakeup        (RTC_WakeupClk clk_sel, uint16 period, uint8 interrupt_enable);
    static void       DisableWakeup    (void);
    static void       ClearWakeupFlag  (void);
    static uint8      GetWakeupFlag    (void);
    static void       EnableTimestamp  (uint8 falling_edge, uint8 interrupt_enable);
    static void       DisableTimestamp (void);
    static void       GetTimestamp     (RTC_Time_t &time, RTC_Date_t &date);
    static void       ClearTimestampFlag(void);
    static void       WriteBackup      (uint8 index, uint32 value);
    static uint32     ReadBackup       (uint8 index);
    static uint8      IsInitialized    (void);
    static void       WaitSync         (void);

private:
    RTC_Driver() = delete;
};

/* ── C-compatible aliases (kept for backward compatibility) ───────── */
using RTC_Config_t_t  = RTC_Config_t;
using RTC_Time_t_t    = RTC_Time_t;
using RTC_Date_t_t    = RTC_Date_t;
using RTC_Alarm_t_t   = RTC_Alarm_t;

/* Provide same function names as old C API */
inline RTC_Status RTC_Init(const RTC_Config_t *cfg)
{
    if (!cfg) return RTC_Status::ERROR;
    return RTC_Driver::Init(*cfg);
}
inline RTC_Status RTC_SetTime(const RTC_Time_t *t) { if (!t) return RTC_Status::ERROR; return RTC_Driver::SetTime(*t); }
inline RTC_Status RTC_GetTime(RTC_Time_t *t)       { if (!t) return RTC_Status::ERROR; return RTC_Driver::GetTime(*t); }
inline RTC_Status RTC_SetDate(const RTC_Date_t *d) { if (!d) return RTC_Status::ERROR; return RTC_Driver::SetDate(*d); }
inline RTC_Status RTC_GetDate(RTC_Date_t *d)       { if (!d) return RTC_Status::ERROR; return RTC_Driver::GetDate(*d); }
