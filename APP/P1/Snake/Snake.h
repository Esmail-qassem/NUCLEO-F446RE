#ifndef SNAKE_H_
#define SNAKE_H_

#include "OLED.h"



#define SNAKE_LENGTH 8192

typedef struct 
{
    uint8 x;
    uint8 y;    
}Snake_Dimention_t;
typedef struct
{
    uint16 Head;
    uint16 Trail;
    uint16 Size;
    Snake_Dimention_t Snake_Array[SNAKE_LENGTH];
}snake_t;

void Snake_Init(void);
void Snake_Game(void);
























#endif