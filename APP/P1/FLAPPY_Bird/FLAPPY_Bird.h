#ifndef FLAPPY_BIRD_H_
#define FLAPPY_BIRD_H_

#include "OLED.h"

/*------------------------------------------------------------------
 *  Drawing primitive
 *------------------------------------------------------------------*/
void BLOCK_DRAW(uint8 width, uint8 height, uint8 xcurser, uint8 ycurser);

/*------------------------------------------------------------------
 *  Bird control
 *------------------------------------------------------------------*/
void  Bird_Init(void);
void  Bird_Update(void);
void  Bird_Jump(void);
uint8 Bird_GetY(void);
void  Bird_Draw(void);

#endif /* FLAPPY_BIRD_H_ */
