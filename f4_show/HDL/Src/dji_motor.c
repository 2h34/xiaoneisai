#include "dji_motor.h"

DJI_motor_t dji_motor[4];

static void DJI_motor_AngleCalculate(DJI_motor_t *motor);
static void DJI_motor_CurrentTransmit(void);
static void DJmotor_SpeedMode(DJI_motor_t *motor);
static void DJmotor_CurrentMode(DJI_motor_t *motor);
static void DJmotor_PositionMode(DJI_motor_t *motor);
static void DJmotor_ZeroMode(DJI_motor_t *motor);
static int16_t ClampPeak(int16_t value, int16_t limit);
static void DJmotor_SwitchMode(DJI_motor_t *motor);


void DJI_motor_init(void)
{
    for (int i = 0; i < 4; i++)
    {
        dji_motor[i].id = i + 1;
        dji_motor[i].rpm = 0.0f;
        dji_motor[i].raw_rpm = 0;

        dji_motor[i].current = 0;
        dji_motor[i].target_current = 0;

        dji_motor[i].position = 0;
        dji_motor[i].target_position = 0;

        dji_motor[i].encoder = 0;
        dji_motor[i].encoder_total = 0;
        dji_motor[i].encoder_initialized = 0;
        dji_motor[i].pulse_per_round = 8192;
        dji_motor[i].reduction_ratio = 1.0f;
        dji_motor[i].mode = DJ_Disable;
    
        dji_motor[i].target_rpm = 0.0f;
        dji_motor[i].current_cmd = 0;
        dji_motor[i].feedback_valid = 0;

        dji_motor[i].zero_rpm = -100.0f;
        dji_motor[i].zero_cnt = 0;
        dji_motor[i].zero_current_limit = 100;
        dji_motor[i].zero_distance = 10;
        dji_motor[i].zero_flag = 0;
    
        dji_motor[i].mode_set = DJ_Disable;
        PID_Init(&dji_motor[i].speed_pid,
         2, 0, 0.2,1000,2000);
        PID_Init(&dji_motor[i].position_pid,
         2, 0, 0.2,1000,2000);
    }
}

/*计算返回总的旋转角度，脉冲->角度*/
static void DJI_motor_AngleCalculate(DJI_motor_t *motor)
{
    if (motor->encoder_initialized == 0)
    {
        motor->last_encoder = motor->encoder;
        motor->encoder_initialized = 1;
        return;
    }

    motor->encoder_delta = (int16_t)(motor->encoder - motor->last_encoder);
    if (motor->encoder_delta > 4096)
    {
        motor->encoder_delta -= 8192;
    }
    else if (motor->encoder_delta < -4096)
    {
        motor->encoder_delta += 8192;
    }

    motor->encoder_total += motor->encoder_delta;
    motor->position = (double)motor->encoder_total *360.0f / 
                      ((double)motor->pulse_per_round * motor->reduction_ratio);

    motor->last_encoder = motor->encoder;
}
    



/* 接收电机反馈信息，解析并更新电机状态 */
void DJI_motor_Receive(CAN_RxHeaderTypeDef *rx_header,uint8_t *rx_data)
{
    if (rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA)
    {
        return;
    }
    if (rx_header->StdId < 0x201 || rx_header->StdId > 0x204)
    {
        return;
    }
    uint8_t motor_id = (uint8_t)(rx_header->StdId - 0x200U); // Assuming the motor ID is encoded in the CAN ID
    if (motor_id < 1 || motor_id > 4)
    {
        return; 
    }

    DJI_motor_t *motor = &dji_motor[motor_id - 1U];

    motor->encoder = (int16_t)((rx_data[0] << 8) | rx_data[1]);
    motor->raw_rpm = (int16_t)((rx_data[2] << 8) | rx_data[3]);motor->rpm =(float)motor->raw_rpm / motor->reduction_ratio*1.0f;
    motor->current = (int16_t)((rx_data[4] << 8) | rx_data[5]);
    motor->feedback_valid = 1;
    DJI_motor_AngleCalculate(motor);
   
}


/* 处理电机控制逻辑，根据当前模式和目标值计算输出电流并发送 */
void DJI_motor_Func(void)
{
    for (int i = 0; i < 4; i++)
    {
        DJmotor_SwitchMode(&dji_motor[i]);
        DJI_motor_t *motor = &dji_motor[i];
        //若无有效反馈，禁止输出
        if (motor->feedback_valid == 0)
        {
            motor->current_cmd = 0;
            continue; 
        }
        
        switch (motor->mode)
        {
            case DJ_RPM:
                DJmotor_SpeedMode(motor);
                break;
            case DJ_Position:
                DJmotor_PositionMode(motor);
                break;
            case DJ_Current:
                DJmotor_CurrentMode(motor);
                break;
            case DJ_Zero:
                DJmotor_ZeroMode(motor);
                break;
            case DJ_Disable:
                motor->current_cmd = 0;
                break;
            default:
                motor->current_cmd = 0;
                break;
        }
        
    }
    DJI_motor_CurrentTransmit();
}

/* 设置电机模式，若模式改变则重置PID控制器 */
void DJI_motor_SetMode(DJI_motor_t *motor,DJ_motor_mode_t mode)
{
    if (motor == NULL)
    {
        return;
    }
    motor->mode_set = mode;
    if (motor->mode!= mode)
    {
        if (mode == DJ_Zero)
        {
        motor->zero_cnt = 0;
        motor->zero_flag = 0;
        }
        motor->mode = mode;
        motor->current_cmd = 0;
        PID_Reset(&motor->speed_pid);
        PID_Reset(&motor->position_pid);
    }
}

/* 切换模式时调用，若设置的模式与当前模式不同，则更新当前模式 */
static void DJmotor_SwitchMode(DJI_motor_t *motor)
{
    if (motor-> mode_set != motor->mode)
    {
        DJI_motor_SetMode(motor, motor->mode_set);
    }
}

/* 发送电机输出电流命令，通过CAN总线发送 */
static void DJI_motor_CurrentTransmit(void)
{
    uint8_t tx_data[8]={0};
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox;
    tx_header.StdId = 0x200U;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8U;
    
    for (int i = 0; i < 4; i++)
    {
        uint16_t current_cmd = dji_motor[i].current_cmd;
        tx_data[i * 2] = (uint8_t)(current_cmd >> 8);
        tx_data[i * 2 + 1] = (uint8_t)(current_cmd & 0xFF);
    }


    HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data,&tx_mailbox);
}   

static void DJmotor_SpeedMode(DJI_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    }
    double Ts = 0.002;
    double output = PID_Caculate(&motor->speed_pid, motor->target_rpm, motor->rpm, Ts);
    motor->current_cmd = (int16_t)output;
}

static void DJmotor_CurrentMode(DJI_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    } 
    motor->current_cmd = motor->target_current;
}

//设置输出轴目标速度
void DJI_motor_Set_Speed(DJI_motor_t *motor,float target_rpm)
{
    if (motor == NULL)
    {
        return;
    }
    motor->target_rpm = target_rpm;
}

/* 设置输出轴目标位置-角度 */
void DJI_motor_Set_Position(DJI_motor_t *motor,double target_position)
{
    if (motor == NULL)
    {
        return;
    }
    motor->target_position = target_position;
}

void DJI_motor_Set_Current(DJI_motor_t *motor, int16_t target_current)
{
    if (motor == NULL)
    {
        return;
    }

    if (target_current > 10000)
    {
        target_current = 10000;
    }
    else if (target_current < -10000)
    {
        target_current = -10000;
    }

    motor->target_current = target_current;
}

/* 设置电机零点 */
void DJI_motor_Set_Zero(DJI_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    }
    motor->encoder_total = 0;
    motor->position = 0.0;
    motor-> target_position = 0.0;
    motor->zero_flag = 1;
}


static void DJmotor_PositionMode(DJI_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    }
    double Ts = 0.002;
    double output1 = PID_Caculate(&motor->position_pid, motor->target_position, motor->position, Ts);
    motor->target_rpm = (float)output1;   
    double output2 = PID_Caculate(&motor->speed_pid, motor->target_rpm, motor->rpm, Ts);
    motor->current_cmd = (int16_t)output2;

}

/* 限幅函数，将值限制在[-limit, limit]范围内 */
static int16_t ClampPeak(int16_t value, int16_t limit)
{
    if (value > limit)
    {
        return limit;
    }
    else if (value < -limit)
    {
        return -limit;
    }

    return value;
}



/* 寻零模式，电机以设定的零点速度运行，直到编码器位置接近零点位置（机械限位） */
static void DJmotor_ZeroMode(DJI_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    }
    motor->target_rpm = motor->zero_rpm;
    double Ts = 0.002;
    motor->current_cmd= (int16_t)PID_Caculate(&motor->speed_pid, motor->target_rpm, motor->rpm, Ts);
    motor->current_cmd = ClampPeak(motor->current_cmd, motor->zero_current_limit);

    if (ABS(motor->encoder_delta) < motor->zero_distance)
    {
        motor->zero_cnt++;
        if (motor->zero_cnt > 100U )
        {
            motor->zero_cnt = 0;
            PID_Reset(&motor->speed_pid);
            PID_Reset(&motor->position_pid);
            DJI_motor_Set_Zero(motor);
            motor->current_cmd = 0;
            motor->mode = DJ_Disable;
            motor->mode_set = DJ_Disable;
        }
    }
    else
    {
        motor->zero_cnt = 0;
    }
}

/* 根据电机ID获取对应的电机结构体指针，若ID无效则返回NULL */
DJI_motor_t *DJI_motor_GetById(uint8_t id)
{
    if (id < 1 || id > 4)
    {
        return NULL;
    }
    return &dji_motor[id - 1];
}


