/*
 * @Author: Frt001 2067314783@qq.com
 * @Date: 2026-08-13 10:00:13
 * @LastEditors: Frt001 2067314783@qq.com
 * @LastEditTime: 2026-08-25 16:43:21
 * @FilePath: \f4_show\IRQ\Src\CAN_IRQHandler.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "CAN_IRQHandler.h"
#include "DJmotor.h"
#include "ZDrive.h"
#include "BlockArm.h"
#include "BlockVacuum.h"

typedef enum{
    LEFT_START_PICK = 0x03U,
    // FINE_ADJUST_START,
    // FINE_ADJUST_STOP,
    // LEFT_CONFIRM_GRAB,
    LEFT_START_PLACE,
    RIGHT_START_PICK,
    RIGHT_START_PLACE,
    LEFT_CONFIRM_RELEASE,
    RIGHT_CONFIRM_RELEASE,
    // STOP,
} signal_type;

#define RESET       0xFFU
#define ERROR       0xEEU
#define DISABLE     0x00U
#define ENABLE      0x01U
#define LOWPICK     0x00U
#define HIGHPICK    0x01U
#define BOTTOMPLACE 0x00U
#define LEVEL1PLACE 0x01U
#define LEVEL2PLACE 0x02U
#define BOTHARM     0x00U
#define LEFTARM     0x01U
#define RIGHTARM    0x02U
#define GET_COMMAND 0x00U
#define FINISH      0x01U

// 发送消息
void send_disable_message(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *TxHeader,
    uint8_t TxData[], uint32_t *TxMailbox, uint8_t process)
{
    TxHeader->IDE   = CAN_ID_EXT;
    TxHeader->RTR   = CAN_RTR_DATA;
    TxHeader->ExtId = 0x05010100 | process;
    TxHeader->DLC = 2;
    TxHeader->TransmitGlobalTime = DISABLE; 
    TxData[0] = 0x02;
    TxData[1] = process;

    HAL_CAN_AddTxMessage(&hcan1, TxHeader, TxData, TxMailbox);
}

void send_busy_message(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *TxHeader,
    uint8_t TxData[], uint32_t *TxMailbox, uint8_t process)
{
    TxHeader->IDE   = CAN_ID_EXT;
    TxHeader->RTR   = CAN_RTR_DATA;
    TxHeader->ExtId = 0x05010100 | process;
    TxHeader->DLC = 2;
    TxHeader->TransmitGlobalTime = DISABLE; 
    TxData[0] = 0x03;
    TxData[1] = process;

    HAL_CAN_AddTxMessage(&hcan1, TxHeader, TxData, TxMailbox);
}

void send_receive_message(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *TxHeader,
    uint8_t TxData[], uint32_t *TxMailbox, uint8_t process)
{
    TxHeader->IDE   = CAN_ID_EXT;
    TxHeader->RTR   = CAN_RTR_DATA;
    TxHeader->ExtId = 0x05010100 | process;
    TxHeader->DLC = 2;
    TxHeader->TransmitGlobalTime = DISABLE; 
    TxData[0] = GET_COMMAND;
    TxData[1] = process;

    HAL_CAN_AddTxMessage(&hcan1, TxHeader, TxData, TxMailbox);
}

void send_finish_message(CAN_HandleTypeDef *hcan, CAN_TxHeaderTypeDef *TxHeader,
    uint8_t TxData[], uint32_t *TxMailbox, uint8_t process)
{
    TxHeader->IDE   = CAN_ID_EXT;
    TxHeader->RTR   = CAN_RTR_DATA;
    TxHeader->ExtId = 0x05010100 | process;
    TxHeader->DLC = 2;
    TxHeader->TransmitGlobalTime = DISABLE; 
    TxData[0] = FINISH;
    TxData[1] = process;

    HAL_CAN_AddTxMessage(&hcan1, TxHeader, TxData, TxMailbox);
}

// 接收关于左臂的消息
void Master_message_handler_left(CAN_HandleTypeDef *hcan, 
    CAN_RxHeaderTypeDef RxHeader, uint8_t *RxData)
{
    uint8_t signal = RxHeader.ExtId & 0xFFU;
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    uint8_t TxData[8];

    if (is_busy() && signal != 0x01 && signal != 0x00)
    {
        send_busy_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
        return;
    }

    switch (signal){

        case 0x00 :{
            if (RxHeader.DLC == 0U){
                TxHeader.IDE   = CAN_ID_EXT;
                TxHeader.RTR   = CAN_RTR_DATA;
                TxHeader.ExtId = 0x05010100;
                TxHeader.DLC = 2;
                TxHeader.TransmitGlobalTime = DISABLE; 

                if (is_disabled()){
                    TxData[0] = 0x02;
                    TxData[1] = 0x00;
                }else if (is_busy()){
                    TxData[0] = 0x01;
                    TxData[1] = 0x80;
                }else {
                    TxData[0] = 0x00;
                    TxData[1] = 0x80;
                }

                HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailBox);
            }
            break;
        }

        case 0x01 :{
            if (RxHeader.DLC == 2){
                if (RxData[0] == DISABLE){
                    BlockArm_Disable();
                    TxHeader.IDE   = CAN_ID_EXT;
                    TxHeader.RTR   = CAN_RTR_DATA;
                    TxHeader.ExtId = 0x05010101;
                    TxHeader.DLC = 2;
                    TxHeader.TransmitGlobalTime = DISABLE; 

                    TxData[0] = 0x00;
                    TxData[1] = 0x00;

                    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailBox);
                }
                else if (RxData[0] == ENABLE){
                    BlockArm_Enable();
                    BlockArm_Home();

                    TxHeader.IDE   = CAN_ID_EXT;
                    TxHeader.RTR   = CAN_RTR_DATA;
                    TxHeader.ExtId = 0x05010101;
                    TxHeader.DLC = 2;
                    TxHeader.TransmitGlobalTime = DISABLE; 

                    TxData[0] = 0x01;
                    TxData[1] = 0x00;

                    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailBox);
                }
            }
            break;
        }

        case RESET :{
            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if ((RxData[0] == BOTHARM || RxData[0] == LEFTARM) && RxData[1] == 0x00){
                send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                BlockArm_Reset();
                send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
            }
            break;
        }

        case ERROR:{
            send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
            break;
        }

        case LEFT_START_PICK:{
            
            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }
            
            if (RxHeader.DLC == 1U){         
                if (RxData[0] == LOWPICK){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToLowPickReady();
                    BlockVacuum_Grab();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }else if (RxData[0] == HIGHPICK){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToHighPickReady();
                    BlockVacuum_Grab();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }   
            }         
            break;
        }

        // case 0x05:{
        //     // 暂无，未来可能有
        // }

        // case 0x06:{
        //     // 暂无，未来可能有
        // }

        // case CONFIRM_GRAB:{
        //     if (RxData[1] == 0x00)
        //     {
        //         BlockVacuum_Grab();
        //     }
        //     break;
        // }

        case LEFT_START_PLACE:{

            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if (RxHeader.DLC == 1U){
                if (RxData[0] == BOTTOMPLACE){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToPlaceBottomReady();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }else if (RxData[0] == LEVEL1PLACE){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToPlaceLevel1Ready();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }else if (RxData[0] == LEVEL2PLACE){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToPlaceLevel2Ready();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }
            }
            break;
        }

        // case CONFIRM_RELEASE:{
        //     if (RxData[1] == 0x00){
        //         BlockVacuum_Release();
        //     }
        //     break;
        // }

        case LEFT_CONFIRM_RELEASE:{

            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if (RxHeader.DLC == 1U){
                send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                BlockVacuum_Release();
                send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
            }
            break;
        }

        // case STOP:{
        //     if (RxData[1] == 0x00){
        //         BlockArm_Stop();
        //     }
        // }
    }
}

// 接收关于右臂的消息
void Master_message_handler_right(CAN_HandleTypeDef *hcan, 
    CAN_RxHeaderTypeDef RxHeader, uint8_t *RxData)
{
    uint8_t signal = RxHeader.ExtId & 0xFFU;
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    uint8_t TxData[8];

    if (is_busy() && signal != 0x00 && signal != 0x01)
    {
        send_busy_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
        return;
    }

    switch (signal){

        case 0x00 :{
            if (RxHeader.DLC == 0U){
                TxHeader.IDE   = CAN_ID_EXT;
                TxHeader.RTR   = CAN_RTR_DATA;
                TxHeader.ExtId = 0x05010100;
                TxHeader.DLC = 2;
                TxHeader.TransmitGlobalTime = DISABLE; 

                if (is_disabled()){
                    TxData[0] = 0x02;
                    TxData[1] = 0x00;
                }else if (is_busy()){
                    TxData[0] = 0x01;
                    TxData[1] = 0x80;
                }else {
                    TxData[0] = 0x00;
                    TxData[1] = 0x80;
                }

                HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailBox);
            }
            break;
        }

        case 0x01 :{
            if (RxHeader.DLC == 2){
                if (RxData[0] == DISABLE){
                    BlockArm_Disable();
                    TxHeader.IDE   = CAN_ID_EXT;
                    TxHeader.RTR   = CAN_RTR_DATA;
                    TxHeader.ExtId = 0x05010101;
                    TxHeader.DLC = 2;
                    TxHeader.TransmitGlobalTime = DISABLE; 

                    TxData[0] = 0x00;
                    TxData[1] = 0x00;

                    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailBox);
                }
                else if (RxData[0] == ENABLE){
                    BlockArm_Enable();
                    BlockArm_Home();

                    TxHeader.IDE   = CAN_ID_EXT;
                    TxHeader.RTR   = CAN_RTR_DATA;
                    TxHeader.ExtId = 0x05010101;
                    TxHeader.DLC = 2;
                    TxHeader.TransmitGlobalTime = DISABLE; 

                    TxData[0] = 0x01;
                    TxData[1] = 0x00;

                    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailBox);
                }
            }
            break;
        }

        case RESET :{

            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if (
                (RxData[0] == BOTHARM || RxData[0] == RIGHTARM) && 
                RxData[1] == 0x00 &&
                RxHeader.DLC == 2
            ){
                send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                BlockArm_Reset();
                send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
            }
            break;
        }

        case ERROR:{
            send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
            break;
        }

        case RIGHT_START_PICK:{   

            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if (RxHeader.DLC == 1U){         
                if (RxData[0] == LOWPICK){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToLowPickReady();
                    BlockVacuum_Grab();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }else if (RxData[0] == HIGHPICK){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToHighPickReady();
                    BlockVacuum_Grab();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }   
            }         
            break;
        }

        // case 0x05:{
        //     // 暂无，未来可能有
        // }

        // case 0x06:{
        //     // 暂无，未来可能有
        // }

        // case CONFIRM_GRAB:{
        //     if (RxData[1] == 0x00)
        //     {
        //         BlockVacuum_Grab();
        //     }
        //     break;
        // }

        case RIGHT_START_PLACE:{

            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if (RxHeader.DLC == 1U){
                if (RxData[0] == BOTTOMPLACE){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToPlaceBottomReady();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }else if (RxData[0] == LEVEL1PLACE){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToPlaceLevel1Ready();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }else if (RxData[0] == LEVEL2PLACE){
                    send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                    BlockArm_MoveToPlaceLevel2Ready();
                    send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                }
            }
            break;
        }

        // case CONFIRM_RELEASE:{
        //     if (RxData[1] == 0x00){
        //         BlockVacuum_Release();
        //     }
        //     break;
        // }

        case RIGHT_CONFIRM_RELEASE:{

            if (is_disabled()){
                send_disable_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                break;
            }

            if (RxHeader.DLC == 1U){
                send_receive_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
                BlockVacuum_Release();
                send_finish_message(hcan, &TxHeader, TxData, &TxMailBox, signal);
            }
            break;
        }

        // case STOP:{
        //     if (RxData[1] == 0x00){
        //         BlockArm_Stop();
        //     }
        // }
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];    

    if (hcan->Instance == CAN1) 
    {
        // 从 FIFO 0 把数据捞出来，存到 RxData 数组里
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
        
            if (RxHeader.IDE == CAN_ID_STD)
            {
                #if USE_DJ && (MOTOR_DJI_CAN_BUS == 0U)
                DJmotor_Receive(RxHeader, RxData);
                #endif
            }
            else if (RxHeader.IDE == CAN_ID_EXT)
            {
                if (RxHeader.ExtId >> 8 == 0x010105)
                {
                    Master_message_handler_left(hcan, RxHeader, RxData);
                    /*Master_message_handler_right(hcan, RxHeader, RxData);
                     *如果在右臂上烧录代码，则把这个从注释里捞出来并注释掉上一句*/
                }
            }            
        }
    } else if (hcan->Instance == CAN2) 
    {
        
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            // 处理 CAN2 的消息...
        }
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK)
        {

        }
    }
    else if (hcan->Instance == CAN2)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK)
        {
        #if USE_ZMDR
            ZdriveReceive(RxHeader, RxData, 1U);
        #endif
        }
    }
}
