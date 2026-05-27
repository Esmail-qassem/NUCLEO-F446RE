/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : Flappy Bird Game                                       */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "FLAPPY_Bird.hpp"

void BLOCK_DRAW(uint8 width, uint8 height, uint8 xcurser, uint8 ycurser)
{
    for (uint8 i = xcurser; i < static_cast<uint8>(width + xcurser); i++)
    {
        for (uint8 j = ycurser; j < static_cast<uint8>(height + ycurser); j++)
        {
            OLED::DrawPixel(i, j, OLED_Color::WHITE);
        }
    }
}
