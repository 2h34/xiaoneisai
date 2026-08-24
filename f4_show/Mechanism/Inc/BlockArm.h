#ifndef BLOCK_ARM_H
#define BLOCK_ARM_H

#include <stdint.h>
#include "motor.h"

/* BlockArm 对外运行状态。 */
typedef enum
{
    BLOCK_ARM_UNHOMED,
    BLOCK_ARM_HOMING,
    BLOCK_ARM_READY,
    BLOCK_ARM_MOVING, /*有两种情况：自动移动到粗定位姿态/人工微调*/
    BLOCK_ARM_REACHED,
    BLOCK_ARM_STOPPED,
    BLOCK_ARM_FAULT
} BlockArmState_t;

/* 人工微调时，操作手希望吸盘末端移动的方向。 */
typedef enum
{
    BLOCK_ARM_FINE_FORWARD,
    BLOCK_ARM_FINE_BACKWARD,
    BLOCK_ARM_FINE_UP,
    BLOCK_ARM_FINE_DOWN
} BlockArmFineAdjustDirection_t;

/* 绑定两个耦合电机并初始化软件状态；初始化后仍需执行 Home。 */
void BlockArm_Init(Motor_t *motor_a, Motor_t *motor_b);

/* 启动机械归零；真实限位检测和双电机归零顺序待后续实现。 */
void BlockArm_Home(void);

/* 中止当前 BlockArm 动作，并请求机构受控停止、保持。 */
void BlockArm_Stop(void);

/* 移动到单层大地块（或双层下方块）的取块粗定位姿态。 */
void BlockArm_MoveToLowPickReady(void);

/* 移动到双层上方大地块的取块粗定位姿态。 */
void BlockArm_MoveToHighPickReady(void);

/* 移动到底层放块粗定位姿态。 */
void BlockArm_MoveToPlaceBottomReady(void);

/* 移动到第一层放块粗定位姿态。 */
void BlockArm_MoveToPlaceLevel1Ready(void);

/* 移动到第二层放块粗定位姿态。 */
void BlockArm_MoveToPlaceLevel2Ready(void);

/* 启动到安全搬运姿态的复合运动。 */
void BlockArm_MoveToSafe(void);

/* 按指定末端方向启动低速人工微调。 */
void BlockArm_StartFineAdjust(BlockArmFineAdjustDirection_t direction);

/* 停止人工微调，并由后续实现保持当前机构姿态。 */
void BlockArm_StopFineAdjust(void);

/* 返回 BlockArm 当前运行状态。 */
BlockArmState_t BlockArm_GetState(void);

/* 周期推进 BlockArm 状态机；函数不得阻塞。 */
void BlockArm_Process(void);

#endif /* BLOCK_ARM_H */
