#include "ESP.h"

uint8* ESP_Commands[] =
{
    (uint8*)"AT\r\n",
    (uint8*)"AT+CWMODE?\r\n",
    (uint8*)"AT+CWMODE=1\r\n",
    //(uint8*)"AT+CWJAP=\"ESMAIL\",\"123456789\"\r\n",
    (uint8*)"AT+CIFSR\r\n",
    (uint8*)"AT+CIPSTART=\"TCP\",\"192.168.137.1\",8080\r\n",
    (uint8*)"AT+CIPSEND=33\r\n",
    (uint8*)"GET /application.bin HTTP/1.1\r\n\r\n"

};

uint8 ESP_APPLICATION_FLAG = 0;
void int_to_str(int num, char* buffer)
{
    int i = 0;
    int tmp = num;

    // Count digits
    do {
        i++;
        tmp /= 10;
    } while(tmp > 0);

    buffer[i] = '\0';

    while(num > 0) {
        buffer[--i] = (num % 10) + '0';
        num /= 10;
    }
}

/***********************************/
uint32 my_strlen(uint8 *str)
{
    uint32 len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}
/***********************************
 * Main ESP State Machine
 ***********************************/
void ESP_MainFunction(void)
{
    static uint8 ESP_Commands_Counter = 0;

    // Send the startup AT commands
    if (ESP_Commands_Counter < sizeof(ESP_Commands)/sizeof(ESP_Commands[0]))
    {
        UART_SendSyncBuffer(UART1,
                            ESP_Commands[ESP_Commands_Counter],
                            my_strlen(ESP_Commands[ESP_Commands_Counter]));

        ESP_Commands_Counter++;
    }

    // When we finish AT+CIPSTART (index 5)
    if (ESP_Commands_Counter == sizeof(ESP_Commands)+1)
    {
        ESP_APPLICATION_FLAG = 1;

        ESP_Commands_Counter++; // avoid repeating
    }
}
