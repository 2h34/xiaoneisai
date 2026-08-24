#include "can_app.h"
#include "can.h"
#include"beep.h"
#include "led.h"
#include "dji_motor.h"
#include "ZDrive.h"


static CAN_RxHeaderTypeDef can_rx_header;
static uint8_t can_rx_data[8];

static volatile uint8_t beep_request = 0U;
static volatile uint8_t beep_count = 0U;
static uint8_t beep_workmode = 0U;

static volatile uint8_t flow_request = 0U;
static volatile uint8_t flow_target_state = 0U;





void can_app_init(void)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_16BIT;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    filter.FilterIdHigh = 0x0000U;
    filter.FilterIdLow  = 0x0000U;
    filter.FilterMaskIdHigh = 0x0000U;
    filter.FilterMaskIdLow =  0x0000U;

    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_CAN_Start(&hcan1) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_CAN_ActivateNotification(&hcan1,CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance != CAN1)
    {
        return;
    }
    if (HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&can_rx_header,
                             can_rx_data) != HAL_OK)
    {
        return;
    }
    if((can_rx_header.IDE != CAN_ID_STD) ||
    (can_rx_header.RTR != CAN_RTR_DATA))
    {
        return;
    }
    if ((can_rx_header.StdId >= 0x201U) &&
    (can_rx_header.StdId <= 0x204U) &&
    (can_rx_header.DLC >= 6U))
    {
        DJI_motor_Receive(&can_rx_header, can_rx_data);
        return;
    }
    else
    {
        ZdriveReceive(&can_rx_header, can_rx_data);
    }




    // if ((can_rx_header.IDE == CAN_ID_EXT) &&
    // (can_rx_header.RTR == CAN_RTR_DATA) &&
    // (can_rx_header.DLC == 1U))
    // {
    //     if (can_rx_header.ExtId == 0x01020101U)
    //     {
    //         beep_count = can_rx_data[0];
    //         beep_request = 1;
    //     }
    //     if (can_rx_header.ExtId == 0x01020201U)
    //     {
    //         if (can_rx_data[0] == 1)
    //         {
    //             /* code */
    //             flow_request = 1;
    //             flow_target_state = 1;
    //         }
    //         if (can_rx_data[0] == 0)
    //         {
    //             /* code */
    //             flow_request = 1;
    //             flow_target_state=0;
    //         }
    //     }
    // }
}


void can_app_process(uint32_t current_tick)
{
    if (beep_request == 1U)
    {
        /* code */
        BEEP_Trigger(current_tick,beep_count);
        beep_request = 0U;
        beep_workmode = 1;
    }
    if (beep_workmode == 1U && (!beep_is_work()))
    {
        /* code */
        CAN_TxHeaderTypeDef tx_header ={0};
        uint8_t can_tx_data[8] = {0};
        uint32_t tx_mailbox;

        can_tx_data[0]='O';can_tx_data[1]='K';
        tx_header.DLC=2U;tx_header.ExtId= 0x02010101U;
        tx_header.IDE = CAN_ID_EXT;
        tx_header.RTR = CAN_RTR_DATA;
        tx_header.TransmitGlobalTime = DISABLE;

         if (HAL_CAN_AddTxMessage(&hcan1,
                             &tx_header,
                             can_tx_data,
                             &tx_mailbox) == HAL_OK)
        {
            beep_workmode = 0U;
        }
    }
    
    if (flow_request == 1U)
    {
        /* code */
        if (flow_target_state == 1U)
        {
            LED_Setmode(STATE_FLOW,current_tick);
        }
        else if (flow_target_state == 0U)
        {
            LED_Setmode(STATE_OFF,current_tick);
        }
        flow_request = 0;
        CAN_TxHeaderTypeDef tx_header ={0};
        uint8_t can_tx_data[8] = {0};
        uint32_t tx_mailbox;

        can_tx_data[0]='O';can_tx_data[1]='K';can_tx_data[2]=(char)flow_target_state;
        tx_header.DLC=3U;tx_header.ExtId= 0x02010201U;
        tx_header.IDE = CAN_ID_EXT;
        tx_header.RTR = CAN_RTR_DATA;
        tx_header.TransmitGlobalTime = DISABLE;

         if(HAL_CAN_AddTxMessage(&hcan1,
                             &tx_header,
                             can_tx_data,
                             &tx_mailbox) != HAL_OK)
        {
            Error_Handler();
        }
    }   
    
}