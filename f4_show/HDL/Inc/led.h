#ifndef __LED_H
#define __LED_H

#include "main.h"
#include "gpio.h"

#define LED_ON(x)  HAL_GPIO_WritePin(LED_##x##_GPIO_Port, LED_##x##_Pin, GPIO_PIN_SET)
#define LED_OFF(x) HAL_GPIO_WritePin(LED_##x##_GPIO_Port, LED_##x##_Pin, GPIO_PIN_RESET)
#define LED_TOGGLE(x) HAL_GPIO_TogglePin(LED_##x##_GPIO_Port, LED_##x##_Pin)

typedef enum
{
  STATE_OFF = 0,
  STATE_FLOW
} led_state_t;

void LED_FlowEnter(uint32_t current_tick);
void LED_FlowProcess(uint32_t current_tick);
void LED_Setmode(led_state_t new_state,uint32_t current_tick);
void LED_BreathEnter(uint32_t current_tick);
void LED_BreathProcess(uint32_t current_tick);
void LED_Process(uint32_t current_tick);
void LED_OFF_mode(void);

#endif // __LED_H