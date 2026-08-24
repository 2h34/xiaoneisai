#ifndef LIFT_H
#define LIFT_H

#include <stdbool.h>
#include <stdint.h>
#include "motor.h"


typedef enum
{
    LIFT_UNZEROED,
    LIFT_HOMING,
    LIFT_READY,
    LIFT_MOVING,
    LIFT_REACHED,
    LIFT_FAULT
} LiftState;
/*Lift状态划分*/

typedef struct
{
    float target_height_mm;   // 目标高度
    float actual_height_mm;   // 当前高度
    float tolerance_mm;       // 到位允许误差
    float zero_angle_deg;   // 归零时的电机角度
    LiftState state;
    uint16_t reached_count;     // 到位计数器
    uint32_t homing_count;      // 归零计数器

    Motor_t *motor;
} Lift_t;
/*Lift结构体，即mechanism需要保存的参数*/

/*对外接口*/
void Lift_Init(Motor_t *motor); /* 初始化 Lift 结构体，绑定电机 */
void Lift_SetHeight(float height_mm);  /* 设置目标高度 */
void Lift_Zero(void); /* 执行归零操作 */
void Lift_Process(void);  /* 处理Lift状态 */

void Lift_Update(void);

bool Lift_IsReached(void); /* 检查是否真正到位 */





bool Lift_HaveZeroed(void); /* 检查是否已经归零 */

#endif /* LIFT_H */