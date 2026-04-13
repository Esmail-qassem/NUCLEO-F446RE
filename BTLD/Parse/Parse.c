#include "Parse.h"
#include "STD_TYPES.h"
/*===========================================================================
                        GLOBAL VARIABLES
===========================================================================*/
uint8 UART_START = 0;
static uint8 flash_done = 0;
extern uint32 ms_ticks;
#define UART_TIMEOUT_MS 20000 // 20 seconds
uint32 ImageSize = 0;
uint32  MagicNumber = 0;
uint32 received_crc = 0;
uint32 crc32_table[256];
uint8 crc32_table_ready = 0;
/*===========================================================================
                        BIN MODE VARIABLES
===========================================================================*/
#define APP_START_ADDRESS 0x08008000UL
#define BIN_BUFFER_SIZE 40900
volatile uint8 bin_buffer[BIN_BUFFER_SIZE];
volatile uint32 bin_index = 0;
uint32 flash_address = APP_START_ADDRESS;
uint8 Start_Flashing = 0;
/*===========================================================================
                        CRC32 LOOKUP TABLE (zlib compatible)
===========================================================================*/
void CRC32_Init(void)
{
    for (uint32 i = 0; i < 256; i++)
    {
        uint32 c = i;
        for (uint8 j = 0; j < 8; j++)
        {
            if (c & 1)
                c = 0xEDB88320 ^ (c >> 1);
            else
                c >>= 1;
        }
        crc32_table[i] = c;
    }
    crc32_table_ready = 1;
}

uint32 CRC32_Calculate(const uint8 *data, uint32 length)
{

    uint32 crc = 0xFFFFFFFF;
    for (uint32 i = 0; i < length; i++)
    {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/*===========================================================================
                        BOOTLOADER ISR HANDLER
===========================================================================*/
static uint8 sync_received = 0;
static uint8 retry_count   = 0;   /* #14 — counts CRC/verify failures */

#define BTLD_NAK    0xFAU   /* sent to ESP on CRC failure → ESP retries  */
#define BTLD_ACK    0xACU   /* sent to ESP after successful verification  */
#define BTLD_FATAL  0xFBU   /* sent after MAX_RETRY failures → give up   */
#define MAX_RETRY   3U

void BootLoader_Handler(uint8 byte)
{
    /* Wait for sync byte 0x55 before accepting any data */
    if (sync_received == 0)
    {
        if (byte == 0x55)
        {
            sync_received = 1;
            retry_count   = 0;   /* new session — reset retry counter */
        }
        return;
    }

    /* Normal firmware receive */
    UART_START = 1;
    ms_ticks = 0;
    if (bin_index < BIN_BUFFER_SIZE)
    {bin_buffer[bin_index++] = byte;}
}

/*===========================================================================
                        BOOTLOADER MAIN FUNCTION
===========================================================================*/
void BootLoader_MainFunction(void)
{
    /*MAGIC NUMBER*/
    if(bin_index == 4u)
    {
        MagicNumber = ((uint32)bin_buffer[3] << 24) |
                       ((uint32)bin_buffer[2] << 16) |
                       ((uint32)bin_buffer[1] << 8) |
                       ((uint32)bin_buffer[0]);

    }
    /*CRC32*/
    if(bin_index == 8u)
    {
        received_crc = ((uint32)bin_buffer[7] << 24) |
                       ((uint32)bin_buffer[6] << 16) |
                       ((uint32)bin_buffer[5] << 8) |
                       ((uint32)bin_buffer[4]);
    }
    /*IMAGE SIZE*/
    if(bin_index == 12u)
    {
        ImageSize =  ((uint32)bin_buffer[11] << 24) |
                     ((uint32)bin_buffer[10] << 16) |
                     ((uint32)bin_buffer[9] << 8) |
                     ((uint32)bin_buffer[8]);
    }
    if(bin_index ==(ImageSize + 12) && Start_Flashing == 0)
    {
        /* Calculate CRC on firmware data in RAM */
        uint32 calculated_crc = CRC32_Calculate((uint8 *)bin_buffer + 12, ImageSize);

        if (calculated_crc == received_crc)
        {
            UART_SendSyncBuffer(UART2, (uint8 *)"\nCRC OK!\n", 9);
            FlashDrv_EraseSector(2);
            FlashDrv_EraseSector(3);
            Start_Flashing = 1;
        }
        else
        {
            /* #14 — CRC retry logic */
            retry_count++;
            UART_SendSyncBuffer(UART2, (uint8 *)"\nCRC FAILED! Attempt ", 21);
            UART_voidSendNumber(UART2, retry_count);
            UART_SendSyncBuffer(UART2, (uint8 *)"/3\n", 3);

            if (retry_count >= MAX_RETRY)
            {
                uint8 fatal = BTLD_FATAL;
                UART_SendSyncBuffer(UART1, &fatal, 1);
                UART_SendSyncBuffer(UART2, (uint8 *)"FATAL: max retries — aborting\n", 30);
                retry_count   = 0;
                sync_received = 0; /* go back to waiting for a new session */
            }
            else
            {
                uint8 nak = BTLD_NAK;
                UART_SendSyncBuffer(UART1, &nak, 1); /* tell ESP to resend */
            }
            ImageSize      = 0;
            bin_index      = 0;
            received_crc   = 0;
            Start_Flashing = 0;
            flash_done     = 0;
            flash_address = APP_START_ADDRESS;
        }

    }
    if(Start_Flashing == 1 && flash_done == 0)
    {
        __asm volatile("CPSID I");

        FlashDrv_ProgramBufferAligned(flash_address, bin_buffer + 12, ImageSize);

        __asm volatile("CPSIE I");
        flash_done = 1;
    }
    /*--- Verify after 2s silence ---*/
    if (flash_done == 1  && UART_START == 1 )
    {
        uint32 stack_ptr = *((volatile uint32 *)0x08008000);
        uint32 reset_handler = *((volatile uint32 *)0x08008004);

        if ((stack_ptr >= 0x20000000 && stack_ptr <= 0x20020000) &&
            (reset_handler >= 0x08008000 && reset_handler <= 0x08080000))
        {
            /* #14 — send ACK before reset so ESP knows flash succeeded */
            uint8 ack = BTLD_ACK;
            UART_SendSyncBuffer(UART1, &ack, 1);
            UART_SendSyncBuffer(UART2, (uint8 *)"\nVerification OK!\n", 18);
            retry_count   = 0;
            flash_address = APP_START_ADDRESS;
            sync_received = 0;
            received_crc = 0;
            Start_Flashing = 0;
            flash_done     = 0;
            bin_index      = 0;
            ImageSize      = 0;
            SCB_AIRCR = 0x5FA0004;
        }
        else
        {
            /* #14 — verification failure also counts as a retry */
            retry_count++;
            UART_SendSyncBuffer(UART2, (uint8 *)"\nVerification FAILED! Attempt ", 30);
            UART_voidSendNumber(UART2, retry_count);
            UART_SendSyncBuffer(UART2, (uint8 *)"/3\n", 3);

            if (retry_count >= MAX_RETRY)
            {
                uint8 fatal = BTLD_FATAL;
                UART_SendSyncBuffer(UART1, &fatal, 1);
                UART_SendSyncBuffer(UART2, (uint8 *)"FATAL: max retries — aborting\n", 30);
                retry_count = 0;
            }
            else
            {
                uint8 nak = BTLD_NAK;
                UART_SendSyncBuffer(UART1, &nak, 1);
            }

            received_crc   = 0;
            sync_received  = 0;
            Start_Flashing = 0;
            flash_done     = 0;
            bin_index      = 0;
            ImageSize      = 0;
            flash_address  = APP_START_ADDRESS;
        }
        UART_START = 0;
    }
    /*--- Timeout: no bytes for 10 seconds → reset ---*/
    if (ms_ticks > UART_TIMEOUT_MS)
    {
        SCB_AIRCR = 0x5FA0004;
    }
}