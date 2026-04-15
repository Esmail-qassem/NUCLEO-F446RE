#include "FLAPPY_Bird.h"
#include "FLAPPY_Bird_Cfg.h"

/*------------------------------------------------------------------
 *  Bird state
 *  Signed internally so velocity can be negative and Y can be
 *  clamped before being exposed as uint8.
 *------------------------------------------------------------------*/
static sint16 s_bird_y        = (sint16)BIRD_START_Y_PX;
static sint16 s_bird_velocity = 0;

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static void Bird_ClampY(void);

/*==================================================================
 *  BLOCK_DRAW — fill a width×height rectangle at (xcurser, ycurser)
 *================================================================*/
void BLOCK_DRAW(uint8 width, uint8 height, uint8 xcurser, uint8 ycurser)
{
    uint8 i;
    uint8 j;

    for (i = xcurser; i < (uint8)(width + xcurser); i++)
    {
        for (j = ycurser; j < (uint8)(height + ycurser); j++)
        {
            OLED_DrawPixel(i, j, OLED_COLOR_WHITE);
        }
    }
}

/*==================================================================
 *  Bird_Init — reset position and velocity to defaults
 *================================================================*/
void Bird_Init(void)
{
    s_bird_y        = (sint16)BIRD_START_Y_PX;
    s_bird_velocity = 0;
}

/*==================================================================
 *  Bird_Update
 *  Advance one physics tick: apply velocity to Y, then apply
 *  gravity to velocity (capped at terminal speed).
 *================================================================*/
void Bird_Update(void)
{
    s_bird_y += s_bird_velocity;

    s_bird_velocity += (sint16)BIRD_GRAVITY;
    if (s_bird_velocity > (sint16)BIRD_VELOCITY_MAX)
    {
        s_bird_velocity = (sint16)BIRD_VELOCITY_MAX;
    }

    Bird_ClampY();
}

/*==================================================================
 *  Bird_Jump — set an upward (negative) velocity
 *================================================================*/
void Bird_Jump(void)
{
    s_bird_velocity = (sint16)BIRD_JUMP_VELOCITY;
}

/*==================================================================
 *  Bird_GetY — current Y position in screen coordinates
 *================================================================*/
uint8 Bird_GetY(void)
{
    return (uint8)s_bird_y;
}

/*==================================================================
 *  Bird_Draw — render the bird at its current position
 *================================================================*/
void Bird_Draw(void)
{
    BLOCK_DRAW(BIRD_WIDTH_PX, BIRD_HEIGHT_PX, BIRD_X_PX, (uint8)s_bird_y);
}

/*==================================================================
 *  Bird_ClampY — keep the bird fully inside the display
 *================================================================*/
static void Bird_ClampY(void)
{
    const sint16 y_max = (sint16)OLED_HEIGHT - (sint16)BIRD_HEIGHT_PX;

    if (s_bird_y < 0)
    {
        s_bird_y        = 0;
        s_bird_velocity = 0;
    }
    else if (s_bird_y > y_max)
    {
        s_bird_y        = y_max;
        s_bird_velocity = 0;
    }
    else
    {
        /* in range — nothing to do */
    }
}
