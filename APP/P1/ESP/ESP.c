#include "ESP.h"
uint8* ESP_Commands[]=
{   
    (uint8*)"AT\r\n",
    (uint8*)"AT+CWMODE?\r\n",
    (uint8*)"AT+CWMODE=1\r\n",
    (uint8*)"AT+CWMODE?\r\n",
    (uint8*)"AT+CWJAP=\"ZzZz\",\"J8702143  \"\r\n",
    (uint8*)"AT+CIFSR\r\n"
};
uint8 ESP_APPLICATION_FLAG=0;


uint32 my_strlen(uint8 *str)
{
    uint32 len = 0;

    while (str[len] != '\0')
    {
        len++;
    }

    return len;
}

void ESP_MainFunction(void)
{
    static ESP_Commands_Counter=0;
    uint8 Size_OF_String=0;
       if(ESP_Commands_Counter < sizeof(ESP_Commands)/sizeof(ESP_Commands[0]))
        {
            UART_SendSyncBuffer(UART1,ESP_Commands[ESP_Commands_Counter], my_strlen((uint8*)ESP_Commands[ESP_Commands_Counter]));
            ESP_Commands_Counter++;
        }
        if(ESP_Commands_Counter == 6)
        {
            ESP_APPLICATION_FLAG=1;

        }
}


