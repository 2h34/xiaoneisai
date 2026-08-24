#include "motor.h"
#include "motor_dji.h"
#include "motor_zdrive.h"
#include <stddef.h>

/*建立 Motor 与具体 Driver 实例的绑定，并完成该实例为了接受后续通用 Motor 命令所必须的 backend-specific readiness 操作。*/ 
bool Motor_Init(Motor_t *motor,MotorType_t type,uint8_t id)
{
    if (motor == NULL)
    {
        return false;
    }
    if (type != MOTOR_TYPE_ZDRIVE && type != MOTOR_TYPE_DJI)
    {
        return false;
    }
    
    switch (type)
    {
        case MOTOR_TYPE_ZDRIVE:
            if (Motor_ZDrive_Bind(motor, id) == false)  /* ZDrive ID 范围检查 */
            {
                return false;
            }
            break;

        case MOTOR_TYPE_DJI:
            if (Motor_DJI_Bind(motor, id) == false)  /* DJI ID 范围检查，看取出来的是不是空指针 */
            {
                return false;
            }
            // 当前无需额外实例启动
            break;
    }
    motor->type = type;
    motor->id = id;

    return true;
}

/*初始化电机位置*/ 
bool Motor_SetPosition(Motor_t* motor, float position)
{
    if (motor == NULL)
    {
        return false;
    }
    if (motor->ops == NULL)
    {
        return false;
    }
    return motor->ops->set_position(motor, position);
    // switch (motor->type)
    // {
    // case MOTOR_TYPE_ZDRIVE:
    //     // 转成 ZDrive 的位置控制
    //     Zdrive_Set_target_mode(motor->id,Zdrive_Position,position);
    //     break;

    // case MOTOR_TYPE_DJI:
    //     // 转成 DJI 的位置控制
    //     {
    //         DJI_motor_t *dji = DJI_motor_GetById(motor->id); //根据id获取对应的电机实例
    //         if (dji == NULL)
    //         {
    //             return false;
    //         }
    //         DJI_motor_SetMode(dji,DJ_Position);
    //         DJI_motor_Set_Position(dji,position);
    //         break;
    //     }
    // }
    // return true;
}

/*设置电机速度*/
bool Motor_SetSpeed(Motor_t* motor, float speed)
{
    if (motor == NULL)
    {
        return false;
    }
    if (motor->ops == NULL)
    {
        return false;
    }
    return motor->ops->set_speed(motor, speed);
    // switch (motor->type)
    // {
    // case MOTOR_TYPE_ZDRIVE:
    //     // 转成 ZDrive 的速度控制
    //     {
    //     Zdrive_Set_target_mode(motor->id,Zdrive_Speed,speed);
    //     break;
    //     }
    // case MOTOR_TYPE_DJI:
    //     // 转成 DJI 的速度控制
    //     {
    //     DJI_motor_t *dji = DJI_motor_GetById(motor->id);
    //     if (dji == NULL)
    //     {
    //         return false;
    //     }
    //     DJI_motor_SetMode(dji,DJ_RPM);
    //     DJI_motor_Set_Speed(dji,speed);
    //     break;
    //     }
    // }
    // return true;
}

/*获取电机位置*/
float Motor_GetPosition(Motor_t* motor)
{
    if (motor == NULL)
    {
        return 0.0f;
    }
    if (motor->ops == NULL)
    {
        return 0.0f;
    }
    return motor->ops->get_position(motor);
    // switch (motor->type)
    // {
    // case MOTOR_TYPE_ZDRIVE:
    //     return Zdrive_Get_Position(motor->id);
    // case MOTOR_TYPE_DJI:
    //     {
    //     DJI_motor_t *dji = DJI_motor_GetById(motor->id);
    //     if (dji == NULL)
    //     {
    //         return 0.0f;
    //     }
    //     return dji->position;
    //     }
    // default:
    //     return 0.0f;
    // }
}

/*获取电机速度*/
float Motor_GetSpeed(Motor_t* motor)
{
    if (motor == NULL)
    {
        return 0.0f;
    }
    if (motor->ops == NULL)
    {
        return 0.0f;
    }
    return motor->ops->get_speed(motor);
    // switch (motor->type)
    // {
    // case MOTOR_TYPE_ZDRIVE:
    //     return Zdrive_GetSpeed(motor->id);
    // case MOTOR_TYPE_DJI:
    //     {
    //     DJI_motor_t *dji = DJI_motor_GetById(motor->id);
    //     if (dji == NULL)
    //     {
    //         return 0.0f;
    //     }
    //     return dji->rpm;
    //     }
    // default:
    //     return 0.0f;
    // }
}

/*取消该 Motor 当前的主动控制，使其不再维持 Position / Speed 目标，并请求底层停止主动驱动输出。*/
void Motor_Disable(Motor_t* motor)
{
    if (motor == NULL)
    {
        return;
    }
    if (motor->ops == NULL)
    {
        return;
    }
    motor->ops->disable(motor);
    // switch (motor->type)
    // {
    // case MOTOR_TYPE_ZDRIVE:
    //     Zdrive_Set_target_mode(motor->id,Zdrive_Disable,0.0f);
    //     break;
    // case MOTOR_TYPE_DJI:
    //     {
    //     DJI_motor_t *dji = DJI_motor_GetById(motor->id);
    //     if (dji == NULL)
    //         {
    //             return;
    //         }
    //     DJI_motor_SetMode(dji,DJ_Disable);
    //     break;
    //     }
    // }
}