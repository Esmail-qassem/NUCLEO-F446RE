#include "STD_TYPES.h"
#include "OLED.h"
#include "Oledh.h"
#include "Snake.h"
#include "Gif.h"

uint8 volatile User_Option=0;

void OLED_APP(void)
{
    uint8 static counter=0;
    uint8 static flag=0;
    if(counter == 0 || User_Option == GAME_RESET)
    {
        OLED_Clear();
        counter= 0;
        OLED_DrawString(0, 0, "-----------------------");
        OLED_DrawString(0, 8,  "i.  Gif");
        OLED_DrawString(0, 16, "j.  Snake Game");
        OLED_DrawString(0, 56, "------------------------");
        OLED_UpdateScreen(I2C1_PORT,0,8);
        flag=1;
    }
    if('i' == User_Option)
    {
       OLED_Clear();
       if(flag == 1)
       {
        OLED_UpdateScreen(I2C1_PORT,0,8);
       }
       Gif_Application();
       flag=0;
       counter=1;
    }
    else if('j' == User_Option)
    {
       Snake_Game();
       counter=1;
       flag=1;
    }
    



}