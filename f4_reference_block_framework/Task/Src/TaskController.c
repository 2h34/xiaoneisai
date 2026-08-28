#include "TaskController.h"

void Control_tasks(task_flag flag)
{
    BlockArm_Process();
    BlockVacuum_Process();
    PickBlockTask_Process();
    PlaceBlockTask_Process();

    switch (flag){

        case IDLE:{

        }

        case PICK_READY:{
            PickBlockTask_StartLowPick();
            break;
        }

        case GRAB:{
            PickBlockTask_ConfirmGrab();
            break;
        }

        case PLACE_READY:{
            PlaceBlockTask_StartBottom();
            break;
        }

        case PLACE:{
            PlaceBlockTask_ConfirmRelease();
            break;
        }

        case RESET:{
            break;
        }
    }
}
