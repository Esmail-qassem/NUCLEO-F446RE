#include "MPU.h"

void MPU_Enable(uint8 privDefEna)
{
    /* TODO Level 1:
     * 1. Write MPU_CTRL with ENABLE bit set
     * 2. Set PRIVDEFENA bit based on privDefEna argument
     * 3. Use DSB + ISB barriers after enabling */

     if(privDefEna)
     {
        MPU_CTRL |= (1U << 2);
     }
     else
     {
        MPU_CTRL &= ~(1U << 2);
     }
     MPU_CTRL |= (1U << 0);  /* enable last — after all config bits are set */
     __asm volatile ("DSB");  /* wait until MPU_CTRL write reaches hardware before moving on */
     __asm volatile ("ISB");  /* flush pipeline — re-fetch next instructions under new MPU rules */

     

}

void MPU_Disable(void)
{
    /* TODO Level 1:
     * 1. Clear MPU_CTRL ENABLE bit
     * 2. Use DSB + ISB barriers */
    MPU_CTRL &= ~(1<<0);
    __asm volatile ("DSB");  /* ensure MPU is fully off before any subsequent access */
    __asm volatile ("ISB");  /* flush pipeline — discard any pre-fetched instructions under old rules */

}

void MPU_ConfigRegion(const MPU_RegionConfig_t *cfg)
{
    /* TODO Level 2:
     * 1. Write region number to MPU_RNR
     * 2. Write base address to MPU_RBAR
     * 3. Build RASR value: SIZE, AP, XN, mem type bits, SRD, ENABLE
     * 4. Write to MPU_RASR */
}

void MPU_EnableRegion(MPU_Region_t region)
{
    /* TODO:
     * 1. Select region via MPU_RNR
     * 2. Set ENABLE bit (bit 0) in MPU_RASR without changing other bits */
}

void MPU_DisableRegion(MPU_Region_t region)
{
    /* TODO:
     * 1. Select region via MPU_RNR
     * 2. Clear ENABLE bit (bit 0) in MPU_RASR without changing other bits */
}

uint32 MPU_GetFaultAddress(void)
{
    /* TODO: return MPU_MMFAR — only valid when CFSR MMARVALID bit is set */
    return 0;
}

uint32 MPU_GetFaultStatus(void)
{
    /* TODO: return lower byte of MPU_CFSR (MemManage status bits) */
    return 0;
}
