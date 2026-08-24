#ifndef CAN_APP_H
#define CAN_APP_H

#include "stdint.h"


void can_app_init(void);
void can_app_process(uint32_t current_tick);

#endif // CAN_APP_H