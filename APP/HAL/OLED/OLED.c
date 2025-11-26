#include "OLED.h"

uint8 BufferA[OLED_WIDTH * OLED_PAGES];
uint8 BufferB[OLED_WIDTH * OLED_PAGES];

uint8 *OLED_Buffer= BufferA;        // FOR DRAWING
uint8 *OLED_DisplayBuffer= BufferB; // FOR DISPLAYING


#define OLED_COLUMN_OFFSET 2   // OLED has 132 columns, 128 visible

//-----------------------------------------------------//
//                Low-level Senders                    //
//-----------------------------------------------------//

void swapBuffer(void)
{
    uint8 *temp=OLED_Buffer;
    OLED_Buffer=OLED_DisplayBuffer;
    OLED_DisplayBuffer=temp;
}



void OLED_SendCommand(I2C_Port_t PORT, uint8 cmd)
{
    uint8 data[2];
    data[0] = 0x00;   // Control byte: Co=0, D/C#=0 (Command)
    data[1] = cmd;
    I2C_MasterTransmit(PORT, OLED_I2C_ADDR, data, 2, 0);
}

void OLED_SendData(I2C_Port_t PORT, uint8 dataByte)
{
    uint8 data[2];
    data[0] = 0x40;   // Control byte: Co=0, D/C#=1 (Data)
    data[1] = dataByte;
    I2C_MasterTransmit(PORT, OLED_I2C_ADDR, data, 2, 0);
}

//-----------------------------------------------------//
//                Initialization Sequence              //
//-----------------------------------------------------//
void OLED_Init(I2C_Port_t PORT)
{
    // Power-up delay (~100 ms)
    for (volatile uint32 i = 0; i < 100000; i++);

    // Recommended OLED initialization sequence
    OLED_SendCommand(PORT, 0xAE); // Display OFF
    OLED_SendCommand(PORT, 0xD5); // Set display clock divide ratio
    OLED_SendCommand(PORT, 0x80);
    OLED_SendCommand(PORT, 0xA8); // Multiplex ratio
    OLED_SendCommand(PORT, 0x3F);
    OLED_SendCommand(PORT, 0xD3); // Display offset
    OLED_SendCommand(PORT, 0x00);
    OLED_SendCommand(PORT, 0x40); // Start line = 0
    OLED_SendCommand(PORT, 0xAD); // DC-DC control mode set
    OLED_SendCommand(PORT, 0x8B);
    OLED_SendCommand(PORT, 0xA1); // Segment remap
    OLED_SendCommand(PORT, 0xC8); // COM scan direction
    OLED_SendCommand(PORT, 0xDA); // COM pins config
    OLED_SendCommand(PORT, 0x12);
    OLED_SendCommand(PORT, 0x81); // Contrast
    OLED_SendCommand(PORT, 0x80);
    OLED_SendCommand(PORT, 0xD9); // Pre-charge period
    OLED_SendCommand(PORT, 0x1F);
    OLED_SendCommand(PORT, 0xDB); // VCOMH deselect level
    OLED_SendCommand(PORT, 0x40);
    OLED_SendCommand(PORT, 0xA4); // Resume to RAM content
    OLED_SendCommand(PORT, 0xA6); // Normal display

    OLED_Clear();
    swapBuffer();
    OLED_UpdateScreen(PORT);

    OLED_SendCommand(PORT, 0xAF); // Display ON
}

//-----------------------------------------------------//
//                Framebuffer Operations               //
//-----------------------------------------------------//
void OLED_Clear(void)
{
    for (uint16 i = 0; i < sizeof(BufferA); i++)
        OLED_Buffer[i] = 0x00;
}

void OLED_DrawPixel(uint8 x, uint8 y, OLED_Color_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }    
    uint16 index = x + (y / 8) * OLED_WIDTH;

    if (color == OLED_COLOR_WHITE)
        OLED_Buffer[index] |= (1 << (y % 8));
    else
        OLED_Buffer[index] &= ~(1 << (y % 8));
}

void OLED_UpdateScreen(I2C_Port_t PORT)
{
    for (uint8 page = 0; page < OLED_PAGES; page++)
    {
        // Set page address and column start
        OLED_SendCommand(PORT, 0xB0 + page);                  // Page address
        OLED_SendCommand(PORT, 0x00 + (OLED_COLUMN_OFFSET & 0x0F)); // Lower column
        OLED_SendCommand(PORT, 0x10 + ((OLED_COLUMN_OFFSET >> 4) & 0x0F)); // Higher column

        // Prepare data buffer (1 control byte + page data)
        uint8 data[1 + OLED_WIDTH];
        data[0] = 0x40;  // Control byte for data

        for (uint8 col = 0; col < OLED_WIDTH; col++)
        {
            data[1 + col] = OLED_DisplayBuffer[page * OLED_WIDTH + col];
        }

        // Send entire page at once
       I2C_MasterTransmit(PORT, OLED_I2C_ADDR, data, sizeof(data), 0);
    }
}

