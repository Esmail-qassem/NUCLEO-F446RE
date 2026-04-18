/***********************************************************************
 * File        : Snake_Display.c
 * Module      : Snake / Display manager
 ***********************************************************************/
#include "Snake_Display.h"
#include "OLED.h"

/*-----------------------------------------------------------------*/
/*  Tiny 5x7 ASCII font                                            */
/*  Each glyph is 5 columns of 7 bits (LSB = top row).             */
/*  Only the characters used in-game are defined; everything else  */
/*  renders as blank space so there is no risk of a bad lookup.    */
/*-----------------------------------------------------------------*/
#define FONT_W      (5U)                /* glyph width in pixels   */
#define FONT_H      (7U)                /* glyph height in pixels  */
#define FONT_PITCH  (FONT_W + 1U)       /* incl. one column gap    */

/* A small helper macro just to keep the glyph table readable. */
#define GLYPH(a,b,c,d,e)  { (a), (b), (c), (d), (e) }

/* Indexing: we store glyphs for space, digits, ':', and A..Z.
   We map ASCII -> table index in Font_GetGlyph().               */
static const uint8 FONT_TABLE[][FONT_W] =
{
    GLYPH(0x00, 0x00, 0x00, 0x00, 0x00), /*  0 : space */
    GLYPH(0x3E, 0x51, 0x49, 0x45, 0x3E), /*  1 : 0 */
    GLYPH(0x00, 0x42, 0x7F, 0x40, 0x00), /*  2 : 1 */
    GLYPH(0x42, 0x61, 0x51, 0x49, 0x46), /*  3 : 2 */
    GLYPH(0x21, 0x41, 0x45, 0x4B, 0x31), /*  4 : 3 */
    GLYPH(0x18, 0x14, 0x12, 0x7F, 0x10), /*  5 : 4 */
    GLYPH(0x27, 0x45, 0x45, 0x45, 0x39), /*  6 : 5 */
    GLYPH(0x3C, 0x4A, 0x49, 0x49, 0x30), /*  7 : 6 */
    GLYPH(0x01, 0x71, 0x09, 0x05, 0x03), /*  8 : 7 */
    GLYPH(0x36, 0x49, 0x49, 0x49, 0x36), /*  9 : 8 */
    GLYPH(0x06, 0x49, 0x49, 0x29, 0x1E), /* 10 : 9 */
    GLYPH(0x00, 0x36, 0x36, 0x00, 0x00), /* 11 : : */
    GLYPH(0x7E, 0x11, 0x11, 0x11, 0x7E), /* 12 : A */
    GLYPH(0x7F, 0x49, 0x49, 0x49, 0x36), /* 13 : B */
    GLYPH(0x3E, 0x41, 0x41, 0x41, 0x22), /* 14 : C */
    GLYPH(0x7F, 0x41, 0x41, 0x22, 0x1C), /* 15 : D */
    GLYPH(0x7F, 0x49, 0x49, 0x49, 0x41), /* 16 : E */
    GLYPH(0x7F, 0x09, 0x09, 0x09, 0x01), /* 17 : F */
    GLYPH(0x3E, 0x41, 0x49, 0x49, 0x7A), /* 18 : G */
    GLYPH(0x7F, 0x08, 0x08, 0x08, 0x7F), /* 19 : H */
    GLYPH(0x00, 0x41, 0x7F, 0x41, 0x00), /* 20 : I */
    GLYPH(0x20, 0x40, 0x41, 0x3F, 0x01), /* 21 : J */
    GLYPH(0x7F, 0x08, 0x14, 0x22, 0x41), /* 22 : K */
    GLYPH(0x7F, 0x40, 0x40, 0x40, 0x40), /* 23 : L */
    GLYPH(0x7F, 0x02, 0x0C, 0x02, 0x7F), /* 24 : M */
    GLYPH(0x7F, 0x04, 0x08, 0x10, 0x7F), /* 25 : N */
    GLYPH(0x3E, 0x41, 0x41, 0x41, 0x3E), /* 26 : O */
    GLYPH(0x7F, 0x09, 0x09, 0x09, 0x06), /* 27 : P */
    GLYPH(0x3E, 0x41, 0x51, 0x21, 0x5E), /* 28 : Q */
    GLYPH(0x7F, 0x09, 0x19, 0x29, 0x46), /* 29 : R */
    GLYPH(0x46, 0x49, 0x49, 0x49, 0x31), /* 30 : S */
    GLYPH(0x01, 0x01, 0x7F, 0x01, 0x01), /* 31 : T */
    GLYPH(0x3F, 0x40, 0x40, 0x40, 0x3F), /* 32 : U */
    GLYPH(0x1F, 0x20, 0x40, 0x20, 0x1F), /* 33 : V */
    GLYPH(0x3F, 0x40, 0x38, 0x40, 0x3F), /* 34 : W */
    GLYPH(0x63, 0x14, 0x08, 0x14, 0x63), /* 35 : X */
    GLYPH(0x07, 0x08, 0x70, 0x08, 0x07), /* 36 : Y */
    GLYPH(0x61, 0x51, 0x49, 0x45, 0x43)  /* 37 : Z */
};

/* Map a printable ASCII byte to an index in FONT_TABLE, or 0 (blank). */
static uint8 Font_GetIndex(char c)
{
    uint8 idx = 0U;

    if ((c >= '0') && (c <= '9'))
    {
        idx = (uint8)(1U + ((uint8)c - (uint8)'0'));
    }
    else if (c == ':')
    {
        idx = 11U;
    }
    else if ((c >= 'A') && (c <= 'Z'))
    {
        idx = (uint8)(12U + ((uint8)c - (uint8)'A'));
    }
    else if ((c >= 'a') && (c <= 'z'))
    {
        /* Fold lowercase to uppercase (font only carries uppercase). */
        idx = (uint8)(12U + ((uint8)c - (uint8)'a'));
    }
    else
    {
        idx = 0U;   /* space / unsupported */
    }

    return idx;
}

/*-----------------------------------------------------------------*/
/*  Low-level drawing primitives                                   */
/*-----------------------------------------------------------------*/

static void Fill_Rect(uint8 x, uint8 y, uint8 w, uint8 h, OLED_Color_t color)
{
    uint8 dx;
    uint8 dy;

    for (dy = 0U; dy < h; dy++)
    {
        for (dx = 0U; dx < w; dx++)
        {
            OLED_DrawPixel((uint8)(x + dx), (uint8)(y + dy), color);
        }
    }
}

static void Draw_Char(uint8 x, uint8 y, char c)
{
    uint8 idx = Font_GetIndex(c);
    uint8 col;
    uint8 row;

    for (col = 0U; col < FONT_W; col++)
    {
        uint8 bits = FONT_TABLE[idx][col];
        for (row = 0U; row < FONT_H; row++)
        {
            if ((bits & (uint8)(1U << row)) != 0U)
            {
                OLED_DrawPixel((uint8)(x + col), (uint8)(y + row), OLED_COLOR_WHITE);
            }
        }
    }
}

/* Render a 4-digit zero-padded decimal starting at (x,y). */
static void Draw_Uint4(uint8 x, uint8 y, uint16 value)
{
    char buf[5];
    uint8 i;

    /* Clamp to 9999 (4 displayed digits). */
    if (value > 9999U)
    {
        value = 9999U;
    }

    buf[0] = (char)('0' + (value / 1000U));
    buf[1] = (char)('0' + ((value / 100U) % 10U));
    buf[2] = (char)('0' + ((value / 10U) % 10U));
    buf[3] = (char)('0' + (value % 10U));
    buf[4] = '\0';

    for (i = 0U; buf[i] != '\0'; i++)
    {
        Draw_Char((uint8)(x + (uint8)(i * FONT_PITCH)), y, buf[i]);
    }
}

/*-----------------------------------------------------------------*/
/*  Public API                                                     */
/*-----------------------------------------------------------------*/
void SnakeDisplay_Clear(void)
{
    OLED_Clear();
}

void SnakeDisplay_Flush(void)
{
    OLED_UpdateScreen(I2C1_PORT);
}

void SnakeDisplay_DrawCell(uint8 grid_x, uint8 grid_y)
{
    uint8 px = (uint8)(grid_x * SNAKE_CELL_PX);
    uint8 py = (uint8)(SNAKE_STATUS_BAR_PX + (grid_y * SNAKE_CELL_PX));

    Fill_Rect(px, py, SNAKE_CELL_PX, SNAKE_CELL_PX, OLED_COLOR_WHITE);
}

void SnakeDisplay_DrawFood(uint8 grid_x, uint8 grid_y)
{
    /* Food is a centred 2x2 dot inside the 4x4 cell. Visually
       distinct from the solid-square snake body.                 */
    uint8 px = (uint8)((grid_x * SNAKE_CELL_PX) + 1U);
    uint8 py = (uint8)(SNAKE_STATUS_BAR_PX + (grid_y * SNAKE_CELL_PX) + 1U);

    Fill_Rect(px, py, 2U, 2U, OLED_COLOR_WHITE);
}

void SnakeDisplay_DrawMagicDot(uint8 grid_x, uint8 grid_y)
{
    /* Magic dot is a hollow 4x4 ring (12 border pixels on, inner 2x2
       left dark). Immediately distinguishable from the solid snake
       block and from the centred 2x2 food dot.                       */
    uint8 px = (uint8)(grid_x * SNAKE_CELL_PX);
    uint8 py = (uint8)(SNAKE_STATUS_BAR_PX + (grid_y * SNAKE_CELL_PX));

    /* Top edge (full 4 pixels). */
    Fill_Rect(px, py, SNAKE_CELL_PX, 1U, OLED_COLOR_WHITE);
    /* Bottom edge. */
    Fill_Rect(px, (uint8)(py + (SNAKE_CELL_PX - 1U)),
              SNAKE_CELL_PX, 1U, OLED_COLOR_WHITE);
    /* Left edge (interior rows only -- top/bottom already drawn). */
    Fill_Rect(px, (uint8)(py + 1U),
              1U, (uint8)(SNAKE_CELL_PX - 2U), OLED_COLOR_WHITE);
    /* Right edge. */
    Fill_Rect((uint8)(px + (SNAKE_CELL_PX - 1U)), (uint8)(py + 1U),
              1U, (uint8)(SNAKE_CELL_PX - 2U), OLED_COLOR_WHITE);
}

void SnakeDisplay_DrawText(uint8 x, uint8 y, const char *str)
{
    uint8 cx = x;

    if (str == NULL)
    {
        return;
    }

    while (*str != '\0')
    {
        Draw_Char(cx, y, *str);
        cx = (uint8)(cx + FONT_PITCH);
        str++;
    }
}

void SnakeDisplay_DrawStatusBar(uint16 score, uint16 high_score)
{
    /* Clear bar area (full 128x8 strip). */
    Fill_Rect(0U, 0U, 128U, SNAKE_STATUS_BAR_PX, OLED_COLOR_BLACK);

    /* "S:" label on the left. */
    SnakeDisplay_DrawText(0U, 0U, "S:");
    Draw_Uint4(12U, 0U, score);

    /* "H:" label on the right. */
    SnakeDisplay_DrawText(68U, 0U, "H:");
    Draw_Uint4(80U, 0U, high_score);

    /* Thin separator line between bar and play area. */
    Fill_Rect(0U, (uint8)(SNAKE_STATUS_BAR_PX - 1U), 128U, 1U, OLED_COLOR_WHITE);
}

void SnakeDisplay_ShowWelcome(uint16 high_score)
{
    OLED_Clear();

    /* "SNAKE" (5 chars -> 30 px) centred horizontally. */
    SnakeDisplay_DrawText(49U, 10U, "SNAKE");

    /* "PRESS ANY KEY" (13 chars -> 78 px). */
    SnakeDisplay_DrawText(25U, 30U, "PRESS ANY KEY");

    /* "TO START" (8 chars -> 48 px). */
    SnakeDisplay_DrawText(40U, 42U, "TO START");

    /* "HI:NNNN" bottom-right. */
    SnakeDisplay_DrawText(46U, 55U, "HI:");
    Draw_Uint4(64U, 55U, high_score);

    SnakeDisplay_Flush();
}

void SnakeDisplay_ShowGameOver(uint16 score, uint16 high_score)
{
    OLED_Clear();

    /* "GAME OVER" (9 chars -> 54 px). */
    SnakeDisplay_DrawText(37U, 4U, "GAME OVER");

    /* "SCORE:NNNN" (10 chars -> 60 px). */
    SnakeDisplay_DrawText(28U, 20U, "SCORE:");
    Draw_Uint4(64U, 20U, score);

    /* "HI:NNNN". */
    SnakeDisplay_DrawText(40U, 34U, "HI:");
    Draw_Uint4(58U, 34U, high_score);

    /* "ANY KEY" prompt. */
    SnakeDisplay_DrawText(22U, 52U, "PRESS TO RETRY");

    SnakeDisplay_Flush();
}
