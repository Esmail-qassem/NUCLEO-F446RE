#include "Parse.h"

volatile uint8 lineBufferA[MAX_LINE_LENGTH];
volatile uint8 lineBufferB[MAX_LINE_LENGTH];

volatile uint8* filling_buffer=lineBufferA;
volatile uint8* processing_buffer=lineBufferB;

volatile uint8 current_index = 0;
volatile uint8 active_buffer = 0; // 0 -> A, 1 -> B
volatile Buffrer_state_t buffer_state[2] = {EMPTY, EMPTY};
void swap_pointer(void)
{
    uint8* temp=processing_buffer;
    processing_buffer=filling_buffer;
    filling_buffer=temp;
}

uint8 processRecord(uint8 *recordBuffer)
{
    uint8 number_of_data = parseByte(recordBuffer[1], recordBuffer[2]);
    uint8 address_high  = parseByte(recordBuffer[3], recordBuffer[4]);
    uint8 address_low   = parseByte(recordBuffer[5], recordBuffer[6]);
    uint16 address = (address_high << 8) | address_low;
    uint8 record_type   = parseByte(recordBuffer[7], recordBuffer[8]);
    
    if(record_type == 0x00) // Data record
    {
        for (uint8 i = 0; i < number_of_data; i += 4)
        {
            uint8 d0 = parseByte(recordBuffer[9 + i*2],   recordBuffer[10 + i*2]);
            uint8 d1 = parseByte(recordBuffer[11 + i*2],  recordBuffer[12 + i*2]);
            uint8 d2 = parseByte(recordBuffer[13 + i*2],  recordBuffer[14 + i*2]);
            uint8 d3 = parseByte(recordBuffer[15 + i*2],  recordBuffer[16 + i*2]);
        
            uint32 word = (d3 << 24) | (d2 << 16) | (d1 << 8) | d0;
          FlashDrv_ProgramWord((0x08000000 +address) , word);
            address += 4;
        }
        return 0x00; // Data processed successfully
    }
    else if(record_type == 0x01) // End of File record
    {
        UART_SendString(UART2, "\nEOF record received - firmware upload complete\n");
        // ADD EOL ACTIONS HERE:
        SCB_AIRCR = 0x5FA0004; /* generate soft reset */
        return 0xFF; // End of load
    }  
    else
    {
        return 0x00; // Ignore other record types but continue
    }
}


uint8 parseByte(uint8 high, uint8 low)
{
    return((asciiToHex[high]<<4)|asciiToHex[low]);
}


void BootLoader_Handler(uint8 byte)
{
    if (byte == '\n' || byte == '\r') // End of line (accept CR or LF)
    {
        if (current_index > 0) // only if we actually received data
        {
            filling_buffer[current_index] = '\0';   // null terminate
            buffer_state[active_buffer] = READY;

            if (active_buffer == 0)
            {    
                active_buffer = 1;
                swap_pointer();
            }
            else
            {
                active_buffer = 0;
                swap_pointer();
            }
        current_index = 0; // reset for next line   
        }

    }
    else
    {
        if (current_index < MAX_LINE_LENGTH - 1)
        {
            buffer_state[active_buffer] = FILLING;
            filling_buffer[current_index++] = byte;
        }
        else
        {
            // Overflow protection
            UART_SendString(UART2, "Line too long!\n");
            current_index = 0;
        }
    }
}

void BootLoader_MainFunction(void)
{
  // Check both buffers
    for (uint8 i = 0; i < 2; i++)
    {
        if (buffer_state[i] == READY)
        {
            // Select correct buffer to process
            uint8* buf = (i == 0) ? lineBufferA : lineBufferB;
            processRecord(buf);
            buffer_state[i] = EMPTY; // Done
        }
    }
}

