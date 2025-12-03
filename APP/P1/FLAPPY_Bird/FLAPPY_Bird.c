#include "FLAPPY_Bird.h"

void BLOCK_DRAW(uint8 width,uint8 height,uint8 xcurser,uint8 ycurser)
{
    for(uint8 i=xcurser;i<width+xcurser;i++)
    {
        for(uint8 j=ycurser;j<height+ycurser;j++)
        {
            OLED_DrawPixel(i,j, OLED_COLOR_WHITE);
        }
    }
}