#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

//定义枚举
typedef enum
{
    STATE_IDLE = 0,
    STATE_RUNNING,
    STATE_ALARM,
    STATE_COUNT
} state_t;

//声明状态机运行函数
void state_machine_run(void);

#endif