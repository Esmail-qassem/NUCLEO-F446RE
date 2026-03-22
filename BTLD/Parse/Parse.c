#include "Parse.h"

/*===========================================================================
                        GLOBAL VARIABLES
===========================================================================*/
uint8 UART_START = 0;
static uint8 flush_done  = 0;
static uint8 verify_done = 0;
extern uint32 ms_ticks;
#define UART_TIMEOUT_MS  10000  // 10 seconds
static uint32 counter=0;
uint32 Header=0;
uint8 Binary_receivingfinished=0;
uint32 received_crc=0;
uint32 crc32_table[256];
uint8  crc32_table_ready = 0;

/*===========================================================================
                        HEX MODE VARIABLES
===========================================================================*/
#ifdef HEX_MODE
volatile uint8 lineBufferA[MAX_LINE_LENGTH];
volatile uint8 lineBufferB[MAX_LINE_LENGTH];

volatile uint8 *filling_buffer    = lineBufferA;
volatile uint8 *processing_buffer = lineBufferB;

volatile uint8           current_index    = 0;
volatile uint8           active_buffer    = 0;
volatile Buffrer_state_t buffer_state[2]  = {EMPTY, EMPTY};
#endif

/*===========================================================================
                        BIN MODE VARIABLES
===========================================================================*/
#ifdef BIN_MODE

#define APP_START_ADDRESS  0x08008000UL
#define BIN_BUFFER_SIZE    4

volatile uint8  bin_buffer[BIN_BUFFER_SIZE];
volatile uint32 bin_index    = 0;
uint32          flash_address = APP_START_ADDRESS;

#endif

/*===========================================================================
                        HEX MODE FUNCTIONS
===========================================================================*/
#ifdef HEX_MODE
static void swap_pointer(void)
{
    uint8 *temp       = processing_buffer;
    processing_buffer = filling_buffer;
    filling_buffer    = temp;
}

uint8 processRecord(uint8 *recordBuffer)
{
    uint8  number_of_data = parseByte(recordBuffer[1], recordBuffer[2]);
    uint8  address_high   = parseByte(recordBuffer[3], recordBuffer[4]);
    uint8  address_low    = parseByte(recordBuffer[5], recordBuffer[6]);
    uint16 address        = (address_high << 8) | address_low;
    uint8  record_type    = parseByte(recordBuffer[7], recordBuffer[8]);

    if (record_type == 0x00) /* Data record */
    {
        for (uint8 i = 0; i < number_of_data; i += 4)
        {
            uint8 d0 = parseByte(recordBuffer[9  + i*2], recordBuffer[10 + i*2]);
            uint8 d1 = parseByte(recordBuffer[11 + i*2], recordBuffer[12 + i*2]);
            uint8 d2 = parseByte(recordBuffer[13 + i*2], recordBuffer[14 + i*2]);
            uint8 d3 = parseByte(recordBuffer[15 + i*2], recordBuffer[16 + i*2]);

            uint32 word = (d3 << 24) | (d2 << 16) | (d1 << 8) | d0;
            FlashDrv_ProgramWord((0x08000000 + address), word);
            address += 4;
        }
        return 0x00;
    }
    else if (record_type == 0x01) /* End of File record */
    {
        UART_SendSyncBuffer(UART2, "\nEOF record received - firmware upload complete\n", 48);
        SCB_AIRCR = 0x5FA0004;
        return 0xFF;
    }

    return 0x00; /* Ignore other record types */
}
#endif

/*===========================================================================
                        PARSE HELPER
===========================================================================*/
uint8 parseByte(uint8 high, uint8 low)
{
    return ((asciiToHex[high] << 4) | asciiToHex[low]);
}
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
    // if (!crc32_table_ready) CRC32_Init();

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
void BootLoader_Handler(uint8 byte)
{
    UART_START  = 1;
    flush_done  = 0;
    verify_done = 0;
    ms_ticks=0;

    #ifdef HEX_MODE
    if (byte == '\n' || byte == '\r')
    {
        if (current_index > 0)
        {
            filling_buffer[current_index] = '\0';
            buffer_state[active_buffer]   = READY;

            active_buffer = (active_buffer == 0) ? 1 : 0;
            swap_pointer();
            current_index = 0;
        }
    }
    else
    {
        if (current_index < MAX_LINE_LENGTH - 1)
        {
            buffer_state[active_buffer]     = FILLING;
            filling_buffer[current_index++] = byte;
        }
        else
        {
            UART_SendSyncBuffer(UART2, "Line too long!\n", 15);
            current_index = 0;
        }
    }
    #endif

    #ifdef BIN_MODE
    if (bin_index < BIN_BUFFER_SIZE)
    {
        bin_buffer[bin_index++] = byte;
    }
    #endif
}

/*===========================================================================
                        BOOTLOADER MAIN FUNCTION
===========================================================================*/
void BootLoader_MainFunction(void)
{
    /*--- Timeout: no bytes for 10 seconds → reset ---*/
    if (ms_ticks > UART_TIMEOUT_MS)
    {
        SCB_AIRCR = 0x5FA0004;
    }

    #ifdef BIN_MODE
    /*--- Write full buffer to flash ---*/
    if (bin_index >= BIN_BUFFER_SIZE)
    {
        uint8  local_buf[BIN_BUFFER_SIZE];
        uint32 local_idx;

        /* Critical section — copy buffer and reset index */
        __asm volatile ("CPSID I");
        for (local_idx = 0; local_idx < BIN_BUFFER_SIZE; local_idx++)
        {
            local_buf[local_idx] = bin_buffer[local_idx];
        }
        bin_index = 0;
        __asm volatile ("CPSIE I");
        if (counter == 1)
        {
        received_crc = ((uint32)local_buf[3] << 24) |
                     ((uint32)local_buf[2] << 16) |
                     ((uint32)local_buf[1] << 8)  |
                     ((uint32)local_buf[0]);
        }
        else if (counter == 2)
        {
            Header = ((uint32)local_buf[3] << 24) |
                     ((uint32)local_buf[2] << 16) |
                     ((uint32)local_buf[1] << 8)  |
                     ((uint32)local_buf[0]);

        }
        else if(counter > 2)
        {
        /* Write to flash outside critical section */
        FlashDrv_ProgramBufferAligned(flash_address, local_buf, BIN_BUFFER_SIZE);
        flash_address += BIN_BUFFER_SIZE;
        }
        counter++;

    }
        /*--- Flush remaining bytes after 1s silence ---*/
if(counter == (Header/BIN_BUFFER_SIZE)+3)
{
    /* Flush remaining bytes FIRST */
    if (bin_index > 0)
    {
        uint8  local_buf[BIN_BUFFER_SIZE];
        uint32 local_idx;

        __asm volatile ("CPSID I");
        for (local_idx = 0; local_idx < bin_index; local_idx++)
            local_buf[local_idx] = bin_buffer[local_idx];
        local_idx = bin_index;
        bin_index = 0;
        __asm volatile ("CPSIE I");

        FlashDrv_ProgramBufferAligned(flash_address, local_buf, local_idx);
        flash_address += local_idx;
    }
    flush_done = 1;

    /* NOW calculate CRC — all bytes are in flash */
    uint32 calculated_crc = CRC32_Calculate((uint8*)APP_START_ADDRESS, Header);

    if (calculated_crc == received_crc)
    {
        UART_SendSyncBuffer(UART2, "\nCRC OK!\n", 9);
        Binary_receivingfinished = 1;
    }
    else
    {
        UART_SendSyncBuffer(UART2, "\nCRC FAILED!\n", 13);
        counter       = 0;
        Header        = 0;
        received_crc  = 0;
        flash_address = APP_START_ADDRESS;
    }
}



    #endif

    /*--- Verify after 2s silence ---*/
    if ((ms_ticks > 2000 || Binary_receivingfinished == 1) && UART_START == 1 && verify_done == 0)
    {
        uint32 stack_ptr     = *((volatile uint32 *)0x08008000);
        uint32 reset_handler = *((volatile uint32 *)0x08008004);

        if ((stack_ptr     >= 0x20000000 && stack_ptr     <= 0x20020000) &&
            (reset_handler >= 0x08008000 && reset_handler <= 0x08080000))
        {
            UART_SendSyncBuffer(UART2, "\nVerification OK!\n", 18);
            flash_address = APP_START_ADDRESS;
            Binary_receivingfinished=0;
            counter=0;
            received_crc = 0;
            SCB_AIRCR = 0x5FA0004;
        }
        else
        {
            UART_SendSyncBuffer(UART2, "\nVerification FAILED!\n", 21);
            counter=0;
            received_crc = 0;
            Binary_receivingfinished=0;
            flash_address = APP_START_ADDRESS;  /* reset for retry */
        }
        verify_done = 1;
        UART_START=0;
    }
        #ifdef HEX_MODE
    /*--- Process HEX line buffers ---*/
    for (uint8 i = 0; i < 2; i++)
    {
        if (buffer_state[i] == READY)
        {
            uint8 *buf = (i == 0) ? lineBufferA : lineBufferB;
            processRecord(buf);
            buffer_state[i] = EMPTY;
        }
    }
    #endif

}