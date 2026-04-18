/***********************************************************************
 * File        : Snake_Game.c
 * Module      : Snake / Game logic
 *
 * Responsibilities:
 *   - Owns the finite state machine (WELCOME / PLAYING / GAMEOVER).
 *   - Maintains the snake body as a circular buffer of cell coords.
 *   - Handles movement, wall collision, self collision and growth.
 *   - Spawns food on random free cells using a 16-bit Galois LFSR.
 *   - Runs the "magical dot" event (random spawn, 5 s lifetime) and
 *     the bonus mode it triggers (5 foods, 10 s window) as a
 *     sub-state of PLAYING.
 *   - Delegates rendering to Snake_Display and persistence to
 *     Snake_Score; only I/O module touched directly is the input one
 *     (via Snake_Input API).
 ***********************************************************************/
#include "Snake_Game.h"
#include "Snake_Input.h"
#include "Snake_Display.h"
#include "Snake_Score.h"

/*-----------------------------------------------------------------*/
/*  Scheduler parameters                                           */
/*-----------------------------------------------------------------*/
#define SNAKE_POLL_PERIOD_MS   (20U)

/* Static bounds for the 20 ms-call divider, derived from the ms values
   in the header. These are compile-time constants so we never do any
   division at runtime.                                                 */
#define SNAKE_STEPS_INIT       ((uint8)(SNAKE_TICK_MS_INIT / SNAKE_POLL_PERIOD_MS))  /* 10 */
#define SNAKE_STEPS_MIN        ((uint8)(SNAKE_TICK_MS_MIN  / SNAKE_POLL_PERIOD_MS))  /*  3 */
#define SNAKE_STEPS_DELTA      ((uint8)(SNAKE_TICK_MS_STEP / SNAKE_POLL_PERIOD_MS))  /*  1 */

/* Magic-dot / bonus timers measured in 20 ms-call units so the main
   tick hook can just decrement them. All divisions happen at compile
   time (constant ms values over a constant period).                    */
#define SNAKE_MAGIC_TTL_TICKS           ((uint16)(SNAKE_MAGIC_TTL_MS            / SNAKE_POLL_PERIOD_MS))  /* 250 */
#define SNAKE_BONUS_DURATION_TICKS      ((uint16)(SNAKE_BONUS_DURATION_MS       / SNAKE_POLL_PERIOD_MS))  /* 500 */
#define SNAKE_MAGIC_COOLDOWN_MIN_TICKS  ((uint16)(SNAKE_MAGIC_COOLDOWN_MIN_MS   / SNAKE_POLL_PERIOD_MS))  /* 500 */
#define SNAKE_MAGIC_COOLDOWN_RNG_TICKS  ((uint16)(SNAKE_MAGIC_COOLDOWN_RANGE_MS / SNAKE_POLL_PERIOD_MS))  /* 500 */

/*-----------------------------------------------------------------*/
/*  Internal state                                                 */
/*-----------------------------------------------------------------*/
static Snake_State_t s_state;

/* Circular buffer of body cells.
   Layout: body[tail_idx], body[(tail_idx+1) % MAX], ... body[head_idx]
   Body length is stored explicitly for clarity.                    */
static uint8  s_body_x[SNAKE_MAX_LENGTH];
static uint8  s_body_y[SNAKE_MAX_LENGTH];
static uint16 s_head_idx;
static uint16 s_tail_idx;
static uint16 s_length;

/* Current travel direction and the direction queued by the last input
   event. We only commit the queued direction at step-time so the user
   cannot reverse into themselves by pressing two keys during one tick. */
static Snake_Dir_t s_dir;
static Snake_Dir_t s_dir_queued;

/* Food storage.
   During normal play s_food_count == 1; during bonus mode it is
   initialised to SNAKE_BONUS_FOODS. Eaten entries are removed by
   swapping with the last slot, so order does not matter.            */
static uint8 s_food_x[SNAKE_BONUS_FOODS];
static uint8 s_food_y[SNAKE_BONUS_FOODS];
static uint8 s_food_count;

/* Magical dot.
   When active, occupies one cell on the grid for up to
   SNAKE_MAGIC_TTL_TICKS before disappearing. Eating it triggers
   bonus mode (it does NOT grow the snake or add score).             */
static uint8  s_magic_active;
static uint8  s_magic_x;
static uint8  s_magic_y;
static uint16 s_magic_ttl;           /* ticks remaining while active */
static uint16 s_magic_cooldown;      /* ticks until next spawn       */

/* Bonus mode (sub-state of PLAYING). */
static uint8  s_bonus_active;
static uint16 s_bonus_ticks;         /* ticks remaining in bonus     */

/* Sub-divider from 20 ms calls to the current step period. */
static uint8 s_step_divider;

/* 16-bit LFSR for pseudo-random numbers.
   Seed is perturbed by the 20 ms counter until the user starts, so the
   game is effectively non-deterministic from run to run.               */
static uint16 s_rng_state = 0xACE1U;

/*-----------------------------------------------------------------*/
/*  PRNG                                                           */
/*-----------------------------------------------------------------*/
static uint16 Rng_Next(void)
{
    /* Classic 16-bit Galois LFSR, taps at 0xB400 (x^16+x^14+x^13+x^11+1). */
    uint8 lsb = (uint8)(s_rng_state & 1U);
    s_rng_state = (uint16)(s_rng_state >> 1);
    if (lsb != 0U)
    {
        s_rng_state ^= (uint16)0xB400U;
    }
    return s_rng_state;
}

/*-----------------------------------------------------------------*/
/*  Snake body helpers                                             */
/*-----------------------------------------------------------------*/

/* Returns 1 if (x,y) is currently part of the snake body. */
static uint8 Body_Contains(uint8 x, uint8 y)
{
    uint16 i;
    uint16 idx = s_tail_idx;
    uint8  hit = 0U;

    for (i = 0U; (i < s_length) && (hit == 0U); i++)
    {
        if ((s_body_x[idx] == x) && (s_body_y[idx] == y))
        {
            hit = 1U;
        }
        idx = (uint16)((idx + 1U) % SNAKE_MAX_LENGTH);
    }

    return hit;
}

/* Returns 1 if (x,y) is occupied by anything we know about: the
   snake body, any currently-active food or the magical dot.         */
static uint8 Cell_Occupied(uint8 x, uint8 y)
{
    uint8 i;

    if (Body_Contains(x, y) == 1U)
    {
        return 1U;
    }

    for (i = 0U; i < s_food_count; i++)
    {
        if ((s_food_x[i] == x) && (s_food_y[i] == y))
        {
            return 1U;
        }
    }

    if ((s_magic_active == 1U) && (s_magic_x == x) && (s_magic_y == y))
    {
        return 1U;
    }

    return 0U;
}

/* Pick a free cell into (*out_x, *out_y). Returns 1 on success, 0
   if the board is full. Tries random draws first, then falls back
   to a deterministic linear scan so placement always succeeds when
   at least one cell is free.                                        */
static uint8 Food_PickFreeCell(uint8 *out_x, uint8 *out_y)
{
    uint16 attempts = 0U;
    uint8  cx;
    uint8  cy;

    while (attempts < (uint16)(SNAKE_MAX_LENGTH * 2U))
    {
        cx = (uint8)(Rng_Next() % SNAKE_GRID_W);
        cy = (uint8)(Rng_Next() % SNAKE_GRID_H);

        if (Cell_Occupied(cx, cy) == 0U)
        {
            *out_x = cx;
            *out_y = cy;
            return 1U;
        }
        attempts++;
    }

    for (cy = 0U; cy < SNAKE_GRID_H; cy++)
    {
        for (cx = 0U; cx < SNAKE_GRID_W; cx++)
        {
            if (Cell_Occupied(cx, cy) == 0U)
            {
                *out_x = cx;
                *out_y = cy;
                return 1U;
            }
        }
    }

    return 0U;
}

/* Spawn 'count' new foods. Appends to s_food_x / s_food_y starting
   at the current s_food_count. Stops early if the board fills up.   */
static void Food_SpawnN(uint8 count)
{
    uint8 i;
    uint8 x;
    uint8 y;

    for (i = 0U; i < count; i++)
    {
        if (s_food_count >= SNAKE_BONUS_FOODS)
        {
            break;
        }
        if (Food_PickFreeCell(&x, &y) == 0U)
        {
            break;
        }
        s_food_x[s_food_count] = x;
        s_food_y[s_food_count] = y;
        s_food_count++;
    }
}

/* Remove the food at index 'idx' by swapping with the last slot. */
static void Food_Remove(uint8 idx)
{
    uint8 last;

    if ((s_food_count == 0U) || (idx >= s_food_count))
    {
        return;
    }

    last = (uint8)(s_food_count - 1U);
    if (idx != last)
    {
        s_food_x[idx] = s_food_x[last];
        s_food_y[idx] = s_food_y[last];
    }
    s_food_count--;
}

/* Find a food slot at (x,y). Returns SNAKE_BONUS_FOODS if none. */
static uint8 Food_FindAt(uint8 x, uint8 y)
{
    uint8 i;

    for (i = 0U; i < s_food_count; i++)
    {
        if ((s_food_x[i] == x) && (s_food_y[i] == y))
        {
            return i;
        }
    }
    return (uint8)SNAKE_BONUS_FOODS;
}

/*-----------------------------------------------------------------*/
/*  Magical-dot / bonus helpers                                    */
/*-----------------------------------------------------------------*/

/* Roll a new cooldown value in [MIN, MIN+RANGE-1] ticks. */
static void Magic_RollCooldown(void)
{
    uint16 r = (uint16)(Rng_Next() % SNAKE_MAGIC_COOLDOWN_RNG_TICKS);
    s_magic_cooldown = (uint16)(SNAKE_MAGIC_COOLDOWN_MIN_TICKS + r);
}

/* Try to place the magic dot on a free cell. */
static void Magic_Spawn(void)
{
    uint8 x;
    uint8 y;

    if (Food_PickFreeCell(&x, &y) == 1U)
    {
        s_magic_x      = x;
        s_magic_y      = y;
        s_magic_active = 1U;
        s_magic_ttl    = SNAKE_MAGIC_TTL_TICKS;
    }
    else
    {
        /* Board too full: try again soon. */
        Magic_RollCooldown();
    }
}

/* Replace current food layout with 5 bonus foods and start the
   10 s bonus timer. Called when the magic dot is eaten.            */
static void Bonus_Enter(void)
{
    s_bonus_active = 1U;
    s_bonus_ticks  = SNAKE_BONUS_DURATION_TICKS;
    s_food_count   = 0U;
    Food_SpawnN((uint8)SNAKE_BONUS_FOODS);
}

/* Leave bonus mode: clear any leftover bonus foods and place the
   single normal food. Also roll a fresh magic-dot cooldown so the
   next magic dot does not appear instantly after bonus ends.       */
static void Bonus_Exit(void)
{
    s_bonus_active = 0U;
    s_bonus_ticks  = 0U;
    s_food_count   = 0U;
    Food_SpawnN(1U);
    Magic_RollCooldown();
}

/* Timer tick for the magic-dot / bonus subsystem. Called every 20 ms
   from Snake_Tick, regardless of whether the snake steps this tick. */
static void Magic_Tick(void)
{
    /* Magic dot lifetime countdown. */
    if (s_magic_active == 1U)
    {
        if (s_magic_ttl > 0U)
        {
            s_magic_ttl--;
        }
        if (s_magic_ttl == 0U)
        {
            /* Uneaten -> disappears and we wait for the next cooldown. */
            s_magic_active = 0U;
            Magic_RollCooldown();
        }
    }
    /* Only try to spawn a new magic dot when we are in normal play:
       no dot active, no bonus running.                                */
    else if (s_bonus_active == 0U)
    {
        if (s_magic_cooldown > 0U)
        {
            s_magic_cooldown--;
        }
        if (s_magic_cooldown == 0U)
        {
            Magic_Spawn();
        }
    }
    else
    {
        /* Bonus running; magic dot suppressed. */
    }

    /* Bonus-mode countdown. */
    if (s_bonus_active == 1U)
    {
        if (s_bonus_ticks > 0U)
        {
            s_bonus_ticks--;
        }
        if (s_bonus_ticks == 0U)
        {
            Bonus_Exit();
        }
    }
}

/*-----------------------------------------------------------------*/
/*  Game reset / start                                             */
/*-----------------------------------------------------------------*/

/* Prepare a fresh round: snake of length 3 centred, moving right. */
static void Game_StartRound(void)
{
    uint8  i;
    uint8  start_x = (uint8)(SNAKE_GRID_W / 2U);
    uint8  start_y = (uint8)(SNAKE_GRID_H / 2U);

    s_length   = SNAKE_INIT_LENGTH;
    s_tail_idx = 0U;
    s_head_idx = (uint16)(SNAKE_INIT_LENGTH - 1U);

    /* Tail is left-most, head is right-most (three cells in a row). */
    for (i = 0U; i < SNAKE_INIT_LENGTH; i++)
    {
        s_body_x[i] = (uint8)(start_x - (SNAKE_INIT_LENGTH - 1U - i));
        s_body_y[i] = start_y;
    }

    s_dir        = SNAKE_DIR_RIGHT;
    s_dir_queued = SNAKE_DIR_RIGHT;

    /* Clear magical-dot and bonus state, roll an initial cooldown
       so the first magic dot cannot appear on the very first step.  */
    s_food_count     = 0U;
    s_magic_active   = 0U;
    s_magic_ttl      = 0U;
    s_bonus_active   = 0U;
    s_bonus_ticks    = 0U;
    Magic_RollCooldown();

    SnakeScore_Reset();
    Food_SpawnN(1U);
    s_step_divider = 0U;
}

/*-----------------------------------------------------------------*/
/*  Direction handling                                             */
/*-----------------------------------------------------------------*/

/* Returns 1 if 'new_dir' is exactly the opposite of 'cur'. */
static uint8 Dir_IsOpposite(Snake_Dir_t cur, Snake_Dir_t new_dir)
{
    uint8 is_opp = 0U;
    if (((cur == SNAKE_DIR_UP)    && (new_dir == SNAKE_DIR_DOWN))  ||
        ((cur == SNAKE_DIR_DOWN)  && (new_dir == SNAKE_DIR_UP))    ||
        ((cur == SNAKE_DIR_LEFT)  && (new_dir == SNAKE_DIR_RIGHT)) ||
        ((cur == SNAKE_DIR_RIGHT) && (new_dir == SNAKE_DIR_LEFT)))
    {
        is_opp = 1U;
    }
    return is_opp;
}

/*-----------------------------------------------------------------*/
/*  One movement step                                              */
/*-----------------------------------------------------------------*/
static void Game_Step(void)
{
    uint8  head_x = s_body_x[s_head_idx];
    uint8  head_y = s_body_y[s_head_idx];
    sint16 nx;
    sint16 ny;
    uint8  food_idx;
    uint8  ate_food;
    uint8  ate_magic;
    uint8  grow;
    uint8  collided;

    /* Commit the queued direction if it is not a 180 degree reverse. */
    if (Dir_IsOpposite(s_dir, s_dir_queued) == 0U)
    {
        s_dir = s_dir_queued;
    }

    /* Compute the next head cell. */
    nx = (sint16)head_x;
    ny = (sint16)head_y;
    switch (s_dir)
    {
        case SNAKE_DIR_UP:    ny--; break;
        case SNAKE_DIR_DOWN:  ny++; break;
        case SNAKE_DIR_LEFT:  nx--; break;
        case SNAKE_DIR_RIGHT: nx++; break;
        default:              /* unreachable */ break;
    }

    /* Wall collision -> game over. */
    if ((nx < 0) || (nx >= (sint16)SNAKE_GRID_W) ||
        (ny < 0) || (ny >= (sint16)SNAKE_GRID_H))
    {
        s_state = SNAKE_STATE_GAMEOVER;
        SnakeScore_Persist();
        return;
    }

    /* What is the new head stepping on?
       - ate_food  -> grows the snake and scores +1 (normal rules).
       - ate_magic -> triggers bonus mode; no growth, no score.       */
    food_idx  = Food_FindAt((uint8)nx, (uint8)ny);
    ate_food  = (food_idx < SNAKE_BONUS_FOODS) ? 1U : 0U;
    ate_magic = ((s_magic_active == 1U) &&
                 (s_magic_x == (uint8)nx) &&
                 (s_magic_y == (uint8)ny)) ? 1U : 0U;
    grow      = ate_food;

    /* Self collision check.
       Without growth, the current tail moves away this step so it is
       not part of the obstacle set. With growth the tail stays and
       must be included.                                                */
    collided = 0U;
    {
        uint16 i;
        uint16 start = (grow == 1U) ? 0U : 1U;
        uint16 idx   = (uint16)((s_tail_idx + start) % SNAKE_MAX_LENGTH);

        for (i = start; i < s_length; i++)
        {
            if ((s_body_x[idx] == (uint8)nx) && (s_body_y[idx] == (uint8)ny))
            {
                collided = 1U;
                break;
            }
            idx = (uint16)((idx + 1U) % SNAKE_MAX_LENGTH);
        }
    }

    if (collided == 1U)
    {
        s_state = SNAKE_STATE_GAMEOVER;
        SnakeScore_Persist();
        return;
    }

    /* Advance body: push head forward, pop tail unless growing. */
    s_head_idx = (uint16)((s_head_idx + 1U) % SNAKE_MAX_LENGTH);
    s_body_x[s_head_idx] = (uint8)nx;
    s_body_y[s_head_idx] = (uint8)ny;

    if (grow == 1U)
    {
        if (s_length < SNAKE_MAX_LENGTH)
        {
            s_length++;
        }
        SnakeScore_Increment();
        Food_Remove(food_idx);

        /* In normal play, always keep exactly 1 food on the board.
           In bonus mode, do NOT respawn -- the 5 foods are a fixed
           set; bonus ends when they are all eaten (below).           */
        if ((s_bonus_active == 0U) && (s_food_count == 0U))
        {
            Food_SpawnN(1U);
        }
    }
    else
    {
        s_tail_idx = (uint16)((s_tail_idx + 1U) % SNAKE_MAX_LENGTH);
    }

    /* Magic-dot consumption -> enter bonus mode (overrides any
       pending food spawn: Bonus_Enter resets the food set to 5).    */
    if (ate_magic == 1U)
    {
        s_magic_active = 0U;
        Bonus_Enter();
    }

    /* If bonus finished early because all 5 foods were eaten,
       fall back to normal play immediately.                         */
    if ((s_bonus_active == 1U) && (s_food_count == 0U))
    {
        Bonus_Exit();
    }
}

/*-----------------------------------------------------------------*/
/*  Rendering                                                      */
/*-----------------------------------------------------------------*/
static void Game_Render(void)
{
    uint16 i;
    uint8  fi;
    uint16 idx = s_tail_idx;

    SnakeDisplay_Clear();
    SnakeDisplay_DrawStatusBar(SnakeScore_GetCurrent(), SnakeScore_GetHigh());

    /* Snake body (solid 4x4 blocks). */
    for (i = 0U; i < s_length; i++)
    {
        SnakeDisplay_DrawCell(s_body_x[idx], s_body_y[idx]);
        idx = (uint16)((idx + 1U) % SNAKE_MAX_LENGTH);
    }

    /* All active food items (1 in normal play, up to 5 in bonus). */
    for (fi = 0U; fi < s_food_count; fi++)
    {
        SnakeDisplay_DrawFood(s_food_x[fi], s_food_y[fi]);
    }

    /* Magical dot, if present. */
    if (s_magic_active == 1U)
    {
        SnakeDisplay_DrawMagicDot(s_magic_x, s_magic_y);
    }

    SnakeDisplay_Flush();
}

/*-----------------------------------------------------------------*/
/*  Difficulty -> step divider mapping                             */
/*-----------------------------------------------------------------*/
/* Current 20 ms-call count required for one game step.
   At score 0 this is SNAKE_STEPS_INIT (=10, i.e. 200 ms). Every
   SNAKE_SPEEDUP_EVERY foods eaten we subtract SNAKE_STEPS_DELTA,
   clamped to SNAKE_STEPS_MIN (=3, i.e. 60 ms).                      */
static uint8 Game_StepsForScore(uint16 score)
{
    uint16 bracket = (uint16)(score / SNAKE_SPEEDUP_EVERY);
    uint16 reduction = (uint16)(bracket * (uint16)SNAKE_STEPS_DELTA);
    uint16 steps;

    if (reduction >= (uint16)(SNAKE_STEPS_INIT - SNAKE_STEPS_MIN))
    {
        steps = (uint16)SNAKE_STEPS_MIN;
    }
    else
    {
        steps = (uint16)((uint16)SNAKE_STEPS_INIT - reduction);
    }

    return (uint8)steps;
}

/*-----------------------------------------------------------------*/
/*  Public API                                                     */
/*-----------------------------------------------------------------*/
void Snake_Init(void)
{
    SnakeInput_Init();
    SnakeScore_Init();

    s_state        = SNAKE_STATE_WELCOME;
    s_step_divider = 0U;

    SnakeDisplay_ShowWelcome(SnakeScore_GetHigh());
}

void Snake_Tick(void)
{
    /* Keep the PRNG moving even before the user starts, so the first
       food location is essentially unpredictable.                    */
    s_rng_state = (uint16)(s_rng_state + 0x9E37U);

    /* Sample inputs every call (20 ms) for responsive debounce. */
    SnakeInput_Poll();

    switch (s_state)
    {
        case SNAKE_STATE_WELCOME:
        {
            if (SnakeInput_AnyKeyPressed() == 1U)
            {
                Game_StartRound();
                s_state = SNAKE_STATE_PLAYING;
                Game_Render();
            }
            break;
        }

        case SNAKE_STATE_PLAYING:
        {
            Snake_Dir_t pressed;
            uint8       magic_before = s_magic_active;
            uint8       bonus_before = s_bonus_active;

            /* Latest directional press -> queued direction. */
            if (SnakeInput_GetDirection(&pressed) == 1U)
            {
                s_dir_queued = pressed;
            }

            /* Advance magic-dot / bonus timers every 20 ms call
               (independent of the snake step rate).                 */
            Magic_Tick();

            s_step_divider++;
            if (s_step_divider >= Game_StepsForScore(SnakeScore_GetCurrent()))
            {
                s_step_divider = 0U;
                Game_Step();

                if (s_state == SNAKE_STATE_GAMEOVER)
                {
                    SnakeDisplay_ShowGameOver(SnakeScore_GetCurrent(),
                                              SnakeScore_GetHigh());
                }
                else
                {
                    Game_Render();
                }
            }
            else if ((s_magic_active != magic_before) ||
                     (s_bonus_active != bonus_before))
            {
                /* Magic dot appeared/disappeared or bonus mode
                   transitioned between steps -- re-render so the
                   player sees it without waiting for the next step. */
                Game_Render();
            }
            else
            {
                /* No visible change this tick -> skip repaint. */
            }
            break;
        }

        case SNAKE_STATE_GAMEOVER:
        {
            if (SnakeInput_AnyKeyPressed() == 1U)
            {
                /* Back to welcome so the player sees the intro again. */
                s_state = SNAKE_STATE_WELCOME;
                SnakeDisplay_ShowWelcome(SnakeScore_GetHigh());
            }
            break;
        }

        default:
        {
            /* Defensive: should never happen. */
            s_state = SNAKE_STATE_WELCOME;
            break;
        }
    }
}

Snake_State_t Snake_GetState(void)
{
    return s_state;
}
