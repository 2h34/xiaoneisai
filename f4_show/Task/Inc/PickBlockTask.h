#ifndef PICK_BLOCK_TASK_H
#define PICK_BLOCK_TASK_H

/* 拾取大地块 Task 的流程状态。 */
typedef enum
{
    PICK_BLOCK_TASK_IDLE,

    PICK_BLOCK_TASK_MOVE_TO_PICK_PREPARE,
    PICK_BLOCK_TASK_MANUAL_ALIGN,
    PICK_BLOCK_TASK_GRAB,
    PICK_BLOCK_TASK_MOVE_TO_SAFE,

    PICK_BLOCK_TASK_DONE,
    PICK_BLOCK_TASK_FAULT
} PickBlockTaskState_t;

/* 初始化拾取 Task 自身状态，不重复初始化 Mechanism。 */
void PickBlockTask_Init(void);

/* 启动单层大地块（或双层下方块）的拾取流程。 */
void PickBlockTask_StartLowPick(void);

/* 启动双层上方大地块的拾取流程。 */
void PickBlockTask_StartHighPick(void);

/* 操作手确认侧面对准完成，允许 Task 开始建立真空。 */
void PickBlockTask_ConfirmGrab(void);

/* 返回拾取 Task 当前状态。 */
PickBlockTaskState_t PickBlockTask_GetState(void);

/* 周期推进拾取 Task 状态机；函数不得阻塞。 */
void PickBlockTask_Process(void);

#endif /* PICK_BLOCK_TASK_H */
