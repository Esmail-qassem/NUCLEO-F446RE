

#ifndef SYSTICK_CONFIG_H_
#define SYSTICK_CONFIG_H_

//AHB = 72 MHZ
#define SYSTEM_CLOCK_HZ 16000000  // 72 MHz

/*	AHB_BY_8
 *  AHB
 * */

#define TSK_CLOCK AHB_BY_8




#if TSK_CLOCK == AHB_BY_8
    #define SYSTICK_CLOCK (SYSTEM_CLOCK_HZ / 8)  // AHB/8
#elif TSK_CLOCK == AHB
    #warning "AHB"
    #define SYSTICK_CLOCK (SYSTEM_CLOCK_HZ)      // AHB
#else
    #error "Invalid TSK_CLOCK configuration"
#endif

#define TICKS_PER_MS (SYSTICK_CLOCK / 1000)  // Ticks per millisecond






#endif







