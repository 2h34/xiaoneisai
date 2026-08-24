#ifndef PID_H
#define PID_H

typedef enum
{
    PID_MODE_POSITION = 0,
    PID_MODE_DELTA = 1,
} PID_Mode_t;

typedef struct
{
    double Kp;
    double Ki;
    double Kd;

    double last_error;
    double last_last_error;
    double integral;
    double output;

    double output_limits;
    double integral_limits;
    PID_Mode_t mode;
    int isfirst_feedback; 
} PID_t;

void PID_Init(PID_t *pid,double Kp,double Ki,double Kd,double output_limits,double integral_limits);
double PID_Caculate(PID_t *pid,double target,double current,double Ts);
double PID_Caculate_Position(PID_t *pid,double target,double current,double Ts);
double PID_Caculate_Delta(PID_t *pid,double target,double current,double Ts);
void PID_Reset(PID_t *pid);

#endif // PID_H