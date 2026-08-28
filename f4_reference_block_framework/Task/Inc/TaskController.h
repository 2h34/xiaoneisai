#ifndef TASKCONTROLLER_H
#define TASKCONTROLLER_H

#include "main.h"
#include "PickBlockTask.h"
#include "PlaceBlockTask.h"
#include "BlockArm.h"
#include "BlockVacuum.h"

typedef enum{
    IDLE = 0,
    PICK_READY,
    GRAB,
    PLACE_READY,
    PLACE,
} task_flag;

#endif
