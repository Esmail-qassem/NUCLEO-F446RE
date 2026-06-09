#ifndef OLED_H_
#define OLED_H_

#include "STD_TYPES.h"
#include "I2C.h"



#define OLED_I2C_ADDR    0x3C   // 7-bit address
#define OLED_WIDTH       128
#define OLED_HEIGHT      64
#define OLED_PAGES       (OLED_HEIGHT / 8)

#define MOVE_UP    0x1
#define MOVE_DOWN  0x2
#define MOVE_RIGHT 0x3
#define MOVE_LEFT  0x4
#define GAME_RESET 0X5

typedef enum {
    OLED_COLOR_BLACK = 0x00,
    OLED_COLOR_WHITE = 0x01
} OLED_Color_t;

void OLED_Init(I2C_Port_t PORT);
void OLED_SendCommand(I2C_Port_t PORT,uint8 cmd);
void OLED_SendData(I2C_Port_t PORT,uint8 data);
void OLED_Clear(void);
void OLED_DrawPixel(uint8 x, uint8 y, OLED_Color_t color);
void OLED_UpdateScreen(I2C_Port_t PORT,uint8 startPage,uint8 EndPage);
void OLED_DrawChar(uint8 x, uint8 y, char c);
void OLED_DrawString(uint8 x, uint8 y, const char *str);
void OLED_APP(void);

#endif