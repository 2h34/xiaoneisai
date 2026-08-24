#include <stdio.h>
#include "state_machine.h"
#include "led.h"

//定义状态函数指针类型
typedef void (*state_func)(void);

//定义当前状态变量
static state_t current_state = STATE_IDLE;

//声明状态函数
static void state_idle(void);
static void state_running(void);
static void state_alarm(void);

//定义状态函数表
static const state_func state_table[STATE_COUNT] =
{
    [STATE_IDLE] = state_idle,
    [STATE_RUNNING] = state_running,
    [STATE_ALARM] = state_alarm
};

//定义运行函数
void state_machine_run(void)
{
    if ((current_state < STATE_COUNT) && (state_table[current_state] != NULL))
    {
        state_table[current_state]();
    }
}

//定义状态函数

//静灭
static void state_idle(void)
{
    LED_OFF(1);
    LED_OFF(2);

    HAL_Delay(1000);

    current_state = STATE_RUNNING;
}

//流水灯
static void state_running(void)
{

    uint8_t led_pins[2] =
    {
        LED_1_Pin,
        LED_2_Pin
    };

    for (uint8_t i = 0; i < 2; i++)
    {
        LED_ON(1);
        HAL_Delay(250);
        LED_OFF(1);
        HAL_Delay(250);
        LED_ON(2);
        HAL_Delay(250);
        LED_OFF(2);
        HAL_Delay(250);
    }
    HAL_Delay(1000);
    current_state = STATE_ALARM;
}

//报警
static void state_alarm(void)
{
    uint8_t led_pins[2] =
    {
        LED_1_Pin,
        LED_2_Pin
    };

    for (uint8_t i = 0; i < 2; i++)
    {
        LED_ON(1);
        HAL_Delay(100);
        LED_OFF(1);
        HAL_Delay(100);
        LED_ON(2);
        HAL_Delay(100);
        LED_OFF(2);
        HAL_Delay(100);
    }
    HAL_Delay(1000);
    current_state = STATE_IDLE;
}

