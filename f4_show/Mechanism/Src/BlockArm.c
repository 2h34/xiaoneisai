#include "BlockArm.h"

#include <stdbool.h>
#include <stddef.h>

/*
 * 不同粗定位姿态附近，同一末端方向所需的双电机组合可能不同。
 * 该枚举只在 BlockArm 内部使用，不向 Task 暴露。
 */
typedef enum
{
    BLOCK_ARM_FINE_PROFILE_NONE,
    BLOCK_ARM_FINE_PROFILE_LOW_PICK,
    BLOCK_ARM_FINE_PROFILE_HIGH_PICK,
    BLOCK_ARM_FINE_PROFILE_PLACE_BOTTOM,
    BLOCK_ARM_FINE_PROFILE_PLACE_LEVEL1,
    BLOCK_ARM_FINE_PROFILE_PLACE_LEVEL2
} BlockArmFineAdjustProfile_t;

/* BlockArm 需要长期保存的最小内部数据。 */
typedef struct
{
    Motor_t *motor_a;
    Motor_t *motor_b;

    BlockArmState_t state;

    float motor_a_current_position;
    float motor_b_current_position;
    float motor_a_target_position;
    float motor_b_target_position;
    float motor_a_zero_position;
    float motor_b_zero_position;

    float tolerance;
    uint16_t reached_count;
    uint32_t homing_count;
    uint32_t motion_count;

    BlockArmFineAdjustProfile_t fine_adjust_profile;
    BlockArmFineAdjustDirection_t fine_adjust_direction;
    bool fine_adjust_active;
} BlockArm_t;

static BlockArm_t block_arm;

/* 更新两个电机的当前位置反馈。 */
static void BlockArm_UpdateFeedback(void)
{
    if (block_arm.motor_a != NULL)
    {
        block_arm.motor_a_current_position = Motor_GetPosition(block_arm.motor_a);
    }

    if (block_arm.motor_b != NULL)
    {
        block_arm.motor_b_current_position = Motor_GetPosition(block_arm.motor_b);
    }
}

/*
 * 记录自动动作对应的微调区域并进入 MOVING。
 * 真实目标角度、轨迹和 Motor_SetPosition() 调用由后续实现补充。
 */
static void BlockArm_StartAutoMove(BlockArmFineAdjustProfile_t profile)
{
    if (block_arm.state != BLOCK_ARM_READY &&
        block_arm.state != BLOCK_ARM_REACHED &&
        block_arm.state != BLOCK_ARM_STOPPED)
    {
        return;
    }

    block_arm.fine_adjust_profile = profile;
    block_arm.fine_adjust_active = false;
    block_arm.reached_count = 0U;
    block_arm.motion_count = 0U;
    block_arm.state = BLOCK_ARM_MOVING;

    /* TODO: 写入两个电机的真实目标，并发起经过机械验证的自动运动。 */
}

/* 判断自动动作是否已经稳定到位。 */
static bool BlockArm_IsReached(void)
{
    /*
     * TODO:
     * 1. 比较两个电机反馈与本次目标；
     * 2. 连续满足误差范围后返回 true；
     * 3. 不允许只根据一次采样判定到位。
     */
    return false;
}

/*
 * 根据“当前粗定位区域 + 微调方向”输出双电机低速组合。
 * 第一版可使用 switch-case 和实测参数，不需要在线复杂轨迹规划。
 */
static void BlockArm_ApplyFineAdjust(void)
{
    switch (block_arm.fine_adjust_profile)
    {
        case BLOCK_ARM_FINE_PROFILE_LOW_PICK:
            /* TODO: 实现低位取块区域的前、后、上、下双电机组合。 */
            break;

        case BLOCK_ARM_FINE_PROFILE_HIGH_PICK:
            /* TODO: 实现高位取块区域的前、后、上、下双电机组合。 */
            break;

        case BLOCK_ARM_FINE_PROFILE_PLACE_BOTTOM:
            /* TODO: 实现底层放块区域的前、后、上、下双电机组合。 */
            break;

        case BLOCK_ARM_FINE_PROFILE_PLACE_LEVEL1:
            /* TODO: 实现第一层放块区域的前、后、上、下双电机组合。 */
            break;

        case BLOCK_ARM_FINE_PROFILE_PLACE_LEVEL2:
            /* TODO: 实现第二层放块区域的前、后、上、下双电机组合。 */
            break;

        case BLOCK_ARM_FINE_PROFILE_NONE:
        default:
            break;
    }
}

/*
 * 记录当前位置为新的保持目标。
 * 真实位置保持命令由后续实现补充，不能简单 Motor_Disable()。
 */
static void BlockArm_StopAndHold(void)
{
    BlockArm_UpdateFeedback();
    block_arm.motor_a_target_position = block_arm.motor_a_current_position;
    block_arm.motor_b_target_position = block_arm.motor_b_current_position;

    /* TODO: 向两个电机发送当前位置保持命令。 */
}

void BlockArm_Init(Motor_t *motor_a, Motor_t *motor_b)
{
    block_arm.motor_a = motor_a;
    block_arm.motor_b = motor_b;

    block_arm.state = BLOCK_ARM_UNHOMED;

    block_arm.motor_a_current_position = 0.0f;
    block_arm.motor_b_current_position = 0.0f;
    block_arm.motor_a_target_position = 0.0f;
    block_arm.motor_b_target_position = 0.0f;
    block_arm.motor_a_zero_position = 0.0f;
    block_arm.motor_b_zero_position = 0.0f;

    block_arm.tolerance = 0.0f;
    block_arm.reached_count = 0U;
    block_arm.homing_count = 0U;
    block_arm.motion_count = 0U;

    block_arm.fine_adjust_profile = BLOCK_ARM_FINE_PROFILE_NONE;
    block_arm.fine_adjust_direction = BLOCK_ARM_FINE_FORWARD;
    block_arm.fine_adjust_active = false;
}

void BlockArm_Home(void)
{
    if (block_arm.state != BLOCK_ARM_UNHOMED &&
        block_arm.state != BLOCK_ARM_STOPPED)
    {
        return;
    }

    block_arm.homing_count = 0U;
    block_arm.fine_adjust_active = false;
    block_arm.fine_adjust_profile = BLOCK_ARM_FINE_PROFILE_NONE;
    block_arm.state = BLOCK_ARM_HOMING;

    /* TODO: 按真实限位方案启动两个电机的归零过程。 */
}

void BlockArm_Stop(void)
{
    if (block_arm.state == BLOCK_ARM_HOMING)
    {
        BlockArm_StopAndHold();
        block_arm.fine_adjust_active = false;
        block_arm.state = BLOCK_ARM_UNHOMED;
        return;
    }

    if (block_arm.state == BLOCK_ARM_MOVING)
    {
        BlockArm_StopAndHold();
        block_arm.fine_adjust_active = false;
        block_arm.state = BLOCK_ARM_STOPPED;
    }
}

void BlockArm_MoveToLowPickReady(void)
{
    BlockArm_StartAutoMove(BLOCK_ARM_FINE_PROFILE_LOW_PICK);
}

void BlockArm_MoveToHighPickReady(void)
{
    BlockArm_StartAutoMove(BLOCK_ARM_FINE_PROFILE_HIGH_PICK);
}

void BlockArm_MoveToPlaceBottomReady(void)
{
    BlockArm_StartAutoMove(BLOCK_ARM_FINE_PROFILE_PLACE_BOTTOM);
}

void BlockArm_MoveToPlaceLevel1Ready(void)
{
    BlockArm_StartAutoMove(BLOCK_ARM_FINE_PROFILE_PLACE_LEVEL1);
}

void BlockArm_MoveToPlaceLevel2Ready(void)
{
    BlockArm_StartAutoMove(BLOCK_ARM_FINE_PROFILE_PLACE_LEVEL2);
}

void BlockArm_MoveToSafe(void)
{
    /* Safe 不对应人工微调区域，完成后不允许直接开始四方向微调。 */
    BlockArm_StartAutoMove(BLOCK_ARM_FINE_PROFILE_NONE);

    /* TODO: 实现经过验证的取块/放块撤离路径与 Safe 目标。 */
}

void BlockArm_StartFineAdjust(BlockArmFineAdjustDirection_t direction)
{
    if (direction > BLOCK_ARM_FINE_DOWN)
    {
        return;
    }

    if (block_arm.fine_adjust_profile == BLOCK_ARM_FINE_PROFILE_NONE)
    {
        return;
    }

    if (block_arm.fine_adjust_active)
    {
        block_arm.fine_adjust_direction = direction;
        BlockArm_ApplyFineAdjust();
        return;
    }

    if (block_arm.state != BLOCK_ARM_REACHED &&
        block_arm.state != BLOCK_ARM_STOPPED)
    {
        return;
    }

    block_arm.fine_adjust_direction = direction;
    block_arm.fine_adjust_active = true;
    block_arm.state = BLOCK_ARM_MOVING;
    BlockArm_ApplyFineAdjust();
}

void BlockArm_StopFineAdjust(void)
{
    if (!block_arm.fine_adjust_active)
    {
        return;
    }

    BlockArm_StopAndHold();
    block_arm.fine_adjust_active = false;
    block_arm.state = BLOCK_ARM_STOPPED;
}

BlockArmState_t BlockArm_GetState(void)
{
    return block_arm.state;
}

void BlockArm_Process(void)
{
    BlockArm_UpdateFeedback();

    switch (block_arm.state)
    {
        case BLOCK_ARM_UNHOMED:
            break;

        case BLOCK_ARM_HOMING:
            block_arm.homing_count++;
            /* TODO: 检测真实归零信号，建立双电机零点后进入 READY。 */
            break;

        case BLOCK_ARM_READY:
            break;

        case BLOCK_ARM_MOVING:
            if (block_arm.fine_adjust_active)
            {
                BlockArm_ApplyFineAdjust();
            }
            else
            {
                block_arm.motion_count++;

                if (BlockArm_IsReached())
                {
                    block_arm.state = BLOCK_ARM_REACHED;
                }

                /* TODO: 增加真实动作超时、反馈掉线和机构异常判断。 */
            }
            break;

        case BLOCK_ARM_REACHED:
            break;

        case BLOCK_ARM_STOPPED:
            break;

        case BLOCK_ARM_FAULT:
            /* TODO: 根据真实机械和执行器方案维持安全状态。 */
            break;

        default:
            block_arm.state = BLOCK_ARM_FAULT;
            break;
    }
}
