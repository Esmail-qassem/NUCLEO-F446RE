#include "STD_TYPES.h"
#include "OLED.h"
#include "Gif.h"
#include "OLED_NyanCat.h"

extern uint8 OLED_Buffer[OLED_WIDTH * OLED_PAGES];


void Gif_Application(void)
{
    static uint8 frame = 0;
    static uint8 counter_oled = 0;
    if(counter_oled == 0u)
    {     
        for (uint16 i = 0; i < 256; i++) OLED_Buffer[i] = 0x00;
        for (uint16 i = 768; i < 1024; i++) OLED_Buffer[i] = 0x00;
        counter_oled++;
    }
    if (frame >= NYANCAT_FRAME_COUNT) frame = 0;
    
    /* clear top 2 pages and bottom 2 pages (Nyan Cat is 32px tall, centered) */
    for (uint16 i = 0; i < 512; i++) OLED_Buffer[256 + i] = NyanCat_Frames[frame][i];
    frame++;

    OLED_UpdateScreen(I2C1_PORT,2,6);



}