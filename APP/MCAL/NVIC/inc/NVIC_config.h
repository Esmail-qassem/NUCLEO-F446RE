/***********************************************************************/
/*Aauthor     : Esamil Qassem                                          */
/*Data        : 3 FEB 2024                                             */
/*SWC         : NVIC                                                   */
/*Version     : V.1                                                    */
/***********************************************************************/

#ifndef NVIC_CONFIG_H_
#define NVIC_CONFIG_H_

/*choose the number of the groups and sub groups*/
/*if you want 16 groups choose                 :  bxxxx  */
/*if you want 8 groups and 2 Sub groups choose :  bxxx_y  */
/*if you want 4 groups and 4 Sub groups choose :  bxx_yy  */
/*if you want 2 groups and 8 Sub groups choose :  bx_yyy  */
/*if you want 0 groups and 16Sub  groups choose:  b_yyyy  */
#define priority_PreBuidConfig bxx_yy

#endif /* INC_NVIC_CONFIG_H_ */
