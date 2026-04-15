#ifndef APP_CTRL_CFG_H_
#define APP_CTRL_CFG_H_

/*==================================================================
 *  App_Ctrl_Cfg.h
 *  Tunables for the application control module.
 *================================================================*/

#define PWM_DUTY_MAX_PCT        (100U)

/* Busy-wait before issuing SYSRESETREQ so the UART "Resetting..."
 * message has time to leave the shift register.                  */
#define SYS_RESET_DELAY_LOOPS   (100000UL)

#endif /* APP_CTRL_CFG_H_ */
