/**
 * @file    ZDrive.c
 * @brief   ZDrive J60/Z-Smart motor driver, ported from R2 chassis.
 */
// #include "includes.h"
#include "ZDrive.h"
#include "string.h"
#include "math.h"
// #include "FD_Canqueue.h"
static void Zdrive_SendDirect(uint32_t id,uint8_t dlc,const uint8_t *data);

// #if USE_ZMDR

Zdrive Zmotor[USE_ZDRIVE_NUM];

// bool CanFullFlag = false; // 发送队列满标志,由 Zdrive_Enqueue() 置位

// // 电机落在哪一路 CAN:SPLIT_COUNT = 0 时全部走第一路,否则前 SPLIT_COUNT 个走第一路
// static inline bool is_Zdrive_OnFirstBus(uint32_t motor_index)
// {
//     return (MOTOR_ZDRIVE_SPLIT_COUNT == 0U) ||
//            (motor_index < MOTOR_ZDRIVE_SPLIT_COUNT);
// }

// // 判断 ID(1-based)的电机是否挂在 bus(0=FDCAN1,1=FDCAN2,2=FDCAN3)上
// static inline bool Zdrive_IdOnBus(uint32_t id, uint8_t bus)
// {
//     uint8_t cfg_bus = is_Zdrive_OnFirstBus(id - 1U) ? (uint8_t)MOTOR_ZDRIVE_CAN_BUS_1
//                                                     : (uint8_t)MOTOR_ZDRIVE_CAN_BUS_2;
//     return cfg_bus == bus;
// }
// 获取电机的发送队列,根据电机 ID(0-based)判断挂在哪一路 CAN 上
// static FDCAN_SendQueueType *Zdrive_GetTxQueue(uint32_t motor_index)
// {
//     uint8_t bus = is_Zdrive_OnFirstBus(motor_index) ? (uint8_t)MOTOR_ZDRIVE_CAN_BUS_1
//                                                     : (uint8_t)MOTOR_ZDRIVE_CAN_BUS_2;

//     if (bus == 0U)
//     {
//         return &CAN1_Txqueue;
//     }
//     if (bus == 2U)
//     {
//         return &CAN3_Txqueue;
//     }
//     return &CAN2_Txqueue;
// }

// 拆分是否生效(第二路总线上有电机)
// static inline bool Zdrive_SplitActive(void)
// {
//     return (MOTOR_ZDRIVE_SPLIT_COUNT > 0U) &&
//            (MOTOR_ZDRIVE_SPLIT_COUNT < MOTOR_ZDRIVE_COUNT) &&
//            (MOTOR_ZDRIVE_CAN_BUS_2 != MOTOR_ZDRIVE_CAN_BUS_1);
// }

static const uint8_t s_empty_data[1] = {0};//空数组，用于ask发送data

// 入队:按帧 ID 解析所属总线队列(ID 1..8 各自解析,0xFU 广播两路都发);
// 满队列置 Can2FullFlag 并丢弃,行为与直接写入一致
// static void Zdrive_Enqueue(uint32_t id, uint8_t dlc, const uint8_t *data)
// {
//      FDCAN_RxHeaderTypeDef header;
//      FDCAN_SendQueueType *queues[2];
//     uint8_t queue_cnt;

//      header.Identifier = id;
//     header.IdType = FDCAN_STANDARD_ID;
//      header.DataLength = dlc;

//      if ((id & 0xFU) == 0xFU)
//    {
//       queues[0] = Zdrive_GetTxQueue(0);
//      queue_cnt = 1U;
//          if (Zdrive_SplitActive())
//       {
// //             queues[1] = Zdrive_GetTxQueue(MOTOR_ZDRIVE_SPLIT_COUNT);
// //             queue_cnt = 2U;
// //         }
// //     }
// //     else
// //     {
//         queues[0] = Zdrive_GetTxQueue((id & 0xFU) - 1U);
//         queue_cnt = 1U;
//     }

//     for (uint8_t k = 0U; k < queue_cnt; k++)
//     {
//         if (CAN_Queue_IfFull(queues[k]))
//         {
//             CanFullFlag = true;
//             continue;
//         }
//         CAN_Enqueue(queues[k], header, (uint8_t *)data);
//         }
//     }
// }
void ZdriveInit(void)
{
    for (uint32_t i = 0; i < USE_ZDRIVE_NUM; i++)
    {
        Zmotor[i].param.GearRatio = 1.0f;
        Zmotor[i].param.ReductionRatio = 1.0f;
        Zmotor[i].valSetPre.pos_deg = 1e5f;
        Zmotor[i].valSetNow.speed_rpm = 0.0f;
        Zmotor[i].valSetNow.pos_deg = 0.0f;
        Zmotor[i].valSetNow.current_A = 0.0f;
        Zmotor[i].statusFlag.Arriveflag = false;
        Zmotor[i].argum.GapCnt = 0;
        Zmotor[i].valReal.pos_deg = 0.0f;
        Zmotor[i].param.kpPos = 2.3f;
        Zmotor[i].param.kiPos = 0.0f;
        Zmotor[i].param.kpVel = 10.0f;
        Zmotor[i].param.kiVel = 0.0f;
        Zmotor[i].mode = Zdrive_Disable; /* 初始 setmode 为 disable,上电即失能 */
        Zmotor[i].modeRead = Zdrive_Disable;
        Zmotor[i].statusFlag.err = Zdrive_Well; /* 上电无错误 */
        Zmotor[i].Begin = false;                /* 初始化完成后由任务层置 true */
        Zmotor[i].statusFlag.ZeroPoint = 0.0f;
        Zmotor[i].statusFlag.Zeroflag = false;
        Zmotor[i].target.isPending = false;
        Zmotor[i].target.pendingTarget = 0.0f;
    }
}

/*根据模式把上层目标转换为对应参数（位置/速度），打包到data，得到zdrive命令，*/
void ZdriveSet(float data, uint8_t id, uint8_t set_code)
{
    if (id == 0U)
    {
        id = 0xFU; /* broadcast address */
    }
    else
    {
        if (id > USE_ZDRIVE_NUM)
        {
            return;
        }
        if ((set_code == PosIn) || (set_code == Pur))
        {
            /* 这里做角度->转数*/
            data = DEG2N(data) * Zmotor[id - 1U].param.ReductionRatio;/*宏定义*/
        }
        else if (set_code == VelIn)
        {       /* 将rpm->rps */
            data /= (60.0f / Zmotor[id - 1U].param.ReductionRatio);
        }
    } /*普通位置模式不用归一化处理，直接发送浮点数*/

    Zdrive_SendDirect(id | ((uint32_t)set_code << 4U), 4U, (const uint8_t *)&data);
}    /*直接用can发送，发送给zdrive目标模式和数据*/

   

//接收zdrive反馈，记得改f4格式，：void ZdriveReceive(CAN_RxHeaderTypeDef *Rxheader,uint8_t *Rx_Data)
void ZdriveReceive(CAN_RxHeaderTypeDef *rxheader, uint8_t *rx_Data)
{
    if (rxheader->IDE != CAN_ID_STD)//过滤非标准帧，f4：rxheader->IDE != CAN_ID_STD
    {
        return;
    }
    if (rxheader->RTR != CAN_RTR_DATA)   /*过滤远程帧*/
    {
        return;
    }

    uint32_t control_id = (uint32_t)(rxheader->StdId & 0x0FU);//提取node ID
    uint32_t operation_id = rxheader->StdId >> 4U;//提取CMD ID
    uint32_t tmp_pos = 0;
    uint16_t tmp_vel = 0;
    uint16_t tmp_cur = 0;

    /* ZDrive frames are standard IDs below 0x400.  VESC status frames on the
       same bus use standard IDs >= 0x0900 and must not be parsed here. */

    
    if ((control_id < 1) || (control_id > USE_ZDRIVE_NUM))
    {
        return;
    }

    /* 该 ID 按配置不在这条总线上(如同总线的 DJI 反馈 0x201..0x204)→ 丢弃 */
    /* if (!Zdrive_IdOnBus(control_id, bus))
    // {
    //     return;
    // }*/
    uint32_t motor_index = control_id - 1U;
    //index为数组编号

    if (rxheader->DLC == 4U)
    //宏定义dlc=4
    {
        switch (operation_id)
        {
            //位置
        case Pur:
            Zmotor[motor_index].valPre.pos_deg = Zmotor[motor_index].valReal.pos_deg;
            memcpy(&Zmotor[motor_index].valReal.pos_deg, rx_Data, sizeof(float));
            Zmotor[motor_index].valReal.pos_deg = N2DEG(Zmotor[motor_index].valReal.pos_deg) /
                                                  Zmotor[motor_index].param.ReductionRatio;
            break;//N2DEG()宏定义没有定义

        case Cur_M://电流
            memcpy(&Zmotor[motor_index].valReal.current_A, rx_Data, sizeof(float));
            Zmotor[motor_index].valReal.torque_Nm = Zmotor[motor_index].valReal.current_A;
            break;

        case Vel://速度
            memcpy(&Zmotor[motor_index].valReal.speed_rpm, rx_Data, sizeof(float));
            Zmotor[motor_index].valReal.speed_rpm *= (60.0f / Zmotor[motor_index].param.ReductionRatio);
            break;

        case Mode: //当前模式,通常用于askmode后的同步(moderead = mode)
        {
            float temp_mode = 0.0f;
            memcpy(&temp_mode, rx_Data, sizeof(float));
            Zmotor[motor_index].modeRead = (ZdriveMode)(int32_t)temp_mode;
            break;
        }

        case Err://错误
        {
            float temp_err = 0.0f;
            memcpy(&temp_err, rx_Data, sizeof(float));
            Zmotor[motor_index].statusFlag.err = (ZdriveErr)(int32_t)temp_err;
            break;
        }

        case PosIn:
        {
            float temp_pos_in = 0.0f;
            memcpy(&temp_pos_in, rx_Data, sizeof(float));
            Zmotor[motor_index].valReal.posIn_deg = N2DEG(temp_pos_in) /
                                                    Zmotor[motor_index].param.ReductionRatio;
            break;
        }

        case Vel_Limit:
            memcpy(&Zmotor[motor_index].valReal.velLimit_rpm, rx_Data, sizeof(float));
            break;

        case Acc_Acu:
            memcpy(&Zmotor[motor_index].valReal.accAcu_rps2, rx_Data, sizeof(float));
            break;

        case Acc_Dec:
            memcpy(&Zmotor[motor_index].valReal.accDec_rps2, rx_Data, sizeof(float));
            break;

        default:
            break;
        }
    }
    else if (rxheader->DLC == 8U)/*宏定义dlc=8，解析心跳帧*/
    {
        memcpy(&tmp_pos, rx_Data, sizeof(float));

        Zmotor[motor_index].valPre.pos_deg = Zmotor[motor_index].valReal.pos_deg;
        Zmotor[motor_index].valReal.pos_deg = ((tmp_pos) / (float)0xffffffffU *
                                                   (POU - POD) +
                                               POD) /
                                              Zmotor[motor_index].param.ReductionRatio;

        memcpy(&tmp_vel, rx_Data + 4U, sizeof(int16_t));
        Zmotor[motor_index].valReal.speed_rpm = (float)tmp_vel / (float)0xffffU *
                                                    (2.0f * Velocity_Limit) -
                                                Velocity_Limit;

        memcpy(&tmp_cur, rx_Data + 6U, sizeof(int16_t));
        Zmotor[motor_index].valReal.current_A = (float)tmp_cur / (float)0xffffU *
                                                    (2.0f * Current_Limit) -
                                                Current_Limit;
    }
}

//向某个zdrive查询某个参数
void ZdriveAsk(uint8_t id, uint8_t ask_code)
{
    if (id == 0U)
    {
        id = 0xFU;
    }

    Zdrive_SendDirect(id | ((uint32_t)ask_code << 4U), 0U, s_empty_data);
    //直接用can发送
}

//同时发送一个速度目标和一个位置目标，并打包到data
void ZdriveSetPVT(float speed, float angle, uint8_t id)
{
    uint8_t data[8] = {0};

    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return;
    }
    //速度归一化
    uint32_t vel_mod_u32 = (uint32_t)((speed + Velocity_Limit) /
                                     (2.0f * Velocity_Limit) * (float)0xffffffffU);
    //位置归一化                                 
    uint32_t pos_mod_u32 = (uint32_t)(((angle - POD) / (POU - POD)) *
                                      (float)0xffffffffU);

    memcpy(data, &vel_mod_u32, sizeof(uint32_t));
    memcpy(data + 4U, &pos_mod_u32, sizeof(uint32_t));
    //分别打包进data[]的前后4个字节

    Zdrive_SendDirect(id | ((uint32_t)PVT_Frame << 4U), 8U, data);
    //直接用can发送
}

//打包pid，代码实现：PID 参数使用归一化 uint32，并发送 DLC=8
//官方协议：普通参数写入使用4字节数据
void ZdriveSetPID(float value, uint8_t id, uint8_t pid_code)
{
    uint8_t data[8] = {0};
    uint32_t val_u32;

    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return;
    }

    val_u32 = (uint32_t)(value / 100.0f * (float)0xffffffffU);

    memcpy(data, &val_u32, sizeof(uint32_t));

    Zdrive_SendDirect(id | ((uint32_t)pid_code << 4U), 8U, data);
}

void ZdriveSetPosVelLimit(float vel_limit, uint8_t id)
{
    uint8_t data[8] = {0};

    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return;
    }

    memcpy(data, &vel_limit, sizeof(float));

    Zdrive_SendDirect(id | ((uint32_t)Vel_Limit << 4U), 4U, data);

    ZdriveAsk(id, Vel_Limit);
}

void ZdriveSetAccel(float ace, uint8_t id)
{
    uint8_t data[8] = {0};

    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return;
    }

    memcpy(data, &ace, sizeof(float));

    Zdrive_SendDirect(id | ((uint32_t)Acc_Acu << 4U), 4U, data);
    Zdrive_SendDirect(id | ((uint32_t)Acc_Dec << 4U), 4U, data);

    ZdriveAsk(id, Acc_Acu);
    ZdriveAsk(id, Acc_Dec);
}

void ZdriveSetVelLimit(float vel, uint8_t id)
{
    if (id == 0U)
    {
        id = 0xFU; /* broadcast address */
    }
    else
    {
        if (id > USE_ZDRIVE_NUM)
        {
            return;
        }
        /* rpm → rps 换算,和 ZdriveSet 的 VelIn 一致(输出端速度) */
        vel /= (60.0f / Zmotor[id - 1U].param.ReductionRatio);
    }

    Zdrive_SendDirect(id | ((uint32_t)Vel_Limit << 4U), 4U, (const uint8_t *)&vel);
    ZdriveAsk(id, Vel_Limit);
}

void ZdriveParamConfig(uint8_t id, ZdriveParam param)
{
    ZdriveParam *p;

    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return;
    }

    p = &Zmotor[id - 1U].param;

    /* 哪一项 PID 有改动就下发哪一项 */
    if (param.kpPos != p->kpPos)
    {
        ZdriveSetPID(param.kpPos, id, PID_POS_P);
    }
    if (param.kiPos != p->kiPos)
    {
        ZdriveSetPID(param.kiPos, id, PID_POS_I);
    }
    if (param.kpVel != p->kpVel)
    {
        ZdriveSetPID(param.kpVel, id, PID_VEL_P);
    }
    if (param.kiVel != p->kiVel)
    {
        ZdriveSetPID(param.kiVel, id, PID_VEL_I);
    }

    *p = param;
}

/* ---- Switch 状态机:判断条件 modeRead != mode ----
   驱动尚未确认目标模式,持续下发 Mode 命令并查询,直至 read == set 进入运行。
   各模式可在切换期间预处理设定值(如 Speed 清零、Position 对齐当前 posIn),
   确认前不执行周期指令。 */
/* 可以放小巧思 */
static void Zdrive_SwitchMachine(Zdrive *motor, uint8_t id) 
{
    switch (motor->mode)  /*用于切换模式，但只是发出指令，实际moderead=mode在ask后接收zdrive解析信息时进行*/
    {
    case Zdrive_Disable:
    case Zdrive_Current:
        break;
    case Zdrive_Speed:
        motor->valSetNow.speed_rpm = 0.f;
        motor->valSetPre.speed_rpm = 0.f;
        break;
    case Zdrive_Position:
        motor->valSetNow.pos_deg = motor->valReal.pos_deg;
        motor->valSetPre.pos_deg = motor->valReal.pos_deg;
        break;

    default:
        break;
    }
    ZdriveSet((float)motor->mode, id, Mode);
    ZdriveAsk(id, Mode);
}

/* ---- 运行状态机:判断条件 modeRead == mode,按模式执行周期指令 ---- */
static void Zdrive_RunMachine(Zdrive *motor, uint8_t id)
{
    switch (motor->mode)
    {
    case Zdrive_Speed:
        if (fabs(motor->valSetNow.speed_rpm - motor->valSetPre.speed_rpm) > 0.1f)
        {                                             /*** 检查速度是否发生变化，如果变化超过0.1，则更新速度 ***/
            ZdriveSet(motor->valSetNow.speed_rpm, id, VelIn);
            motor->valSetPre.speed_rpm = motor->valSetNow.speed_rpm;
        }
        break;

    case Zdrive_Current:
        // ZDrive MIT 帧格式,未适配
        // ZdriveSet(motor->valSetNow.current_A, id, CurIn);
        break;

    case Zdrive_Position:
        if (!motor->pvtparam.PVTflag)
        {
            if (fabs(motor->valSetNow.pos_deg - motor->valSetPre.pos_deg) > 0.01f) 
            {                         /** 检查位置是否发生变化，如果变化超过0.01，则更新位置 ***/
                motor->valSetPre.pos_deg = motor->valSetNow.pos_deg;
                ZdriveSet(motor->valSetNow.pos_deg, id, PosIn);
            }
            else if (motor->valSetNow.pos_deg == 0.0f &&
                     fabs(motor->valReal.posIn_deg) > 0.5f)  /* 如果目标位置为0且当前位置偏离0超过0.5度，则更新位置 ***/
            {
                ZdriveSet(motor->valSetNow.pos_deg, id, PosIn);
            }
        }
        else
        {
            ZdriveSetPVT(motor->valSetNow.speed_rpm, motor->valSetNow.pos_deg, id);
        }
        break;


    case Zdrive_Disable:
    default:
        /* Disable 由驱动自身保持,无周期指令 */
        break;
    }
}

// 错误处理:err != Well 时在此处理,内容暂空白(错误码定义见 ZdriveErr)
static void Zdrive_ErrHandle(Zdrive *motor)
{
    if (motor->statusFlag.err == Zdrive_Well)
    {
        return;
    }

    /* TODO: 按错误码处理,如强制 mode = Zdrive_Disable 等*/
}

void ZdriveFunc(void)
{
    uint32_t i;

    for (i = 0; i < USE_ZDRIVE_NUM; i++)
    {
        /*Begin == false:初始化未完成,跳过该电机*/
        if (!Zmotor[i].Begin)
        {
            continue;
        }

        /* 错误处理 */
        Zdrive_ErrHandle(&Zmotor[i]);
        // /* 目标模式与驱动确认的当前模式一致,且有待处理的目标值,则下发目标值 */
        if (Zmotor[i].modeRead == Zmotor[i].mode && Zmotor[i].target.isPending)
        {                                             /*这里是为了避免切换状态时刷新适当的目标值，用pendingTarget寄存参数*/
            /* 根据当前模式设置相应的目标值 */
            if (Zmotor[i].mode == Zdrive_Position)
            {
                Zmotor[i].valSetNow.pos_deg = Zmotor[i].target.pendingTarget;
            }
            else if (Zmotor[i].mode == Zdrive_Speed)
            {
                Zmotor[i].valSetNow.speed_rpm = Zmotor[i].target.pendingTarget;
            }
            Zmotor[i].target.isPending = false;
        }
        

        /* Switch 状态机 */
        if (Zmotor[i].modeRead != Zmotor[i].mode)
        {
            Zdrive_SwitchMachine(&Zmotor[i], (uint8_t)(i + 1U));
            continue;
        }

        /* 运行状态机 */
        Zdrive_RunMachine(&Zmotor[i], (uint8_t)(i + 1U));
    }

    /* 任何时候都执行的 ask:广播查询反馈,入队时自动覆盖两条总线 */
    ZdriveAsk(0, Pur);
    /* ZdriveAsk(0, PosIn); */
    ZdriveAsk(0, Vel);
}
// #endif /* USE_ZMDR */

/* 直接发送 CAN 帧,不入队,不检查总线,不检查电机 ID,不检查模式 */
static void Zdrive_SendDirect(uint32_t id, uint8_t dlc, const uint8_t *data)
{
    uint8_t tx_data[8]={0};
    memcpy(tx_data, data, dlc);
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox;
    tx_header.StdId = id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = dlc;
    if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data,&tx_mailbox) != HAL_OK)
    {
        
    }
}

/* 设置电机目标模式和目标值,并标记为待处理,在 ZdriveFunc 中处理，通常用于上位机 */
void Zdrive_Set_target_mode(uint8_t id, ZdriveMode mode, float target)
{
    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
         return;
    }
    Zmotor[id - 1U].mode = mode;
   Zmotor[id - 1U].target.pendingTarget = target;
     Zmotor[id - 1U].target.isPending = true;
}

float Zdrive_Get_Position(uint8_t id)
{
    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return 0.0f;
    }
    return Zmotor[id - 1U].valReal.pos_deg;
}

float Zdrive_GetSpeed(uint8_t id)
{
    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return 0.0f;
    }
    return Zmotor[id - 1U].valReal.speed_rpm;
}

/*设置电机初始化运行状态*/
void Zdrive_Begin(uint8_t id)
{
    if ((id == 0U) || (id > USE_ZDRIVE_NUM))
    {
        return;
    }
    Zmotor[id - 1U].Begin = true;
}