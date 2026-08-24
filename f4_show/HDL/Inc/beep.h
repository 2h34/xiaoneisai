#ifndef __BEEP_H
#define __BEEP_H


#include <stdint.h>



uint8_t beep_is_work(void);
void BEEP_Trigger(uint32_t current_tick,uint16_t count);
void BEEP_Process(uint32_t current_tick);

#endif // __BEEP_H