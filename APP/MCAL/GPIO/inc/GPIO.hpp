/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : GPIO                                                   */
/* Version    : V3.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Hardware Register Layout ─────────────────────────────────────── */
struct GPIO_RegDef_t
{
    volatile uint32 MODER;    /* 0x00 – Mode                 */
    volatile uint32 OTYPER;   /* 0x04 – Output type          */
    volatile uint32 OSPEEDR;  /* 0x08 – Output speed         */
    volatile uint32 PUPDR;    /* 0x0C – Pull-up/pull-down    */
    volatile uint32 IDR;      /* 0x10 – Input data           */
    volatile uint32 ODR;      /* 0x14 – Output data          */
    volatile uint32 BSRR;     /* 0x18 – Bit set/reset        */
    volatile uint32 LCKR;     /* 0x1C – Configuration lock   */
    volatile uint32 AFRL;     /* 0x20 – AF low  (pins 0-7)  */
    volatile uint32 AFRH;     /* 0x24 – AF high (pins 8-15) */
};

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 GPIOA_BASE = 0x40020000UL;
constexpr uint32 GPIOB_BASE = 0x40020400UL;
constexpr uint32 GPIOC_BASE = 0x40020800UL;
constexpr uint32 GPIOD_BASE = 0x40020C00UL;
constexpr uint32 GPIOE_BASE = 0x40021000UL;
constexpr uint32 GPIOF_BASE = 0x40021400UL;
constexpr uint32 GPIOG_BASE = 0x40021800UL;
constexpr uint32 GPIOH_BASE = 0x40021C00UL;

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class GPIO_Mode : uint8
{
    INPUT  = 0b00,
    OUTPUT = 0b01,
    AF     = 0b10,
    ANALOG = 0b11
};

enum class GPIO_OType : uint8
{
    PP = 0,
    OD = 1
};

enum class GPIO_Speed : uint8
{
    LOW    = 0b00,
    MEDIUM = 0b01,
    FAST   = 0b10,
    HIGH   = 0b11
};

enum class GPIO_Pull : uint8
{
    NONE = 0b00,
    UP   = 0b01,
    DOWN = 0b10
};

enum class GPIO_Port : uint8
{
    A, B, C, D, E, F, G, H
};

enum class GPIO_Pin : uint8
{
    P0, P1, P2, P3, P4, P5, P6,  P7,
    P8, P9, P10, P11, P12, P13, P14, P15
};

enum class GPIO_Status : uint8
{
    OK,
    WrongPort,
    WrongPin
};

constexpr uint8 GPIO_HIGH = 1u;
constexpr uint8 GPIO_LOW  = 0u;

/* ── GPIO Driver Class ────────────────────────────────────────────── */
class GPIO
{
public:
    static GPIO_Status InitPin(GPIO_Port port, GPIO_Pin   pin,
                               GPIO_Mode mode,  GPIO_OType otype,
                               GPIO_Speed speed, GPIO_Pull  pull);

    static GPIO_Status WritePin (GPIO_Port port, GPIO_Pin pin, uint8 value);
    static GPIO_Status TogglePin(GPIO_Port port, GPIO_Pin pin);
    static GPIO_Status ReadPin  (GPIO_Port port, GPIO_Pin pin, uint8 &value);
    static void        SetAF    (GPIO_Port port, uint8    pin, uint8 AF);

private:
    static GPIO_RegDef_t* GetPort(GPIO_Port port);
    GPIO() = delete;
};
