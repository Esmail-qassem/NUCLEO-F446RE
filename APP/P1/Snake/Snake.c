
#include "Snake.h"
#include "SysTick_interface.h"
uint8 volatile move =0u;
snake_t Snake;
static uint8 Game_Status= E_Ok;
static uint8 forbidden_direction =7u;
static uint16 eat=1;

Snake_Dimention_t food;

uint32 Random(void)
{
    return SysTick_GetRemaningTime();
}

void static GameReset()
{
    Game_Status =E_Ok;
    move= 0u;
    forbidden_direction= 7;
    eat=1;
    Snake_Init();
}
void static SnakeDraw()
{
    for(uint16 counter=Snake.Trail; counter < (Snake.Trail + Snake.Size);counter++)
    {
        OLED_DrawPixel(Snake.Snake_Array[counter].x, Snake.Snake_Array[counter].y, OLED_COLOR_WHITE);
    }
    OLED_DrawPixel(food.x, food.y, OLED_COLOR_WHITE);
}

void static Snake_Self_Eat_Avoid()
{
    for(uint16  i = 1 ; i <= Snake.Size;i++)
    {
        if((Snake.Snake_Array[Snake.Head].x == Snake.Snake_Array[(Snake.Head + SNAKE_LENGTH -i) % SNAKE_LENGTH].x) &&
           (Snake.Snake_Array[Snake.Head].y == Snake.Snake_Array[(Snake.Head + SNAKE_LENGTH -i) % SNAKE_LENGTH].y))
        {
            OLED_Clear();
            OLED_DrawString(30, 0, "GAME OVER");
            Game_Status = E_Not_Ok;
        }
    }
}


Status_t static Snake_Movement (void)
{
    Status_t  Snake_mov_status=E_Not_Ok;
    Snake_Dimention_t tempp;

    if(Game_Status ==E_Ok)
    {
        if(forbidden_direction != move)
        {
            switch(move)
            {
                case MOVE_UP:
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    // move the head  head ++  but with taking into consideration the size of the array or snake length
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].y = tempp.y-1;
                    Snake.Snake_Array[Snake.Head].x = tempp.x;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    forbidden_direction = MOVE_DOWN;
                    Snake_mov_status = E_Ok;
                    break;
                }
                case MOVE_RIGHT:
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].x = tempp.x+1;
                    Snake.Snake_Array[Snake.Head].y = tempp.y;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    forbidden_direction = MOVE_LEFT;
                    Snake_mov_status = E_Ok;
                    break;
                }
                case MOVE_DOWN:
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].y = tempp.y+1;
                    Snake.Snake_Array[Snake.Head].x = tempp.x;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    forbidden_direction = MOVE_UP;
                    Snake_mov_status = E_Ok;
                    break;
                }          
                case MOVE_LEFT:
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].x = tempp.x-1;
                    Snake.Snake_Array[Snake.Head].y = tempp.y;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    forbidden_direction = MOVE_RIGHT;
                    Snake_mov_status = E_Ok;
                    break;
                }               
            }
        }
        else
        {
            switch (move)
            {
                case MOVE_UP :    
                    {   
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].y = tempp.y+1;
                    Snake.Snake_Array[Snake.Head].x = tempp.x;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    Snake_mov_status = E_Ok;
                        break;
                    }
                case MOVE_RIGHT :
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].x = tempp.x-1;
                    Snake.Snake_Array[Snake.Head].y = tempp.y;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    Snake_mov_status = E_Ok;
                    break;  
                } 
                case MOVE_DOWN :
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].y = tempp.y-1;
                    Snake.Snake_Array[Snake.Head].x = tempp.x;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    Snake_mov_status = E_Ok;
                    break; 
                }   
                case MOVE_LEFT :
                {
                    tempp = Snake.Snake_Array[Snake.Head] ;
                    Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                    Snake.Snake_Array[Snake.Head].x = tempp.x+1;
                    Snake.Snake_Array[Snake.Head].y = tempp.y;
                    Snake.Trail = (Snake.Trail + 1) % SNAKE_LENGTH;
                    Snake_mov_status = E_Ok;
                    break; 
                }   
            }
        }
    }

return Snake_mov_status;
}

    void static Check_Boundries(void)
    {
            if(Snake.Snake_Array[Snake.Head].y == 255)
            {
                Snake.Snake_Array[Snake.Head].y=63;
            }
            else if (Snake.Snake_Array[Snake.Head].y == 64)
            {
                Snake.Snake_Array[Snake.Head].y=0;
            }
            else if (Snake.Snake_Array[Snake.Head].x == 128)
            {
                Snake.Snake_Array[Snake.Head].x=0;
            }
            else if (Snake.Snake_Array[Snake.Head].x == 255)
            {
                Snake.Snake_Array[Snake.Head].x=127;
            }
    }










void Snake_Init(void)
{
    Snake.Size = 1;
    Snake.Trail = 0;
    Snake .Head = Snake.Size - 1u;
    for(uint16 temp=Snake.Trail; temp < Snake.Size; temp++)
    {
        Snake.Snake_Array[temp].x=temp;
        Snake.Snake_Array[temp].y=32;
    }
}
void Snake_Game(void)
{
    if(move == GAME_RESET)
    {
        GameReset();
    }
    OLED_Clear();
    if( Game_Status ==E_Ok)
    {   
        if(E_Not_Ok == Snake_Movement())
        {
           OLED_DrawString(0, 30, "Please Move the snake"); 
        }
        else
        {
            Check_Boundries();
            SnakeDraw();
            Snake_Self_Eat_Avoid();
            if(eat == 1)
            {
                food.y = Random() % 64;
                food.x = Random() % 128;
                eat = 0;
            }
            if(Snake.Snake_Array[Snake.Head].x == food.x && Snake.Snake_Array[Snake.Head].y == food.y)
            {   
                Snake_Dimention_t tempp = Snake.Snake_Array[Snake.Head] ;
                Snake.Head = (Snake.Head + 1) % SNAKE_LENGTH;
                switch(move)
                {
                    case MOVE_UP:
                    {
                        Snake.Snake_Array[Snake.Head].x = tempp.x;
                        Snake.Snake_Array[Snake.Head].y = tempp.y--;                        
                        break;
                    }                     
                    case MOVE_DOWN:
                    {
                        Snake.Snake_Array[Snake.Head].x = tempp.x;
                        Snake.Snake_Array[Snake.Head].y = tempp.y++;
                        break;
                    }                   
                    case MOVE_RIGHT:
                    {
                        Snake.Snake_Array[Snake.Head].x = tempp.x++;
                        Snake.Snake_Array[Snake.Head].y = tempp.y;
                        break;
                    }                   
                    case MOVE_LEFT:
                    {
                        Snake.Snake_Array[Snake.Head].x = tempp.x--;
                        Snake.Snake_Array[Snake.Head].y = tempp.y;
                        break;
                    }                   
                }

                Snake.Size +=1 ;
                eat = 1;
            }

        }
    }
    else
    {
       OLED_DrawString(30, 0, "GAME OVER"); 
       OLED_DrawString(0, 8, "Press r to restart"); 
    }
    OLED_UpdateScreen(I2C1_PORT,0,8);
}











