#include "beep.h"
#include "main.h"

#define BEEP_DURATION_TICKS  50U

typedef enum
{
    BEEP_STATE_IDLE = 0,
    BEEP_STATE_ON,
    BEEP_STATE_OFF
} beep_state_t;

static beep_state_t beep_state = BEEP_STATE_IDLE;
static uint8_t beep_remain = 0;
static uint32_t beep_start_tick = 0;

void BEEP_ON(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
}

void BEEP_OFF(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
}



void BEEP_Trigger(uint32_t current_tick,uint16_t count)
{
    if (count == 0)
    {
        return;
    }
    BEEP_ON();
    beep_start_tick = current_tick;
    beep_remain = count;
    beep_state = BEEP_STATE_ON;
}

void BEEP_Process(uint32_t current_tick)
{
    if (beep_state == BEEP_STATE_IDLE)
    {
        return;
    }
    if (beep_state == BEEP_STATE_ON)
    {
        /* code */
        if (current_tick - beep_start_tick >= BEEP_DURATION_TICKS)
        {
            BEEP_OFF();
            beep_remain--;
            beep_state = BEEP_STATE_OFF;
            beep_start_tick = current_tick;
        }      
    }
    else if ( beep_state == BEEP_STATE_OFF) 
    {
        if (beep_remain > 0)
        {
            if (current_tick - beep_start_tick >= BEEP_DURATION_TICKS)
            {
                BEEP_ON();
                beep_state = BEEP_STATE_ON;
                beep_start_tick = current_tick;
            }  
        }
        else
        {
            beep_state = BEEP_STATE_IDLE;
        }
    }   
                   
}


uint8_t beep_is_work(void)
{
    return(beep_state != BEEP_STATE_IDLE);
}

