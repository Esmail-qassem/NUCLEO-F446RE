// #include "Parse.h"


// void swap_pointer(void)
// {
//     uint8* tmep=processing_buffer;
//     processing_buffer=current_buffer;
//     current_buffer=tmep;
// }

// uint8 processRecord(uint8 *recordBuffer)
// {
//     int CRC;
//     uint16 sum = 0;   // local sum
//     uint8 number_of_data = parseByte(recordBuffer[1], recordBuffer[2]);
//     uint8 address_high  = parseByte(recordBuffer[3], recordBuffer[4]);
//     uint8 address_low   = parseByte(recordBuffer[5], recordBuffer[6]);
//     uint16 address = (address_high << 8) | address_low;
//     uint8 record_type   = parseByte(recordBuffer[7], recordBuffer[8]);

//     sum += number_of_data + address_high + address_low + record_type;
//     if(record_type == 0x00) // Data record
//     {
//         for(uint8 i = 0; i < number_of_data; i += 2)
//             {
//                 uint8 data_low  = parseByte(recordBuffer[9 + i*2],   recordBuffer[10 + i*2]);
//                 uint8 data_high  = parseByte(recordBuffer[11 + i*2],  recordBuffer[12 + i*2]);
//                 uint16 halfword = (data_high << 8) | data_low;
//                 sum += data_high + data_low;
            
//                 FPEC_voidFlashWrite(0x08000000 + address, &halfword, 1);
//                 address += 2;

//             }
//     }
//     else if(record_type == 0x01)
//     {
//       	UART_SendString(UART2,"\nsoftware updated \n");
//         return 0xFF;
//     }  
//     // Checksum
//     uint8 crc = parseByte(recordBuffer[9 + number_of_data*2], recordBuffer[10 + number_of_data*2]);
//     sum += crc;
//     CRC = sum & 0xFF;   // only keep lowest 8 bits
//     return CRC; // must be 0 if valid
// }

// uint8 parseByte(uint8 high, uint8 low)
// {
//     return((asciiToHex[high]<<4)|asciiToHex[low]);
// }


// void BootLoader_Handler(uint8 byte)
// {
//     static uint16 index = 0;

//     if (byte == '\n' || byte == '\r') // End of line (accept CR or LF)
//     {
//         if (index > 0) // only if we actually received data
//         {
//             current_buffer[index] = '\0';   // null terminate
//             line_ready=1;
//             swap_pointer();
//         }
//         index = 0; // reset for next line
//     }
//     else
//     {
//         if (index < MAX_LINE_LENGTH - 1)
//         {
//             current_buffer[index++] = byte;
//         }
//         else
//         {
//             // Overflow protection
//             UART_SendString(UART2, "Line too long!\n");
//             index = 0;
//         }
//     }
// }
// void BootLoader_MainFunction(void)
// {

//   Receive_Uart= UART_ReceiveByte(UART2);
//   BootLoader_Handler(Receive_Uart);
//   if(line_ready==1)
//   {
//     processRecord(processing_buffer);
//     line_ready=0;
//   }

// }