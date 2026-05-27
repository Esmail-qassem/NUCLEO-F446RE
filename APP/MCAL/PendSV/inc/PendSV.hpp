#pragma once
#include "STD_TYPES.h"

constexpr uint32 SCB_SHPR3_ADDR = 0xE000ED20UL;
constexpr uint32 SCB_ICSR_ADDR  = 0xE000ED04UL;

class PendSV
{
public:
    static void Init   (void);
    static void Trigger(void);
    PendSV() = delete;
};
