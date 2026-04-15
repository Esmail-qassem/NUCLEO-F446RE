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

#endif /* FLAPPY_BIRD_CFG_H_ */
