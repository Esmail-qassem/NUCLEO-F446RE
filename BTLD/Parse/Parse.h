#ifndef PARSE_H_
#define PARSE_H_

#include "STD_TYPES.h"
#include "UART.h"
#include "FLASH.h"
#define SCB_AIRCR *((volatile uint32*)0xE000ED0C)

#define MAX_LINE_LENGTH 64

uint8 lineBufferA[MAX_LINE_LENGTH];
uint8 lineBufferB[MAX_LINE_LENGTH];

uint8* processing_buffer=lineBufferA;
uint8* current_buffer=lineBufferB;

uint8 Receive_Uart=0;
uint8 line_ready =0;

static const uint8 asciiToHex[105] = {
    ['0']=0, ['1']=1, ['2']=2, ['3']=3, ['4']=4, ['5']=5, ['6']=6, ['7']=7, 
    ['8']=8, ['9']=9, ['A']=10, ['B']=11, ['C']=12, ['D']=13, ['E']=14, ['F']=15,
    ['a']=10, ['b']=11, ['c']=12, ['d']=13, ['e']=14, ['f']=15
};


uint8 parseByte(uint8 high, uint8 low);
uint8 processRecord(uint8 *recordBuffer);
void BootLoader_Handler(uint8 data);

void BootLoader_MainFunction(void);

#endif