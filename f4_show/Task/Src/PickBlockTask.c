#include "PickBlockTask.h"

#include <stdbool.h>
#include "BlockArm.h"
#include "BlockVacuum.h"

static PickBlockTaskState_t pick_block_task_state;

/* 判断当前 BlockArm 状态是否允许发起新的自动动作。 */
static bool PickBlockTask_ArmCanStart(void)
{
    BlockArmState_t arm_state = BlockArm_GetState();

    return arm_state == BLOCK_ARM_READY ||
           arm_state == BLOCK_ARM_REACHED ||
           arm_state == BLOCK_ARM_STOPPED;
}

/* 判断当前 Task 生命周期是否允许重新启动。 */
static bool PickBlockTask_CanStart(void)
{
    return pick_block_task_state == PICK_BLOCK_TASK_IDLE ||
           pick_block_task_state == PICK_BLOCK_TASK_DONE;
}

void PickBlockTask_Init(void)
{
    pick_block_task_state = PICK_BLOCK_TASK_IDLE;
}

void PickBlockTask_StartLowPick(void)
{
    if (!PickBlockTask_CanStart() ||
        !PickBlockTask_ArmCanStart() ||
        BlockVacuum_GetState() != BLOCK_VACUUM_RELEASED)
    {
        return;
    }

    BlockArm_MoveToLowPickReady();
    pick_block_task_state = PICK_BLOCK_TASK_MOVE_TO_PICK_PREPARE;
}

void PickBlockTask_StartHighPick(void)
{
    if (!PickBlockTask_CanStart() ||
        !PickBlockTask_ArmCanStart() ||
        BlockVacuum_GetState() != BLOCK_VACUUM_RELEASED)
    {
        return;
    }

    BlockArm_MoveToHighPickReady();
    pick_block_task_state = PICK_BLOCK_TASK_MOVE_TO_PICK_PREPARE;
}

void PickBlockTask_ConfirmGrab(void)
{
    if (pick_block_task_state != PICK_BLOCK_TASK_MANUAL_ALIGN)
    {
        return;
    }

    BlockArm_StopFineAdjust();
    BlockVacuum_Grab();
    pick_block_task_state = PICK_BLOCK_TASK_GRAB;
}

PickBlockTaskState_t PickBlockTask_GetState(void)
{
    return pick_block_task_state;
}

void PickBlockTask_Process(void)
{
    BlockArmState_t arm_state = BlockArm_GetState();
    BlockVacuumState_t vacuum_state = BlockVacuum_GetState();

    if (pick_block_task_state != PICK_BLOCK_TASK_IDLE &&
        pick_block_task_state != PICK_BLOCK_TASK_DONE &&
        pick_block_task_state != PICK_BLOCK_TASK_FAULT)
    {
        if (arm_state == BLOCK_ARM_FAULT ||
            vacuum_state == BLOCK_VACUUM_FAULT)
        {
            pick_block_task_state = PICK_BLOCK_TASK_FAULT;
            return;
        }
    }

    switch (pick_block_task_state)
    {
        case PICK_BLOCK_TASK_IDLE:
            break;

        case PICK_BLOCK_TASK_MOVE_TO_PICK_PREPARE:
            if (arm_state == BLOCK_ARM_REACHED)
            {
                pick_block_task_state = PICK_BLOCK_TASK_MANUAL_ALIGN;
            }
            break;

        case PICK_BLOCK_TASK_MANUAL_ALIGN:
            /* 等待操作手微调并调用 PickBlockTask_ConfirmGrab()。 */
            break;

        case PICK_BLOCK_TASK_GRAB:
            if (vacuum_state == BLOCK_VACUUM_GRABBED)
            {
                BlockArm_MoveToSafe();
                pick_block_task_state = PICK_BLOCK_TASK_MOVE_TO_SAFE;
            }
            break;

        case PICK_BLOCK_TASK_MOVE_TO_SAFE:
            if (arm_state == BLOCK_ARM_REACHED)
            {
                pick_block_task_state = PICK_BLOCK_TASK_DONE;
            }
            break;

        case PICK_BLOCK_TASK_DONE:
            break;

        case PICK_BLOCK_TASK_FAULT:
            break;

        default:
            pick_block_task_state = PICK_BLOCK_TASK_FAULT;
            break;
    }
}
