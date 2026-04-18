/***********************************************************************
 * File        : Snake_Input.c
 * Module      : Snake / Input handling
 ***********************************************************************/
#include "Snake_Input.h"
#include "GPIO_interface.h"

/*-----------------------------------------------------------------*/
/*  Hardware mapping                                               */
/*-----------------------------------------------------------------*/
#define BTN_PORT       GPIO_PORTC

/* Array-based mapping keeps the polling loop small and generic. */
#define BTN_COUNT      (4U)

static const PIN_t BTN_PINS[BTN_COUNT] =
{
    PIN0,   /* UP    */
    PIN1,   /* DOWN  */
    PIN2,   /* LEFT  */
    PIN4    /* RIGHT (PC3 is reserved for bootloader) */
};

static const Snake_Dir_t BTN_DIRS[BTN_COUNT] =
{
    SNAKE_DIR_UP,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT
};

/*-----------------------------------------------------------------*/
/*  Debounce + edge detection state                                */
/*-----------------------------------------------------------------*/
static uint8       s_stable[BTN_COUNT];    /* last stable level  */
static uint8       s_previous[BTN_COUNT];  /* previous raw level */
static uint8       s_pending_flag = 0U;    /* 1 if direction ready */
static Snake_Dir_t s_pending_dir  = SNAKE_DIR_NONE;
static uint8       s_any_flag     = 0U;    /* 1 if any key just pressed */

/*-----------------------------------------------------------------*/
/*  Public API                                                     */
/*-----------------------------------------------------------------*/
void SnakeInput_Init(void)
{
    uint8 i;

    for (i = 0U; i < BTN_COUNT; i++)
    {
        (void)GPIO_InitPin(BTN_PORT, BTN_PINS[i],
                           GPIO_MODE_INPUT,
                           GPIO_OTYPE_PP,
                           GPIO_SPEED_FAST,
                           GPIO_PULL_UP);

        /* Buttons idle HIGH (pull-up). */
        s_stable[i]   = 1U;
        s_previous[i] = 1U;
    }

    s_pending_flag = 0U;
    s_any_flag     = 0U;
    s_pending_dir  = SNAKE_DIR_NONE;
}

void SnakeInput_Poll(void)
{
    uint8 i;

    for (i = 0U; i < BTN_COUNT; i++)
    {
        uint8 raw = 1U;

        (void)GPIO_ReadPin(BTN_PORT, BTN_PINS[i], &raw);

        /* Two consecutive identical samples => accept as stable. */
        if ((raw == s_previous[i]) && (raw != s_stable[i]))
        {
            /* Falling edge (button pressed) -> latch event. */
            if (raw == 0U)
            {
                s_pending_dir  = BTN_DIRS[i];
                s_pending_flag = 1U;
                s_any_flag     = 1U;
            }
            s_stable[i] = raw;
        }

        s_previous[i] = raw;
    }
}

uint8 SnakeInput_GetDirection(Snake_Dir_t *dir)
{
    uint8 has_event = 0U;

    if ((dir != NULL) && (s_pending_flag == 1U))
    {
        *dir           = s_pending_dir;
        s_pending_flag = 0U;
        has_event      = 1U;
    }

    return has_event;
}

uint8 SnakeInput_AnyKeyPressed(void)
{
    uint8 pressed = s_any_flag;

    /* Consume both flags; a fresh press is needed for the next query. */
    s_any_flag     = 0U;
    s_pending_flag = 0U;

    return pressed;
}
