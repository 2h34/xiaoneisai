#include "led.h"
#include "tim.h"


typedef enum
{
  BREATH_UP = 0,
  BREATH_DOWN
} breath_state_t;




static uint8_t flow_led_index = 0;
static uint32_t flow_last_switch_tick = 0;
static uint32_t breath_last_tick = 0;
static uint16_t breath_value = 0;
static breath_state_t breath_state = BREATH_UP;
static led_state_t current_state = STATE_OFF;


void LED_FlowEnter(uint32_t current_tick)
{
    // 停止呼吸灯
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
    // LED1亮
    LED_ON(1);
    // LED2灭
    LED_OFF(2);
    // 记录当前亮的是LED1
    flow_led_index = 0;
    // 记录本轮流水灯开始计时的tick
    flow_last_switch_tick = current_tick;
}

void LED_FlowProcess(uint32_t current_tick)
{
    if (current_tick - flow_last_switch_tick < 150U)
    {
        return;
    }
    // 每300ms切换一次LED
    flow_last_switch_tick = current_tick;
    // 切换LED状态
    if (flow_led_index == 0)
        {
            LED_OFF(1);
            LED_ON(2);
            flow_led_index = 1;
        }
    else
        {
            LED_OFF(2);
            LED_ON(1);
            flow_led_index = 0;
        }
    
}

void LED_Setmode(led_state_t new_state,uint32_t current_tick)
{
    if (new_state == STATE_FLOW)
    {
        current_state = new_state;
        LED_FlowEnter(current_tick);
    }
    if (new_state == STATE_OFF)
    {
        current_state = new_state;
        LED_OFF(1);
        LED_OFF(2);
    }
}

void LED_OFF_mode(void)
{
    LED_OFF(1);
    LED_OFF(2);
}

void LED_Process(uint32_t current_tick)
{
    if (current_state == STATE_FLOW )
    {
        LED_FlowProcess(current_tick);
    }
    if (current_state == STATE_OFF)
    {
        LED_OFF_mode();
    }
}





void LED_BreathEnter(uint32_t current_tick)
{
    LED_OFF(1);
    LED_OFF(2);
    breath_value = 0;
    breath_state = BREATH_UP;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, breath_value);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 999U - breath_value);
    breath_last_tick = current_tick;
}

void LED_BreathProcess(uint32_t current_tick)
{
    if (current_tick == breath_last_tick )
    {
        return;
    }

    breath_last_tick = current_tick;

    if (breath_state == BREATH_UP)
    {
        breath_value++;
        if (breath_value >= 999U)
        {
            breath_value = 999U;
            breath_state = BREATH_DOWN;
        }
    }
    else
    {
        breath_value--;
        if (breath_value == 0U)
        {
            breath_value = 0U;
            breath_state = BREATH_UP;
        }
    }
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, breath_value);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 999U - breath_value);
}

