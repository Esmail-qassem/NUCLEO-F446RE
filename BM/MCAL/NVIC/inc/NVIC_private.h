/***********************************************************************/
/*Aauthor     : Esamil Qassem                                          */
/*Data        : 3 FEB 2024                                             */
/*SWC         : NVIC                                                   */
/*Version     : V.1                                                    */
/***********************************************************************/

#ifndef NVIC_PRIVATE_H_
#define NVIC_PRIVATE_H_

typedef union
{
struct
{
	uint32 VECTRESET:1;
	uint32 VECTCLRACTIVE:1;
	uint32 SYSRESETREQ:1;
	uint32:5;
	uint32 PRIGROUP :3; /*priority group*/
	uint32:4;
	uint32 ENDIANESS :1;
	uint32 VECTKEY:16;
};

uint32 x;

}NVIC_SCB_AIRCR;

/*number of group priorities and number of sub priorities*/
enum Priority_Grouping
{
	bxxxx= 0b011, /*all groups no sub group 16     priority */
	bxxx_y=0b100, /*8 groups and 2 sub group      priority */
	bxx_yy=0b101, /*4 groups and 4 sub group      priority */
	bx_yyy=0b110, /*2 groups and 8 sub group      priority */
	b_yyyy=0b111  /*all sub groups (16), no group priority */
};
#endif /* INC_NVIC_PRIVATE_H_ */
