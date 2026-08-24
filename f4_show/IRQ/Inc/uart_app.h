#ifndef __UART_APP_H
#define __UART_APP_H

#include "stdint.h"

void uart_app_init(void);
void uart_app_process(uint32_t current_tick);



#endif // __UART_APP_H