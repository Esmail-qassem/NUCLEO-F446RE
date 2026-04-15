#ifndef FLAPPY_BIRD_CFG_H_
#define FLAPPY_BIRD_CFG_H_

/*==================================================================
 *  FLAPPY_Bird_Cfg.h
 *  Tunables for the Flappy-Bird game module.
 *  Screen Y grows downward (OLED convention), so positive velocity
 *  moves the bird down and a jump sets a negative velocity.
 *================================================================*/

/*------------------------------------------------------------------
 *  Bird geometry (pixels)
 *------------------------------------------------------------------*/
#define BIRD_WIDTH_PX           (4U)
#define BIRD_HEIGHT_PX          (4U)
#define BIRD_X_PX               (16U)   /* fixed horizontal position */

/*------------------------------------------------------------------
 *  Physics (units: pixels per Bird_Update() call)
 *------------------------------------------------------------------*/
#define BIRD_START_Y_PX         (32)    /* OLED_HEIGHT / 2           */
#define BIRD_GRAVITY            (1)     /* added to velocity each tick */
#define BIRD_JUMP_VELOCITY      (-5)    /* applied on Bird_Jump()    */
#define BIRD_VELOCITY_MAX       (6)     /* terminal fall speed       */

/*------------------------------------------------------------------
 *  Play-field (leave a strip at the top for the score)
 *------------------------------------------------------------------*/
#define GAME_FIELD_TOP_PX       (8U)    /* first playable row        */
#define GAME_FIELD_BOT_PX       (OLED_HEIGHT)

/*------------------------------------------------------------------
 *  Pipe / obstacle geometry (pixels)
 *------------------------------------------------------------------*/
#define PIPE_COUNT              (2U)    /* pipes on screen at once   */
#define PIPE_WIDTH_PX           (10U)
#define PIPE_GAP_PX             (24U)   /* vertical opening          */
#define PIPE_LIP_PX             (2U)    /* lip overhang each side    */
#define PIPE_LIP_HEIGHT_PX      (3U)
#define PIPE_SPACING_PX         (64U)   /* horizontal distance       */
#define PIPE_SPEED_PX           (2U)    /* scroll per game tick      */
#define PIPE_GAP_MIN_PX         (GAME_FIELD_TOP_PX + 4U)
#define PIPE_GAP_MAX_PX         (GAME_FIELD_BOT_PX - PIPE_GAP_PX - 4U)

/*------------------------------------------------------------------
 *  Score digits (3x5 bitmap font)
 *------------------------------------------------------------------*/
#define SCORE_DIGIT_W_PX        (3U)
#define SCORE_DIGIT_H_PX        (5U)
#define SCORE_DIGIT_GAP_PX      (1U)
#define SCORE_X_PX              (2U)
#define SCORE_Y_PX              (1U)

/*------------------------------------------------------------------
 *  Difficulty — gap shrinks as the score grows
 *------------------------------------------------------------------*/
#define PIPE_GAP_SHRINK_STEP    (3U)    /* score points per shrink   */
#define PIPE_GAP_SHRINK_PX      (2U)    /* pixels removed per step   */
#define PIPE_GAP_MIN_LIMIT_PX   (12U)   /* never narrower than this  */

/*------------------------------------------------------------------
 *  Lives / hit handling
 *------------------------------------------------------------------*/
#define GAME_LIVES_NORMAL       (3U)
#define GAME_LIVES_EASY         (1U)
#define GAME_LIVES_MIXED        (3U)
#define GAME_INVULN_TICKS       (20U)   /* i-frames after a hit      */
#define LIVES_ICON_W_PX         (3U)
#define LIVES_ICON_H_PX         (3U)
#define LIVES_ICON_GAP_PX       (2U)
#define LIVES_X_RIGHT_PX        (OLED_WIDTH - 2U)   /* right-aligned */
#define LIVES_Y_PX              (2U)

/*------------------------------------------------------------------
 *  Input
 *  JUMP  — on-board blue user button PC13 (active low).
 *  RESET — external button on PC0, pull-up, active low.
 *          • during play / game-over : restart
 *          • on the READY screen     : toggle Normal/Easy mode
 *------------------------------------------------------------------*/
#define GAME_BTN_JUMP_PORT      GPIO_PORTC
#define GAME_BTN_JUMP_PIN       PIN13
#define GAME_BTN_RESET_PORT     GPIO_PORTC
#define GAME_BTN_RESET_PIN      PIN0
#define GAME_BTN_ACTIVE_LEVEL   (0U)

#endif /* FLAPPY_BIRD_CFG_H_ */
