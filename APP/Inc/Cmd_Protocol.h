#ifndef CMD_PROTOCOL_H_
#define CMD_PROTOCOL_H_

/*==================================================================
 *  Cmd_Protocol.h
 *  Single-byte command identifiers received over UART1 / UART2.
 *================================================================*/

typedef enum
{
    CMD_LED_ON      = 0x01U,
    CMD_LED_OFF     = 0x02U,
    CMD_BTLD_JUMP   = 0x03U,
    CMD_BTLD_UPDATE = 0x04U,    /* wired link only */
    CMD_RUN_TIME    = 0x05U,
    CMD_RESET       = 0x06U,
    CMD_PWM_SET     = 0x07U,    /* followed by one duty byte 0..100 */
    CMD_GET_VERSION = 0xA1U
} Cmd_Id_t;

#endif /* CMD_PROTOCOL_H_ */
