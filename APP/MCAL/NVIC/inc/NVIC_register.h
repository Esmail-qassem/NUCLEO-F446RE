/***********************************************************************/
/*Aauthor     : Esamil Qassem                                          */
/*Data        : 3 FEB 2024                                             */
/*SWC         : NVIC                                                   */
/*Version     : V.1                                                    */
/***********************************************************************/

#ifndef NVIC_REGISTER_H_
#define NVIC_REGISTER_H_



#define NVIC_BASE_ADD 0xE000E100

#define NVIC_ISER0       *((volatile uint32*)NVIC_BASE_ADD)
#define NVIC_ISER1       *((volatile uint32*)(NVIC_BASE_ADD+0x04))


#define NVIC_ICER0       *((volatile uint32*)(NVIC_BASE_ADD+0x80))
#define NVIC_ICER1       *((volatile uint32*)(NVIC_BASE_ADD+0x84))

#define NVIC_ISPR0       *((volatile uint32*)(NVIC_BASE_ADD+0x100))
#define NVIC_ISPR1       *((volatile uint32*)(NVIC_BASE_ADD+0x104))


#define NVIC_ICPR0      *((volatile uint32*)(NVIC_BASE_ADD+0x180))
#define NVIC_ICPR1      *((volatile uint32*)(NVIC_BASE_ADD+0x184))

#define NVIC_IABR0       *((volatile uint32*)(NVIC_BASE_ADD+0x200))
#define NVIC_IABR1       *((volatile uint32*)(NVIC_BASE_ADD+0x204))
/********************************************************************/
#define NVIC_IPRn         ((volatile uint8*)(NVIC_BASE_ADD+0x300))

/**********************************************************************/
/*SCB REGISTER*/
/*Application interrupt and reset control register (SCB_AIRCR)*/

#define NVIC_SCB_AIRCR_Reg   ((volatile NVIC_SCB_AIRCR*)(0xE000ED0C))

#endif /* INC_NVIC_REGISTER_H_ */
