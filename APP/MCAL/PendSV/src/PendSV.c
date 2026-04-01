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


Status_t PendSV_CallBack(void (*Copy_p2vCallBackFunc)(void))
{
    if (Copy_p2vCallBackFunc == NULL)
    {
        return E_Null_Pointer;
    }
    Copy_pvCallBackFunc = Copy_p2vCallBackFunc;
    return E_Ok;
}


void PendSV_Handler(void)
{
    Copy_pvCallBackFunc();
}
