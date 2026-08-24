#ifndef Motor_H
#define Motor_H

#include <stdint.h>
#include <stdbool.h>



typedef struct Motor Motor_t;

typedef enum
{
    MOTOR_TYPE_ZDRIVE,
    MOTOR_TYPE_DJI
} MotorType_t;

/*ops表结构体，定义了各种电机操作的函数指针*/
typedef struct
{
    bool  (*set_position)(Motor_t *motor, float position);
    bool  (*set_speed)(Motor_t *motor, float speed);

    float (*get_position)(Motor_t *motor);
    float (*get_speed)(Motor_t *motor);

    void  (*disable)(Motor_t *motor);

    
} MotorOps_t;

struct Motor
{
    MotorType_t type;
    uint8_t id;

    const MotorOps_t *ops;
} ;


/*规定：速度：单位为 RPM，为输出轴速度。位置：单位为度*/


float Motor_GetPosition(Motor_t* motor);
float Motor_GetSpeed(Motor_t* motor);
bool Motor_SetPosition(Motor_t* motor, float position);
bool Motor_SetSpeed(Motor_t* motor, float speed);

bool Motor_Init(Motor_t *motor,MotorType_t type,uint8_t id);
/*建立 Motor 与具体 Driver 实例的绑定，并完成该实例为了接受后续通用 Motor 命令所必须的 backend-specific readiness 操作。*/

void Motor_Disable(Motor_t* motor);  
/*取消该 Motor 当前的主动控制，使其不再维持 Position / Speed 目标，并请求底层停止主动驱动输出。*/

#endif // Motor_H
