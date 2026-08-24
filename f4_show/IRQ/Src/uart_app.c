#include "stdint.h"
#include "usart.h"
#include "beep.h"
#include "tim.h"


static uint8_t rx_buffer[64];
static volatile uint16_t rx_size;
static volatile uint8_t rx_ready;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        rx_size = Size;
        rx_ready = 1;

        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, sizeof(rx_buffer));
    }
}

void uart_app_process(uint32_t current_tick)
{
    if (rx_ready)
    {
        rx_ready = 0;

        // 处理 rx_buffer[0 ~ rx_size-1]
        // 统计 1 的个数
        // 控制蜂鸣器
        uint16_t count = 0;
        for (uint16_t i = 0; i < rx_size; i++)
        {
            if (rx_buffer[i] == '1')
            {
                count++;
            }
        }
    
        BEEP_Trigger(current_tick, count);
        
        
    }
}

void uart_app_init(void)
{
    rx_size = 0;
    rx_ready = 0;

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, sizeof(rx_buffer));
}