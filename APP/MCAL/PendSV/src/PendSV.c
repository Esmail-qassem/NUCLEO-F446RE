#include "PendSV.h"

void (*Copy_pvCallBackFunc)(void) = NULL;


void PendSV_Init(void)
{
    // Set PendSV priority to lowest (bits 23:16)
    SCB_SHPR3 |= (0xFF << 16);
}

void Trigger_PendSV(void)
{
    SCB_ICSR |= (1 << 28);  // Set PendSV pending bit
}
