#include "myostasks.h"

#include "BlockArm.h"


#define BLOCK_ARM_PROCESS_PERIOD_MS  5U

uint8_t BeepAlarmTimes = 0;

void LedWaterTask(void *argument)
{
  for(;;)
  {
    Led_Water();
  }
}


void BeepAlarmTask(void *argument)
{
  for(;;)
  {
    uint8_t i;
    for(i = 0; i < BeepAlarmTimes; i++)
    {
        BEEP_ON();
        osDelay(40);
        BEEP_OFF();
        osDelay(40);
    }
    osDelay(1);
  }
}

/*
 * 单机械臂阶段的固定运行入口。
 * 电机驱动，任务和机构已在 main() 中初始化
 * 不自动调用 BlockArm_Home()，归零仍由上层命令在后续接入。
 */
void BlockArmServiceTask(void *argument)
{
  (void)argument;
  uint32_t next_wake = osKernelGetTickCount();

  int flag = 0;

  for (;;)
  {
    BlockArm_Process();
    // BlockVacuum_Process();
    // PickBlockTask_Process();
    // PlaceBlockTask_Process();
    if (flag == 1)
    {
      BlockArm_Home();
      flag = 0;
    }

    next_wake += BLOCK_ARM_PROCESS_PERIOD_MS;
    osDelayUntil(next_wake);
  }
}
