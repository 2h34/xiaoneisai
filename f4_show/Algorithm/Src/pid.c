#include "pid.h"

void PID_Init(PID_t *pid,double Kp,double Ki,double Kd,double output_limits,double integral_limits)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;

    pid->last_error = 0.0;
    pid->last_last_error = 0.0;
    pid->integral = 0.0;
    pid->output = 0.0;

    pid->isfirst_feedback = 1;

    pid->output_limits = output_limits;
    pid->integral_limits = integral_limits;

    pid->mode = PID_MODE_POSITION; 
}

double PID_Caculate(PID_t *pid,double target,double current,double Ts)
{
    if (pid->mode == PID_MODE_POSITION)
    {
        return PID_Caculate_Position(pid, target, current, Ts);
    }
    else if (pid->mode == PID_MODE_DELTA)
    {
        return PID_Caculate_Delta(pid, target, current, Ts);
    }
    else
    {
        return 0.0;
    }
    
}

void PID_Reset(PID_t *pid)
{
    pid->last_error = 0.0;
    pid->last_last_error = 0.0;
    pid->integral = 0.0;
    pid->output = 0.0;
    pid->isfirst_feedback = 1;
}

double  PID_Caculate_Position(PID_t *pid,double target,double current,double Ts)
{
    double error = target - current;
    double derivative = 0.0;
    pid->integral += error*Ts;

    if (pid->integral > pid->integral_limits)
    {
        pid->integral = pid->integral_limits;
    }
    else if (pid->integral < -pid->integral_limits)
    {
        pid->integral = -pid->integral_limits;
    }

    if (pid->isfirst_feedback)
    {
        pid->last_error = error;
        derivative = 0.0;
        pid->isfirst_feedback = 0;
    }
    else
    {
        derivative = (error - pid->last_error) / Ts;
    }
    double output = pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;

    if (output > pid->output_limits)
    {
        output = pid->output_limits;
    }
    else if (output < -pid->output_limits)
    {
        output = -pid->output_limits;
    }

    pid->last_error = error;
    return output;
}

double PID_Caculate_Delta(PID_t *pid,double target,double current,double Ts)
{
    double error = target - current;
    double delta_output;
    if (pid->isfirst_feedback)
    {
    pid->last_error = error;
    pid->last_last_error = error;
    pid->isfirst_feedback = 0;

    delta_output =pid->Kp * error + pid->Ki * error * Ts;
    }
    else
    {
    delta_output = pid->Kp * (error - pid->last_error) + pid->Ki * error * Ts + pid->Kd * (error - 2.0 * pid->last_error + pid->last_last_error) / Ts;
    }
    pid->output += delta_output;

    if (pid->output > pid->output_limits)
    {
        pid->output = pid->output_limits;
    }
    else if (pid->output < -pid->output_limits)
    {
        pid->output = -pid->output_limits;
    }

    pid->last_last_error = pid->last_error;
    pid->last_error = error;

    return pid->output;
}