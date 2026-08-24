#include "vofa_app.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
#include <math.h>



void vofa_app_process(void)
{
    float value = 1.0f;
    static float x = 0.0f;
    value = sinf(x);

    x += 0.1f;

    if (x >= 6.2831853f)
    {
        x -= 6.2831853f;
    }

    uint8_t tx_buffer[8];
    uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7F};

    memcpy(&tx_buffer[0], &value, sizeof(float));
    memcpy(&tx_buffer[4], tail, 4);

    HAL_UART_Transmit(&huart1, tx_buffer, sizeof(tx_buffer), 10);
}