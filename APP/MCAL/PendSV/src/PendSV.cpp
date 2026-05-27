#include "PendSV.hpp"

void PendSV::Init(void)
{
    *reinterpret_cast<volatile uint32*>(SCB_SHPR3_ADDR) |= (0xFFu << 16u);
}

void PendSV::Trigger(void)
{
    *reinterpret_cast<volatile uint32*>(SCB_ICSR_ADDR) |= (1u << 28u);
}
