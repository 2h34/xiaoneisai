#ifndef DJI_MOTOR_H
#define DJI_MOTOR_H

#include "main.h"
#include "pid.h"
#include "can.h"

#define ABS(x) ((x) >= 0 ? (x) : -(x))

typedef enum
{
    DJ_Disable = 0,
    DJ_RPM = 1,
    DJ_Position = 2,
    DJ_Current = 3,
    DJ_Zero = 4,
} DJ_motor_mode_t;

typedef struct
{
    uint16_t id;
    int16_t raw_rpm;  // 电机侧原始 CAN rpm
    float rpm;       // 输出轴转速，单位为 RPM
    float target_rpm;//输出轴目标速度

    int16_t current;  //Current是转矩
    int16_t current_cmd; //输出的目标电流
    int16_t target_current;

    double position; //输出轴位置
    double target_position; //输出轴目标位置

    float zero_rpm;
    uint32_t zero_cnt;  //零点校准计数器
    int16_t zero_current_limit;
    int16_t zero_distance;
    uint8_t zero_flag;   //零点是否校准标志位，1表示已校准，0表示未校准

    int16_t encoder;  //电机侧原始 CAN 编码器值
    int16_t last_encoder;  
    int16_t encoder_delta;
    int32_t encoder_total;
    uint8_t encoder_initialized;  //编码器是否初始化标志位，1表示已初始化，0表示未初始化

    uint16_t pulse_per_round;   //电机侧编码器每圈脉冲数
    float reduction_ratio;     //减速比

    DJ_motor_mode_t mode;    //当前模式
    DJ_motor_mode_t mode_set;   //设置的模式，可能与当前模式不同
    PID_t speed_pid;
    PID_t position_pid;

    uint8_t feedback_valid;   //电机反馈是否有效，1表示有效，0表示无效

} DJI_motor_t;


void DJI_motor_init(void);
void DJI_motor_Receive(CAN_RxHeaderTypeDef *rx_header,uint8_t *rx_data);
void DJI_motor_Func(void);


/*上层接口*/
void DJI_motor_SetMode(DJI_motor_t *motor,DJ_motor_mode_t mode);  //设置模式
void DJI_motor_Set_Speed(DJI_motor_t *motor,float target_rpm);   //设置目标速度
void DJI_motor_Set_Position(DJI_motor_t *motor,double target_position);   //设置目标位置
void DJI_motor_Set_Current(DJI_motor_t *motor, int16_t target_current);  //设置目标电流



void DJI_motor_Set_Zero(DJI_motor_t *motor);    //设置零点
DJI_motor_t *DJI_motor_GetById(uint8_t id);

    




#endif // DJI_MOTOR_H