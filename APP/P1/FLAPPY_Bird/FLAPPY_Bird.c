#include "FLAPPY_Bird.h"
#include "FLAPPY_Bird_Cfg.h"
#include "GPIO_interface.h"

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

/*##################################################################
 *  GAME ENGINE
 *  Pipes scroll right→left, bird collides, score on top, game-over
 *  freezes the frame until the button is pressed again.
 *##################################################################*/

typedef enum
{
    GAME_STATE_READY,
    GAME_STATE_PLAY,
    GAME_STATE_OVER
} Game_State_t;

typedef enum
{
    GAME_MODE_NORMAL,   /* 3 lives, lose one per hit                */
    GAME_MODE_EASY,     /* hits cost score; game-over at score 0    */
    GAME_MODE_MIXED,    /* lives first, then score-drain when empty */
    GAME_MODE_COUNT     /* sentinel — keeps cycling clean           */
} Game_Mode_t;

typedef struct
{
    sint16 x;       /* left edge, signed so it can scroll past 0 */
    uint8  gap_y;   /* top of the gap                            */
    uint8  gap_h;   /* gap height (difficulty-scaled)            */
    uint8  scored;  /* 1 once the bird has passed it             */
} Pipe_t;

static Game_State_t s_state;
static Game_Mode_t  s_mode = GAME_MODE_NORMAL;
static Pipe_t       s_pipes[PIPE_COUNT];
static uint16       s_score;
static uint8        s_lives;
static uint8        s_invuln;           /* i-frame down-counter   */
static uint16       s_rand;
static uint8        s_btn_jump_prev;
static uint8        s_btn_reset_prev;

/*------------------------------------------------------------------
 *  3x5 font for digits 0-9 — each row is 3 LSBs, bit2..bit0 = L..R
 *------------------------------------------------------------------*/
static const uint8 s_font3x5[10][SCORE_DIGIT_H_PX] =
{
    {0x7,0x5,0x5,0x5,0x7}, /* 0 */
    {0x2,0x6,0x2,0x2,0x7}, /* 1 */
    {0x7,0x1,0x7,0x4,0x7}, /* 2 */
    {0x7,0x1,0x7,0x1,0x7}, /* 3 */
    {0x5,0x5,0x7,0x1,0x1}, /* 4 */
    {0x7,0x4,0x7,0x1,0x7}, /* 5 */
    {0x7,0x4,0x7,0x5,0x7}, /* 6 */
    {0x7,0x1,0x1,0x1,0x1}, /* 7 */
    {0x7,0x5,0x7,0x5,0x7}, /* 8 */
    {0x7,0x5,0x7,0x1,0x7}, /* 9 */
};

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static uint8 Game_CurrentGap(void);
static uint8 Game_RandGapY(uint8 gap_h);
static void  Game_Reset(void);
static void  Game_OnHit(void);
static void  Pipe_Respawn(Pipe_t *p, sint16 x);
static void  Pipes_Update(void);
static void  Pipes_Draw(void);
static uint8 Pipes_CheckHit(void);
static void  Score_Draw(void);
static void  Lives_Draw(void);
static void  Mode_Draw(void);
static void  Digit_Draw(uint8 d, uint8 x, uint8 y);
static void  HLine_Draw(uint8 y);
static uint8 Button_Edge(PORT_t port, PIN_t pin, uint8 *prev);
static void  GameOver_Draw(void);

/*==================================================================
 *  Game_Init — power-on entry: go to READY screen
 *================================================================*/
void Game_Init(void)
{
    if (s_rand == 0U)
    {
        s_rand = 0xACE1U;   /* seed once, keep across restarts */
    }
    s_btn_jump_prev  = (uint8)!GAME_BTN_ACTIVE_LEVEL;
    s_btn_reset_prev = (uint8)!GAME_BTN_ACTIVE_LEVEL;

    Game_Reset();
    s_state = GAME_STATE_READY;
}

/*------------------------------------------------------------------
 *  Game_Reset — fresh round in the current mode
 *------------------------------------------------------------------*/
static void Game_Reset(void)
{
    uint8 i;

    Bird_Init();
    s_score  = 0U;
    s_invuln = 0U;

    switch (s_mode)
    {
        case GAME_MODE_EASY:  s_lives = GAME_LIVES_EASY;  break;
        case GAME_MODE_MIXED: s_lives = GAME_LIVES_MIXED; break;
        default:              s_lives = GAME_LIVES_NORMAL; break;
    }

    for (i = 0U; i < PIPE_COUNT; i++)
    {
        Pipe_Respawn(&s_pipes[i],
                     (sint16)OLED_WIDTH + (sint16)(i * PIPE_SPACING_PX));
    }
}

/*==================================================================
 *  Game_Task — one frame: input → physics → collide → render
 *================================================================*/
void Game_Task(void)
{
    uint8 jump  = Button_Edge(GAME_BTN_JUMP_PORT,  GAME_BTN_JUMP_PIN,
                              &s_btn_jump_prev);
    uint8 reset = Button_Edge(GAME_BTN_RESET_PORT, GAME_BTN_RESET_PIN,
                              &s_btn_reset_prev);

    switch (s_state)
    {
        case GAME_STATE_READY:
            if (reset != 0U)
            {
                s_mode = (Game_Mode_t)(((uint8)s_mode + 1U) % (uint8)GAME_MODE_COUNT);
                Game_Reset();           /* refresh lives for new mode */
            }
            if (jump != 0U)
            {
                s_state = GAME_STATE_PLAY;
                Bird_Jump();
            }
            break;

        case GAME_STATE_PLAY:
            if (reset != 0U)
            {
                Game_Reset();
                s_state = GAME_STATE_READY;
                break;
            }
            if (jump != 0U)
            {
                Bird_Jump();
            }
            Bird_Update();
            Pipes_Update();

            if (s_invuln > 0U)
            {
                s_invuln--;
            }
            else if (Pipes_CheckHit() != 0U)
            {
                Game_OnHit();
            }
            break;

        case GAME_STATE_OVER:
        default:
            if (reset != 0U)
            {
                Game_Reset();
                s_state = GAME_STATE_READY;
            }
            break;
    }

    /* ---- render ---- */
    OLED_Clear();
    HLine_Draw(GAME_FIELD_TOP_PX - 1U);
    Pipes_Draw();
    if ((s_invuln == 0U) || ((s_invuln & 1U) != 0U))
    {
        Bird_Draw();        /* blink during i-frames */
    }
    Score_Draw();
    Lives_Draw();
    Mode_Draw();
    if (s_state == GAME_STATE_OVER)
    {
        GameOver_Draw();
    }
    OLED_UpdateScreen(I2C1_PORT);
}

/*------------------------------------------------------------------
 *  Game_OnHit — apply mode-specific penalty
 *------------------------------------------------------------------*/
static void Game_OnHit(void)
{
    switch (s_mode)
    {
        case GAME_MODE_EASY:
            /* score shields the player; game-over at 0 */
            if (s_score > 0U)
            {
                s_score--;
                s_invuln = GAME_INVULN_TICKS;
            }
            else
            {
                s_state = GAME_STATE_OVER;
            }
            break;

        case GAME_MODE_MIXED:
            /* burn a life first; when lives are gone fall back to
               score-drain, then game-over when score is also 0    */
            if (s_lives > 0U)
            {
                s_lives--;
                s_invuln = GAME_INVULN_TICKS;
            }
            else if (s_score > 0U)
            {
                s_score--;
                s_invuln = GAME_INVULN_TICKS;
            }
            else
            {
                s_state = GAME_STATE_OVER;
            }
            break;

        case GAME_MODE_NORMAL:
        default:
            /* pure lives — game-over when they run out */
            if (s_lives > 0U)
            {
                s_lives--;
            }
            if (s_lives == 0U)
            {
                s_state = GAME_STATE_OVER;
            }
            else
            {
                s_invuln = GAME_INVULN_TICKS;
            }
            break;
    }
}

/*==================================================================
 *  Local helpers
 *================================================================*/

/*------------------------------------------------------------------
 *  Button_Edge — rising edge detect on a pin (per-button prev state)
 *------------------------------------------------------------------*/
static uint8 Button_Edge(PORT_t port, PIN_t pin, uint8 *prev)
{
    uint8 level = (uint8)!GAME_BTN_ACTIVE_LEVEL;
    uint8 edge  = 0U;

    (void)GPIO_ReadPin(port, pin, &level);

    if ((level == GAME_BTN_ACTIVE_LEVEL) && (*prev != GAME_BTN_ACTIVE_LEVEL))
    {
        edge = 1U;
    }
    *prev = level;
    return edge;
}

/*------------------------------------------------------------------
 *  Game_CurrentGap — gap height shrinks with score (difficulty)
 *------------------------------------------------------------------*/
static uint8 Game_CurrentGap(void)
{
    uint16 shrink = (s_score / PIPE_GAP_SHRINK_STEP) * PIPE_GAP_SHRINK_PX;
    uint16 gap    = PIPE_GAP_PX;

    if (shrink >= (gap - PIPE_GAP_MIN_LIMIT_PX))
    {
        return (uint8)PIPE_GAP_MIN_LIMIT_PX;
    }
    return (uint8)(gap - shrink);
}

/*------------------------------------------------------------------
 *  Game_RandGapY — 16-bit LFSR → gap-top in [MIN .. BOT-gap_h-4]
 *------------------------------------------------------------------*/
static uint8 Game_RandGapY(uint8 gap_h)
{
    uint8  y_max = (uint8)(GAME_FIELD_BOT_PX - gap_h - 4U);
    uint16 bit   = ((s_rand >> 0) ^ (s_rand >> 2) ^
                    (s_rand >> 3) ^ (s_rand >> 5)) & 1U;
    s_rand = (uint16)((s_rand >> 1) | (bit << 15));

    return (uint8)(PIPE_GAP_MIN_PX +
                   (s_rand % (uint16)(y_max - PIPE_GAP_MIN_PX + 1U)));
}

static void Pipe_Respawn(Pipe_t *p, sint16 x)
{
    p->x      = x;
    p->gap_h  = Game_CurrentGap();
    p->gap_y  = Game_RandGapY(p->gap_h);
    p->scored = 0U;
}

/*------------------------------------------------------------------
 *  Pipes_Update — scroll left, recycle, award score
 *------------------------------------------------------------------*/
static void Pipes_Update(void)
{
    uint8 i;

    for (i = 0U; i < PIPE_COUNT; i++)
    {
        s_pipes[i].x -= (sint16)PIPE_SPEED_PX;

        if ((s_pipes[i].scored == 0U) &&
            ((s_pipes[i].x + (sint16)PIPE_WIDTH_PX) < (sint16)BIRD_X_PX))
        {
            s_pipes[i].scored = 1U;
            s_score++;
        }

        if ((s_pipes[i].x + (sint16)PIPE_WIDTH_PX) < 0)
        {
            Pipe_Respawn(&s_pipes[i],
                         s_pipes[i].x + (sint16)(PIPE_COUNT * PIPE_SPACING_PX));
        }
    }
}

/*------------------------------------------------------------------
 *  Pipes_Draw — body + wider lip at the gap edges
 *------------------------------------------------------------------*/
static void Pipes_Draw(void)
{
    uint8 i;

    for (i = 0U; i < PIPE_COUNT; i++)
    {
        sint16 x0 = s_pipes[i].x;
        sint16 x1 = x0 + (sint16)PIPE_WIDTH_PX;
        uint8  gy = s_pipes[i].gap_y;
        uint8  gh = s_pipes[i].gap_h;
        uint8  vx;
        uint8  vw;
        uint8  lx;
        uint8  lw;

        if ((x1 <= 0) || (x0 >= (sint16)OLED_WIDTH))
        {
            continue;   /* fully off-screen */
        }

        /* clip body to screen */
        vx = (x0 < 0) ? 0U : (uint8)x0;
        vw = (uint8)(((x1 > (sint16)OLED_WIDTH) ? (sint16)OLED_WIDTH : x1) - (sint16)vx);

        /* top body */
        BLOCK_DRAW(vw, (uint8)(gy - GAME_FIELD_TOP_PX), vx, GAME_FIELD_TOP_PX);
        /* bottom body */
        BLOCK_DRAW(vw, (uint8)(GAME_FIELD_BOT_PX - (gy + gh)),
                   vx, (uint8)(gy + gh));

        /* lips (wider caps at the gap) — clip the same way */
        {
            sint16 lx0 = x0 - (sint16)PIPE_LIP_PX;
            sint16 lx1 = x1 + (sint16)PIPE_LIP_PX;
            if (lx0 < 0)                     { lx0 = 0; }
            if (lx1 > (sint16)OLED_WIDTH)    { lx1 = (sint16)OLED_WIDTH; }
            lx = (uint8)lx0;
            lw = (uint8)(lx1 - lx0);

            BLOCK_DRAW(lw, PIPE_LIP_HEIGHT_PX, lx,
                       (uint8)(gy - PIPE_LIP_HEIGHT_PX));
            BLOCK_DRAW(lw, PIPE_LIP_HEIGHT_PX, lx,
                       (uint8)(gy + gh));
        }
    }
}

/*------------------------------------------------------------------
 *  Pipes_CheckHit — AABB overlap of bird vs. any pipe column
 *------------------------------------------------------------------*/
static uint8 Pipes_CheckHit(void)
{
    uint8  i;
    uint8  by0 = Bird_GetY();
    uint8  by1 = (uint8)(by0 + BIRD_HEIGHT_PX);
    sint16 bx0 = (sint16)BIRD_X_PX;
    sint16 bx1 = bx0 + (sint16)BIRD_WIDTH_PX;

    for (i = 0U; i < PIPE_COUNT; i++)
    {
        sint16 px0 = s_pipes[i].x;
        sint16 px1 = px0 + (sint16)PIPE_WIDTH_PX;

        if ((bx1 > px0) && (bx0 < px1))
        {
            if ((by0 < s_pipes[i].gap_y) ||
                (by1 > (uint8)(s_pipes[i].gap_y + s_pipes[i].gap_h)))
            {
                return 1U;
            }
        }
    }
    return 0U;
}

/*------------------------------------------------------------------
 *  Score / digit rendering
 *------------------------------------------------------------------*/
static void Digit_Draw(uint8 d, uint8 x, uint8 y)
{
    uint8 row;
    uint8 col;

    for (row = 0U; row < SCORE_DIGIT_H_PX; row++)
    {
        uint8 bits = s_font3x5[d][row];
        for (col = 0U; col < SCORE_DIGIT_W_PX; col++)
        {
            if ((bits & (uint8)(1U << (SCORE_DIGIT_W_PX - 1U - col))) != 0U)
            {
                OLED_DrawPixel((uint8)(x + col), (uint8)(y + row),
                               OLED_COLOR_WHITE);
            }
        }
    }
}

static void Score_Draw(void)
{
    uint8  buf[5];
    uint8  len = 0U;
    uint8  i;
    uint16 v   = s_score;
    uint8  x   = SCORE_X_PX;

    do
    {
        buf[len++] = (uint8)(v % 10U);
        v /= 10U;
    } while ((v != 0U) && (len < sizeof(buf)));

    for (i = len; i > 0U; i--)
    {
        Digit_Draw(buf[i - 1U], x, SCORE_Y_PX);
        x += (uint8)(SCORE_DIGIT_W_PX + SCORE_DIGIT_GAP_PX);
    }
}

/*------------------------------------------------------------------
 *  Lives_Draw — small blocks, right-aligned in the HUD strip
 *------------------------------------------------------------------*/
static void Lives_Draw(void)
{
    uint8 i;
    uint8 x = LIVES_X_RIGHT_PX;

    for (i = 0U; i < s_lives; i++)
    {
        x -= (uint8)(LIVES_ICON_W_PX + LIVES_ICON_GAP_PX);
        BLOCK_DRAW(LIVES_ICON_W_PX, LIVES_ICON_H_PX, x, LIVES_Y_PX);
    }
}

/*------------------------------------------------------------------
 *  Mode_Draw — single pixel-row tag in HUD centre: 'E' or 'N'
 *  (kept tiny: 3x5 letter built from the digit font slot trick)
 *------------------------------------------------------------------*/
static void Mode_Draw(void)
{
    static const uint8 glyph_N[5] = {0x5,0x7,0x7,0x7,0x5};
    static const uint8 glyph_E[5] = {0x7,0x4,0x6,0x4,0x7};
    static const uint8 glyph_M[5] = {0x5,0x7,0x7,0x5,0x5};
    const uint8 *g;
    uint8 row;
    uint8 col;
    uint8 x = (uint8)(OLED_WIDTH / 2U - 1U);

    switch (s_mode)
    {
        case GAME_MODE_EASY:  g = glyph_E; break;
        case GAME_MODE_MIXED: g = glyph_M; break;
        default:              g = glyph_N; break;
    }

    for (row = 0U; row < 5U; row++)
    {
        for (col = 0U; col < 3U; col++)
        {
            if ((g[row] & (uint8)(1U << (2U - col))) != 0U)
            {
                OLED_DrawPixel((uint8)(x + col), (uint8)(SCORE_Y_PX + row),
                               OLED_COLOR_WHITE);
            }
        }
    }
}

static void HLine_Draw(uint8 y)
{
    uint8 x;
    for (x = 0U; x < OLED_WIDTH; x++)
    {
        OLED_DrawPixel(x, y, OLED_COLOR_WHITE);
    }
}

/*------------------------------------------------------------------
 *  GameOver_Draw — frozen scene + cross over the bird
 *------------------------------------------------------------------*/
static void GameOver_Draw(void)
{
    uint8 by = Bird_GetY();
    uint8 k;

    for (k = 0U; k < 8U; k++)
    {
        OLED_DrawPixel((uint8)(BIRD_X_PX - 2U + k), (uint8)(by - 2U + k),
                       OLED_COLOR_WHITE);
        OLED_DrawPixel((uint8)(BIRD_X_PX - 2U + k), (uint8)(by + 5U - k),
                       OLED_COLOR_WHITE);
    }
}
