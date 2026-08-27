#ifndef __MYOSTASKS_H
#define __MYOSTASKS_H

#include "cmsis_os2.h"
#include "main.h"
#include "Led.h"
#include "Beep.h"

extern uint8_t BeepAlarmTimes;

/* 单机械臂的机构服务任务，周期推进状态机。 */
void BlockArmServiceTask(void *argument);

#endif // __MYOSTASKS_H
