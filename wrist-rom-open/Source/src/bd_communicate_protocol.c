/* Copyright (c) [2014 Baidu]. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File Name          : 
 * Author             : 
 * Version            : $Revision:$
 * Date               : $Date:$
 * Description        : 
 *                      
 * HISTORY:
 * Date               | Modification                    | Author
 * 28/03/2014         | Initial Revision                | 
 
 */
#include "stdint.h"
#include "string.h"

#include "config.h"
#include "bd_communicate_protocol.h"
#include "bd_ble_nus.h"
#include "nrf_error.h"
#include "app_scheduler.h"
#include "app_error.h"
#include "app_timer.h"
#include "simple_uart.h"
#include "bd_private_bond.h"
#include "bd_input_event_source.h"
#include "bd_wall_clock_timer.h"
#include "bd_led_flash.h"
#include "hal_vibrate.h"
#include "hal_acc.h"
#include "step_counter.h"
#include "ble_flash.h"
#include "bd_crc16.h"
#include "bd_led_flash.h"
#include "bd_battery.h"
#include "nrf_assert.h"

#include "bd_sync_data.h"
#include "bd_level_drive_motor.h"
#include "bd_factory_test.h"
#include "bd_interaction.h"
#include "nrf_soc.h"
#include "core_cm0.h"
#include "ble_hci.h"
#include "bd_stack_dump_repoter.h"
#include "ble_debug_assert_handler.h"
#include "hal_acc.h"
#include "bd_data_manager.h"
#include "bd_bluetooth_log.h"
#include "bd_switch_sleep.h"


/****************************************************************************
* Tempory send buffer
*****************************************************************************/
uint8_t global_reponse_buffer[GLOBAL_RESPONSE_BUFFER_SIZE];
//used to send buffer individually
uint8_t global_L1_header_buffer[L1_HEADER_SIZE];

extern SportsData_U mSportsData;
extern bool notify_steps_enable;

extern uint16_t global_connect_handle;

static SEND_TASK_TYPE_T current_task_type = TASK_NONE;
static SEND_TASK_TYPE_T next_task_type = TASK_NONE;

/*************************************************************************
* private bond manchine
**************************************************************************/
BLUETOOTH_BOND_STATE private_bond_machine = PRIVATE_NOT_BOND;
/**************************************************************************
* L1 send sequence id
***************************************************************************/
uint16_t L1_sequence_id = 0;

/***************************************************************************
* define a single response buffer
* used to store response package triggered while sending
****************************************************************************/
static struct Response_Buff_Type_t g_ack_package_buffer =
    {
        0,0,0
    };
static L1_Send_Content * g_next_L1_send_content_buffer = NULL;
/***************************************************************************
* This variable is used to deal with : send header immediately after send response
****************************************************************************/
static L1_Header_Schedule_type_t L1_header_need_schedule = {NULL,0};


#ifdef DEBUG_LOG
void simple_send_hex(uint8_t * content, uint8_t length);
void simple_uart_putstring(const uint8_t *str);
#endif

/* Macro defination */
#define   RESEND_DELAY          APP_TIMER_TICKS(8000, APP_TIMER_PRESCALER)
#define   RECEIVE_TIMEOUT       APP_TIMER_TICKS(8000, APP_TIMER_PRESCALER)
#define   USER_ACTION_TIMEOUT        APP_TIMER_TICKS(60000,APP_TIMER_PRESCALER)
#define         OTA_DELAY_TIMEOUT                   APP_TIMER_TICKS(5000,APP_TIMER_PRESCALER)

/* static & global varable */
void schedule_async_send(void * contenxt,SEND_TASK_TYPE_T task);
extern ble_nus_t                        m_nus;
static L1_Send_Content sendContent[MAX_SEND_TASK];
static app_timer_id_t delay_send_wait_timer;   /* This timer is used to */
static app_timer_id_t receive_time_out_timer;   /* receive time out timer */
app_timer_id_t user_action_delay_timer;    /* wait for user action timeout timer */
extern app_timer_id_t  ota_delay_timer;     /* ota delay timer*/


void stop_health_algorithm(void);

/***********************************************************************
* global flags used to trigger ota mode 
************************************************************************/
bool global_should_trigger_ota = false;

bool get_global_should_trigger_ota_flag(void)
{
    return global_should_trigger_ota;
}

void set_global_should_trigger_ota_flag(bool value)
{
    global_should_trigger_ota = value;
}

/************************************************************************
* Is bluetooth connected
*************************************************************************/
bool is_bluetooth_connected(void)
{
    return (global_connect_handle != BLE_CONN_HANDLE_INVALID);
}


void async_send(void * para,uint16_t event_size,SEND_TASK_TYPE_T new_task_type);
static uint32_t L1_resend_package(L1_Send_Content * content);
static L1Header_t* construct_response_package(uint16_t sequence_id, bool check_success);
static void delay_send_func(void * context)
{
    uint32_t error_code;
    L1_Send_Content * content = context;
    SendCompletePara sendPara;

    //content != NULL
    if(content->isUsed == 0) {
        return;
    }

    if(content->contentLeft == 0) { //send complete but not get ack

        //wait response package timeout
        //whole package resent
        if(content->resendCount < 3) {
            content->resendCount++;

            LOG(LEVEL_INFO,"time out resend \r\n");

            error_code = L1_resend_package(content);
            if(error_code == NRF_SUCCESS) {
                return;
            }

        }

    }

    //comes here for some reason :(1) resend more than three times (2)data not send complete but content buffer is full last more than 8s
    sendPara.callback = NULL;
    sendPara.context = NULL;
    sendPara.task_type = TASK_NONE;
    set_complete_callback(sendPara); //cancle callback

    current_task_type = TASK_NONE;
    next_task_type = TASK_NONE;

    content->isUsed = 0; //clean current send, then it can be used by other send request
    if(content->callback) { //if callback was set
        content->callback(SEND_FAIL);
    }

}

/*****************************************************************
* receiver timeout handle
******************************************************************/
static void receive_time_out_handle(void * contenxt)
{
    RECEIVE_STATE * state = (RECEIVE_STATE *)contenxt;

    *state = WAIT_START; /* restart receive state machine*/
#ifdef DEBUG_LOG
    //simple_uart_putstring((const uint8_t * )"receive time out so restart machine \r\n");
#endif
}

/* bonding time out handle */
static void user_action_timeout_handle(void * context)
{
    USER_TIMER_COMMAND_t command = (USER_TIMER_COMMAND_t)context;
    if(command == DO_BOND) {
        led_action_stop();
        //        reset_short_press_action_SM(INPUT_ACCEPT_BOND);
        bond_fail_event_dispatch();
    } else if(command == DO_WAIT_BOND_COMMAND) { //wait bond time out
        if(global_connect_handle != BLE_CONN_HANDLE_INVALID) { //still connected
            sd_ble_gap_disconnect(global_connect_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION); //disconnect to the peer
        }
    } else {}
}


/**********************************************************************
* User to reset the MCU & enter OTA
***********************************************************************/
void trigger_ota_mode(void)
{

    stop_health_algorithm();

    hal_acc_reset();

  /***********************************************************
   //stop all the timer 
    app_timer_stop_all();
    //disable radio
    radio_disable();
    //disalbe all irq
    __disable_irq();
   ***********************************************************/

    //store time into flash
    ota_pre_restart_info_store();

    //alreay disabled softdevice
    sd_power_gpregret_set(0x01);
    //reset the system and start OTA
    NVIC_SystemReset();

}


/**********************************************************************
 * OTA time out handle
 **********************************************************************/
void ota_time_out_handle(void * context)
{
    (void)context;
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"will switch to OTA mode\r\n");
    //simple_uart_putstring((const uint8_t *)"will switch to OTA mode\r\n");
#endif
    //set flags to enter ota
    set_global_should_trigger_ota_flag(true);
    sd_ble_gap_disconnect(global_connect_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);

}


/**********************************************************************
* init the environment for private protocol
***********************************************************************/
uint32_t bluetooth_l0_init(void)
{
    uint32_t i = 0;
    uint32_t error_code;

    for( ; i< MAX_SEND_TASK; ++i) {
        sendContent[i].isUsed = 0;
        sendContent[i].resendCount = 0;
    }

    /*create  delay schedule function */
    error_code = app_timer_create(&delay_send_wait_timer, APP_TIMER_MODE_SINGLE_SHOT, delay_send_func);
    APP_ERROR_CHECK(error_code);

    error_code = app_timer_create(&receive_time_out_timer,APP_TIMER_MODE_SINGLE_SHOT,receive_time_out_handle);
    APP_ERROR_CHECK(error_code);

    error_code = app_timer_create(&user_action_delay_timer,APP_TIMER_MODE_SINGLE_SHOT,user_action_timeout_handle);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}


/**********************************************************************
* register wait response package
***********************************************************************/
static L1_Send_Content* current_package_wait_response = NULL;
static void register_wait_response(L1_Send_Content * content)
{
    current_package_wait_response = content;
}

/**********************************************************************
* Whole package resend
***********************************************************************/
static uint32_t L1_resend_package(L1_Send_Content * content)
{
    LOG(LEVEL_INFO,"will resend a package\r\n");

    if(!content) {
        return NRF_ERROR_INVALID_PARAM;
    }

    /*fill header*/
    global_L1_header_buffer[L1_HEADER_MAGIC_POS] = L1_HEADER_MAGIC;           /* Magic */
    global_L1_header_buffer[L1_HEADER_PROTOCOL_VERSION_POS] = L1_HEADER_VERSION;       /* protocol version */
    global_L1_header_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] = (content->length >> 8 & 0xFF);    /* length high byte */
    global_L1_header_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] = (content->length & 0xFF);      /* length low byte */
    /*cal crc*/
    uint16_t crc16_ret = bd_crc16(0,content->content,content->length);
    global_L1_header_buffer[L1_HEADER_CRC16_HIGH_BYTE_POS] = ( crc16_ret >> 8) & 0xff;
    global_L1_header_buffer[L1_HEADER_CRC16_LOW_BYTE_POS] = crc16_ret & 0xff;

    //sequence id
    global_L1_header_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] = (L1_sequence_id >> 8) & 0xff;
    global_L1_header_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] = L1_sequence_id & 0xff;

    //prepare for send L2 content
    content->contentLeft = content->length;
    content->sequence_id = L1_sequence_id;
    //every time send a package,increase L1_sequence_id, whether it's success or not
    L1_sequence_id ++;

    //register need to schedule header
    L1_header_need_schedule.isUsed = 1;
    L1_header_need_schedule.content = content;

    //schedule async send
    schedule_async_send(content,TASK_DATA);
    return NRF_SUCCESS;
}

/************************************************************************
* If receive response package call this function
*************************************************************************/
static void response_package_handle(uint16_t sequence_id,uint8_t crc_check)
{
    uint32_t err_code;

    SendCompletePara sendPara;
    if(!current_package_wait_response) {
        LOG(LEVEL_INFO," no package wait res\r\n");

        return;
    }


    if(current_package_wait_response->sequence_id == sequence_id ) {

        // get response for current package so stop timer
        app_timer_stop(delay_send_wait_timer);


        if( crc_check == CRC_SUCCESS) {

            LOG(LEVEL_INFO,"get response show crc succ\r\n");

            sendPara.callback = NULL;
            sendPara.context = NULL;
            sendPara.task_type = TASK_NONE;
            set_complete_callback(sendPara); //cancle callback

            current_package_wait_response->isUsed = 0;
            if(current_package_wait_response->callback) {
                current_package_wait_response->callback(SEND_SUCCESS);
            }


        } else { //error resend
            LOG(LEVEL_INFO,"get response show crc fail\r\n");

            if(current_package_wait_response->resendCount >= 3) {

                sendPara.callback = NULL;
                sendPara.context = NULL;
                sendPara.task_type = TASK_NONE;
                set_complete_callback(sendPara); //cancle callback

                current_task_type = TASK_NONE;
                next_task_type = TASK_NONE;
                current_package_wait_response->isUsed = 0;

                if(current_package_wait_response->callback) {
                    current_package_wait_response->callback(SEND_FAIL);
                }
            } else {
                LOG(LEVEL_INFO,"response crc err resend\r\n");

                current_package_wait_response->resendCount++;
                err_code = L1_resend_package(current_package_wait_response);
                if(err_code != NRF_SUCCESS) {
                    sendPara.callback = NULL;
                    sendPara.context = NULL;
                    sendPara.task_type = TASK_NONE;
                    set_complete_callback(sendPara); //cancle callback

                    current_package_wait_response->isUsed = 0;
                    if(current_package_wait_response->callback) {
                        current_package_wait_response->callback(SEND_FAIL);
                    }
                }
            }
        }
    }
    else {
        LOG(LEVEL_INFO,"receive a package with wrong sequesnce id\r\n");
    }
}



/**********************************************************************
* Schedule next package to be send after prev package send success
***********************************************************************/
void schedule_async_send(void * contenxt,SEND_TASK_TYPE_T task)
{
    /***********************************************************************
    * Call this function shows that last package send success
    *************************************************************************/
    ASSERT(contenxt != NULL);

    if(task == TASK_DATA) { //may be comes from data (send & resend) or (send complete callback)
        L1_Send_Content * data_content = (L1_Send_Content *)contenxt;

        if(data_content->contentLeft != 0) {
            app_timer_stop(delay_send_wait_timer);
        }
        async_send(&data_content,sizeof(L1_Send_Content *),TASK_DATA);
    } else if (task == TASK_ACK) {
        struct Response_Buff_Type_t * ack_content = (struct Response_Buff_Type_t *)contenxt;
        async_send(&ack_content,sizeof(struct Response_Buff_Type_t *),TASK_ACK);

    } else {//task none
        LOG(LEVEL_INFO,"call schedule_async_send with wrong para\r\n");

    }
}

/****************************************************************************
* new_task_type shows where call this function
*****************************************************************************/
void async_send(void * para,uint16_t event_size,SEND_TASK_TYPE_T new_task_type)
{

    ASSERT(para != NULL);
    ASSERT(event_size == sizeof(void*));

    if(current_task_type == TASK_NONE) {
        current_task_type = new_task_type;
    } else {
        if((current_task_type == TASK_ACK) && (new_task_type == TASK_DATA) && (L1_header_need_schedule.isUsed == 1)) {//get a data send request while no buffer to send ack
            g_next_L1_send_content_buffer = *((L1_Send_Content **)para);
            next_task_type = TASK_DATA;
            return;
        } else if((current_task_type == TASK_DATA) && (new_task_type == TASK_ACK)) {
            next_task_type = TASK_ACK;
            return;
        }
    }

    uint32_t error_code;
    uint16_t sendLen = 0;
    uint8_t * currentSendPointer = NULL;
    SendCompletePara sendPara;
    L1_Send_Content * content = NULL;


    error_code = NRF_SUCCESS;

LABEL_SEND_ACK:
    if(current_task_type == TASK_ACK) {

        if(g_ack_package_buffer.isUsed == 1) {

            currentSendPointer = (uint8_t *)construct_response_package(g_ack_package_buffer.sequence_id,(g_ack_package_buffer.check_success == 1) ?true:false );
            sendLen = L1_HEADER_SIZE;

            error_code = ble_nus_send_string(&m_nus,currentSendPointer,&sendLen);

            if(error_code == NRF_SUCCESS) {
                //set task content
                current_task_type = TASK_NONE;
                g_ack_package_buffer.isUsed = 0;
                if((next_task_type == TASK_DATA) && (g_next_L1_send_content_buffer != NULL)) {
                    current_task_type = TASK_DATA;
                    next_task_type = TASK_NONE;
                    content = g_next_L1_send_content_buffer;
                    goto SEND_DATA_LABLE; //FIXME://////////////
                }
                return;

            } else if (error_code == BLE_ERROR_NO_TX_BUFFERS) {
                sendPara.callback = schedule_async_send;
                sendPara.context = &g_ack_package_buffer;
                sendPara.task_type = TASK_ACK;
                set_complete_callback(sendPara);

            } else {
                //丢弃
                current_task_type = TASK_NONE;
                next_task_type = TASK_NONE;
                g_ack_package_buffer.isUsed= 0;
                if((next_task_type == TASK_DATA) && (g_next_L1_send_content_buffer != NULL)) {
                    current_task_type = TASK_DATA;
                    next_task_type = TASK_NONE;
                    content = g_next_L1_send_content_buffer;
                    goto SEND_DATA_LABLE; //FIXME://////////////
                }
            }

        }
        return;
    }

    if(current_task_type == TASK_NONE) {
        return; //error condition
    }

    content =  *((L1_Send_Content **)para);

    //error status
    ASSERT(content != NULL);

SEND_DATA_LABLE:
    error_code = NRF_SUCCESS;
    SEND_CONTENT_TYPE_T sendContentType = CONTENT_NONE;
    while(error_code == NRF_SUCCESS) { // send will continue before buffer was full
        /***********************************************************
        * Order :header content ack
        ************************************************************/
        //first need to check header send request
        if(L1_header_need_schedule.isUsed == 1) {

            currentSendPointer = global_L1_header_buffer;
            sendLen = L1_HEADER_SIZE;
            sendContentType = CONTENT_HEADER;

        } else {
            //check other content

            if(content ->contentLeft != 0) {

                ASSERT(content->content != NULL);
                sendLen = ( content->contentLeft > NUS_MAX_DATA_LENGTH ) ? NUS_MAX_DATA_LENGTH : (content->contentLeft);
                currentSendPointer = (content->content + (content->length - content->contentLeft));
                sendContentType = CONTENT_DATA;

            } else {
                sendContentType = CONTENT_NONE;
            }
        }


        //first check if data is send complete
        if(sendContentType == CONTENT_NONE) {
            break; //which means send data complete
        }

        error_code = ble_nus_send_string(&m_nus,currentSendPointer,&sendLen);

        if(error_code == NRF_SUCCESS) {
            //do flag
            switch(sendContentType) {
                case CONTENT_NONE:
                    break;
                case CONTENT_HEADER: //mark header send complete
                    if(L1_header_need_schedule.isUsed == 1) {

                        L1_header_need_schedule.isUsed = 0;
                        memset(global_L1_header_buffer,0,L1_HEADER_SIZE);
                    }
                    break;
                case CONTENT_DATA:
                    content ->contentLeft -= sendLen;
                    if(content ->contentLeft == 0) { //send complete & will register wait response
                        
                        sendPara.callback = NULL;
                        sendPara.context = NULL;
                        sendPara.task_type = TASK_NONE;
                        set_complete_callback(sendPara);
                        //set task content
                        current_task_type = TASK_NONE;
                        //begin to wait package response
                        register_wait_response(content);
                        //start timer wait for response
                        app_timer_start(delay_send_wait_timer,RESEND_DELAY,(void *)content);
                        if((next_task_type == TASK_ACK) && (g_ack_package_buffer.isUsed == 1)) {
                            current_task_type = TASK_ACK;
                            next_task_type = TASK_NONE;
                            goto LABEL_SEND_ACK;
                        }
                    }
                    break;
                case CONTENT_ACK:
                    if(g_ack_package_buffer.isUsed == 1) { //send ack package
                        //set task content
                        current_task_type = TASK_NONE;
                        g_ack_package_buffer.isUsed = 0;
                    }
                    break;
                default:
                    break;
            }

        } else if (error_code == BLE_ERROR_NO_TX_BUFFERS) {
            sendPara.callback = schedule_async_send;
            sendPara.context = content;
            sendPara.task_type = TASK_DATA;
            set_complete_callback(sendPara);
            //start timer wait for response
            app_timer_start(delay_send_wait_timer,RESEND_DELAY,(void *)content);
            break; //wait to reschedule
        } else {
            //send fail
            sendPara.callback = NULL;
            sendPara.context = NULL;
            sendPara.task_type = TASK_NONE;
            set_complete_callback(sendPara);
            //set task content
            current_task_type = TASK_NONE;

            if(content->callback) {
                content->isUsed = 0;
                content->callback(SEND_FAIL);
            }

            if(next_task_type == TASK_ACK) {
                current_task_type =  TASK_ACK;
                next_task_type = TASK_NONE;
                goto LABEL_SEND_ACK;
            } else {
                return;
            }
        }
    
    }

}

/*************************************************************
 * L1 send content implementation
 * para description:
 *  content->content
 * content->callback
 *  content->length
**************************************************************/

uint32_t L1_send(L2_Send_Content * content)
{
    uint32_t err_code;

    if(!content) {
        return NRF_ERROR_INVALID_PARAM;
    }

    uint32_t i = 0;

    err_code = NRF_ERROR_NO_MEM;
    for( i = 0; i<MAX_SEND_TASK ; ++i ) {

        if( sendContent[i].isUsed ) {
            continue;
        } else {
            sendContent[i].isUsed = 1;
            err_code = 0;
            break;
        }

    }

    if(err_code == NRF_ERROR_NO_MEM) {
        return NRF_ERROR_NO_MEM;
    }

    /*fill header*/
    global_L1_header_buffer[L1_HEADER_MAGIC_POS] = L1_HEADER_MAGIC;           /* Magic */
    global_L1_header_buffer[L1_HEADER_PROTOCOL_VERSION_POS] = L1_HEADER_VERSION;       /* protocol version */
    global_L1_header_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] = (content->length >> 8 & 0xFF);    /* length high byte */
    global_L1_header_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] = (content->length & 0xFF);      /* length low byte */
    /*cal crc*/
    uint16_t crc16_ret = bd_crc16(0,content->content,content->length);
    global_L1_header_buffer[L1_HEADER_CRC16_HIGH_BYTE_POS] = ( crc16_ret >> 8) & 0xff;
    global_L1_header_buffer[L1_HEADER_CRC16_LOW_BYTE_POS] = crc16_ret & 0xff;

    //sequence id
    global_L1_header_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] = (L1_sequence_id >> 8) & 0xff;
    global_L1_header_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] = L1_sequence_id & 0xff;

    //prepare for send L2 content
    sendContent[i].callback  =  content->callback;
    sendContent[i].content   =  content->content;
    sendContent[i].length  = content->length;
    sendContent[i].contentLeft  = content->length;
    sendContent[i].resendCount = 0;
    sendContent[i].sequence_id = L1_sequence_id;

    //every time send a package,increase L1_sequence_id, whether it's success or not
    L1_sequence_id ++;

    //register need to schedule header
    L1_header_need_schedule.isUsed = 1;
    L1_header_need_schedule.content = &sendContent[i];

    //schedule async send
    schedule_async_send(&sendContent[i],TASK_DATA);

    return NRF_SUCCESS;

}


/***********************************************************************
* Debug callback, & will be removede in the near future
************************************************************************/
void send_status_callback(SEND_STATUS status )
{
    if(status == SEND_SUCCESS) {

#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"send success\r\n");
        //    simple_uart_putstring((const uint8_t *)"send success\r\n");
#endif

    } else {
#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"send fail \r\n");
        //    simple_uart_putstring((const uint8_t *)"send fail \r\n");
#endif

    }
}

extern uint32_t user_id[];

/***********************************************************************
* L2 command resolve health data function
************************************************************************/
static uint32_t  resolve_HealthData_command(uint8_t key,const uint8_t * value ,uint16_t length)
{
    uint32_t err_code = NRF_SUCCESS;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"key %d:len:%d - %d",key,length,value[0]);
    //    char str[32];
    //    sprintf(str,"key %d:len:%d - %d\t",key,length,value[0]);
    //    simple_uart_putstring((const uint8_t *)str);
#endif

    switch(key) {
        case 0:
            if(length == 0) {
                send_all_data(false);
            }
            break;
        case KEY_REQUEST_DATA: //return sports data
            if(length == 0) {

#ifdef DEBUG_LOG
                LOG(LEVEL_INFO,"KEY_REQUEST_DATA\n");
                //   char str[32];
                //   sprintf(str,"key %d:len:%d - %d\t",key,length,value[0]);
                //   simple_uart_putstring((const uint8_t *)str);
#endif
                send_all_data(false);
            }
            break;
        case KEY_SET_STEPS_NOTIFY: //set auto notify
            if(length == 1) {
#ifdef DEBUG_LOG
                LOG(LEVEL_INFO,"KEY_SET_STEPS_NOTIFY value:%d\n",value[0]);
                //   char str[32];
                //   sprintf(str,"key %d:len:%d - %d\t",key,length,value[0]);
                //   simple_uart_putstring((const uint8_t *)str);
#endif


                if(value[0] == 1) {
                    notify_steps_enable = true;
                    send_all_data(false);
                } else {
                    notify_steps_enable = false;
                }
            }
            break;
            
            case KEY_DAILY_DATA_SYNC:
                if(length == 12) {
                    uint32_t daily_step = 0;
                    uint32_t daily_distance = 0;
                    uint32_t daily_calory = 0;
                    
                    daily_step |= value[3];
                    daily_step |= value[2] << 8;
                    daily_step |= value[1] << 16;
                    daily_step |= value[0] << 24;
            
            
                    daily_distance |= value[3 + 4];
                    daily_distance |= value[2 + 4] << 8;
                    daily_distance |= value[1 + 4] << 16;
                    daily_distance |= value[0 + 4] << 24;
            
            
                    daily_calory |= value[3 + 8];
                    daily_calory |= value[2 + 8] << 8;
                    daily_calory |= value[1 + 8] << 16;
                    daily_calory |= value[0 + 8] << 24;
            
                    if(daily_step != get_global_step_counts_today()) {
                        set_global_step_counts_today(daily_step);
//                        set_global_calories_today(daily_calory*10);
                    }
                    
                }
            
                break;
            case KEY_LATEST_DATA_SYNC:
                if(length == 10){
                    uint8_t mode = 0;
                    uint8_t active_time = 0;
                    uint32_t calories = 0;                
                    uint16_t steps = 0;
                    uint16_t distance = 0;
                    
                    mode = value[0];
                    active_time = value[1];
                    calories |= value[2 + 0] << 24;
                    calories |= value[2 + 1] << 16;
                    calories |= value[2 + 2] << 8;
                    calories |= value[2 + 3];
            
                    steps |= value[6 + 0] << 8;
                    steps |= value[6 + 1];
            
                    distance |= value[8 + 0] << 8;
                    distance |= value[8 + 1];
                    
                    set_quarter_steps(steps);
                    set_quarter_distance(distance*10000);
                    LOG(LEVEL_INFO,"calories:    0x%x",calories);
                    set_quarter_calories(calories*10);
                    set_quater_sport_mode(mode);
                    set_quater_active_time(active_time);
                    
                }
                break;

        default:
            err_code = NRF_ERROR_INVALID_PARAM;

    }

    return err_code;


}


/***********************************************************************
* L2 command resolve function
************************************************************************/
static uint32_t  resolve_notify_command(uint8_t key,const uint8_t * value ,uint16_t length)
{
    uint32_t err_code = NRF_SUCCESS;


    switch(key) {
        case 0x01: //phone call
            notification_start(NOTIFICATION_CALLING,0);
#ifdef DEBUG_LOG

//            LOG(LEVEL_INFO,"----------PHONE CALLED\r\n");
            //        simple_uart_putstring("----------PHONE CALLED\r\n");
#endif
            //hal_vibrate_once(1,1);
            //hal_vibrate_once(1,1);
            //hal_vibrate_once(1,1);
            break;
        case 0x02: //answered
            motor_action_control_stop();
            led_action_stop();

            break;
        case 0x03: //rejected
            motor_action_control_stop();
            led_action_stop();

            break;
        default:
            err_code = NRF_ERROR_INVALID_PARAM;

    }

#ifdef DEBUG_LOG
//    LOG(LEVEL_INFO,"notify: key %d:len:%d -v: %d",key,length,value[0]);
    //     char str[32];
    //     sprintf(str,"notify: key %d:len:%d -v: %d\t",key,length,value[0]);
    //     simple_uart_putstring((const uint8_t *)str);
#endif

    return err_code;

}
/************************************************************************
* resolve_settings_config_command
*************************************************************************/
static uint32_t  resolve_settings_config_command(uint8_t key,const uint8_t * value ,uint16_t length)
{
    uint32_t err_code = NRF_SUCCESS;
    switch(key) {
        case KEY_TIME_SETTINGS: //set wall clock timer
            if(length == 0x04 ) {
                time_union_t time;
                UTCTimeStruct * tm = get_wall_clock_time();
                time.data = 0;
                time.data |= value[3];
                time.data |= value[2] << 8;
                time.data |= value[1] << 16;
                time.data |= value[0] << 24;
                
                //if set time pass a day,reset step count
                if(tm->day != time.time.day) {                    
                    set_global_step_counts_today(0);
                    set_daily_target_achieved(false);
                }

                set_system_clock(time);

                if(check_has_bonded()&& false == is_algorithm_started()) { // restart case
                    restart_health_algorithm();
                }
            } else {
                err_code = NRF_ERROR_INVALID_LENGTH;
            }
            break;
        case KEY_ALARM_SETTINGS: //set alarm clock
            if(length%5 == 0 ) {
                uint8_t index;
                uint64_t alarmData;
                uint8_t num;
                alarm_union_t alarm;


                num = length/5;
                if(num > MAX_ALARM_NUM) {
                    num = MAX_ALARM_NUM;
                }
                set_alarm_number(num);
                for(index = 0; index < num; index ++) {
                    alarmData = value[0 + index * 5];
                    alarm.data = alarmData << 32;
                    alarmData = value[1 + index * 5];
                    alarm.data |= alarmData << 24;
                    alarmData = value[2 + index * 5];
                    alarm.data |= alarmData << 16;
                    alarmData = value[3 + index * 5];
                    alarm.data |= alarmData << 8;
                    alarmData = value[4 + index * 5];
                    alarm.data |= alarmData;
                    set_alarm(index,&alarm);
                }

                save_alarm();


            } else {
                err_code = NRF_ERROR_INVALID_LENGTH;
            }

            break;
        case KEY_REQUEST_ALARM_SETTINGS: //request alarm clock list
            if(length == 0)
                return_alarm_list();
            else {
                err_code = NRF_ERROR_INVALID_LENGTH;
            }
            break;
        case KEY_STEP_TARGET_SETTINGS:
            if(length == 4) {
                uint32_t target  = 0;
                target |= value[3];
                target |= value[2] << 8;
                target |= value[1] << 16;
                target |= value[0] << 24;

                if(target == 0)
                    target = DEFAULT_STEP_TASK;

                set_daily_target(target);
                save_daily_target();

            } else {
                err_code = NRF_ERROR_INVALID_LENGTH;
            }
            break;
        case KEY_PROFILE_SETTINGS: //set user profile
            if(length == 4) {
                userprofile_union_t profile;
                profile.data = 0;
                profile.data |= value[3];
                profile.data |= value[2] << 8;
                profile.data |= value[1] << 16;
                profile.data |= value[0] << 24;

                set_user_profile(&profile);
                save_user_profile();
                if(is_systme_clock_valid()) {
                    restart_health_algorithm();
                }
            } else {
                err_code = NRF_ERROR_INVALID_LENGTH;
            }


            break;
        case KEY_DEV_LOSS_ALERT_SETTINGS: //alert level
            if(length == 1) {
                uint8_t alert_level = value[0] & 0x0F; //low byte
                if(alert_level <= (uint8_t)HIGH_ALERT) {
                    set_global_dev_loss_alert_level((DEV_LOSS_ALERT_LEVEL)alert_level);

                    if(alert_level == NO_ALERT) {
                        set_global_dev_loss_should_alert(0);
                    } else {
                        set_global_dev_loss_should_alert(1);
                    }
                }
            }
            break;
        default:
            err_code = NRF_ERROR_INVALID_PARAM;

    }
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"SETTINGS---key %d:len:%d - %d\n",key,length,value[0]);
    //    char str[32];
    //    sprintf(str,"SETTINGS---key %d:len:%d - %d\t",key,length,value[0]);
    //    simple_uart_putstring((const uint8_t *)str);
#endif

    return err_code;

}

/************************************************************************
* Thins function is used to notify that a new bond occure
*************************************************************************/
void bond_success_event_observer(void)
{
    set_device_has_bonded(true);

    //when a new bond created , device loss alert should not work
    set_global_dev_loss_controller_data(0);

    set_global_step_counts_today(0);

    stop_health_algorithm();

    reset_user_data_for_new_bond();

    reset_health_algorithm_data();
}


/************************************************************************
* resolve bond command
* 区分绑定和登录两个命令，最主要是登录命令是不应该提示用户敲击手环
*************************************************************************/
static uint32_t  resolve_private_bond_command(uint8_t key,const uint8_t * value ,uint16_t length)
{
    uint32_t err_code = NRF_SUCCESS;

    if( (key == 0x01) || (key == 0x03)) { //receive bond or login command
        app_timer_stop(user_action_delay_timer); //stop wait bond command timer
    }
#ifdef DEBUG_LOG
    {
//        LOG(LEVEL_INFO,"resolve_private_bond_command:key[%d]\n",key);
        //   char str[128];
        //   sprintf(str,"resolve_private_bond_command:key[%d]\r\n",key);
        // simple_uart_putstring((const uint8_t *)str);
        //    sprintf(str,"value:[%s],length[%d]\r\n",value,length);
        //    simple_uart_putstring((const uint8_t *)str);
    }
#endif
    switch(key) {
        case 0x01: //bond request
            /* schdule proper task */
            if((length == USER_ID_LENGTH) && value) {

                uint8_t batt = get_battery_power_percentage();

                if( batt < PRIVATE_BOND_POWER_LIMIT ){
#ifdef DEBUG_LOG
                    LOG(LEVEL_INFO,"Battery Level is too Low for BONDING(%d)\n",batt);
#endif                
                    bond_fail_event_dispatch();
                    break;
                }

                memcpy((uint8_t*)user_id,value,USER_ID_LENGTH);
                //                                global_short_press_action = INPUT_ACCEPT_BOND; //wait knock event to accept bond
                app_timer_start(user_action_delay_timer,USER_ACTION_TIMEOUT,(void *)DO_BOND);
                notification_start(NOTIFICATION_BONDING,0);
                //        led_action_control(NOTIFICATION_BONDING,SERIAL_FLASH,1000); //set a big loop time as flash entil time out

            } else {
                bond_fail_event_dispatch();
                //
            }
            break;
        case 0x03: //login request
            if((length == USER_ID_LENGTH) && value) {

                err_code = check_user_id_bonded(value,USER_ID_LENGTH);
                if(err_code == NRF_SUCCESS) {
                    if(sleep_setting_count() == 0){// resend sleeping status if apk was reinstalled
                        send_last_time_sleep_mode();
                    }

                    login_success_event_dispatch();
                    //change bond status machine
                    private_bond_machine =PRIVATE_BOND_SUCCESS;
                    break;
                }

            }

            //code comes here reflects login fail
            login_fail_event_dispatch();
            //login fail so restart wait bond command
            app_timer_start(user_action_delay_timer,WAIT_BOND_COMMAND_TIMEOUT,(void *)DO_WAIT_BOND_COMMAND);
            break;

    }
    return err_code;

}

uint32_t enter_ota_status_response(uint8_t status_code ,uint8_t err_code)
{
    L2_Send_Content sendContent;

    global_reponse_buffer[0] = FIRMWARE_UPDATE_CMD_ID;                        /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;                             /*L2 header version */
    global_reponse_buffer[2] = KEY_ENTER_DFU_MODE_RET;                        /*first key */
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = 2;                                             /* length  = 2 */

    global_reponse_buffer[5] = status_code;
    global_reponse_buffer[6] = err_code;

    sendContent.callback    = NULL;       //FIXME: should add send fail resend
    sendContent.content     = global_reponse_buffer;
    sendContent.length      = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4];
    return L1_send(&sendContent);

}

static void resolve_firmware_update_command(FIRMWARE_UPDATE_KEY key)
{
    switch (key) {
        case KEY_ENTER_FIRMWARE_UPDATE_MODE: //begin to OTA
            {
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO,"received ota command\n");
            //     simple_uart_putstring((const uint8_t *)"received ota command\r\n");
#endif  
            bool origin_sending_flag = get_data_sending_flag();
            set_data_sending_flag(true); //stop data sync,put stop here for: we have chance to resend enter ota response

            uint8_t batt = get_battery_power_percentage();
            if( batt < FIRMWARE_UPDATE_POWER_LIMIT ){
#ifdef DEBUG_LOG
            LOG(LEVEL_INFO,"Battery Level is too Low for OTA(%d)\n",batt);
#endif                
                enter_ota_status_response(0x01,0x01);   //ota fail ,low power
                set_data_sending_flag(origin_sending_flag); //revert sending flag
                break;
            }
                enter_ota_status_response(0x00,0x00); //enter ota mode success
                //set flags to trigger ota mode
                set_global_should_trigger_ota_flag(true);
                app_timer_start(ota_delay_timer,OTA_DELAY_TIMEOUT,NULL);
            }
            break;
        case KEY_GET_FIRMWARE_VERSION_INFO: //get firm ware version info
            global_reponse_buffer[0] = FIRMWARE_UPDATE_CMD_ID;                        /*command id*/
            global_reponse_buffer[1] = L2_HEADER_VERSION;                               /*L2 header version */
            global_reponse_buffer[2] = KEY_RET_FIRMWARE_VERSION_INFO;         /*first key, bond response*/
            global_reponse_buffer[3] = 0;
            global_reponse_buffer[4] = 4;                                   /* length  = 4 */
            //FIXME: need to def version information
            global_reponse_buffer[5] = 0x01;                     /* value */
            global_reponse_buffer[6] = 0x01;
            global_reponse_buffer[7] = 0x01;
            global_reponse_buffer[8] = 0x01;

            L2_Send_Content sendContent;
            sendContent.callback    = NULL;
            sendContent.content     = global_reponse_buffer;
            sendContent.length      = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4];
            L1_send(&sendContent);
            break;
        default:
            break;
    }
}
#ifdef TESTFLASH
static uint32_t resolve_test_flash_command(uint8_t key,const uint8_t * value ,uint16_t length)
{
    uint32_t err_code = NRF_SUCCESS;


    switch(key) {
        case 0x01: // save one group
            test_group_data_write(1);
            break;
        case 0x02: // return all data
            send_all_data();
            break;
        case 0x03: //rejected
            test_flash_send_2_group();
            break;
        case 0x04:
        default:
            err_code = NRF_ERROR_INVALID_PARAM;

    }
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"notify: key %d:len:%d -v: %d\n",key,length,value[0]);
#endif

    return err_code;

}
#endif
/***********************************************************************
* para introduction
* data                                   :      just the full of L2
* content_length :      length of data
* resolve_state  :  L1 receive data length
************************************************************************/
static uint32_t L2_frame_resolve(uint8_t * data, uint16_t length,RECEIVE_STATE * resolve_state)
{
    //para check
    if((!data) || (length == 0)) {
        return NRF_ERROR_INVALID_PARAM;
    }

    BLUETOOTH_COMMUNICATE_COMMAND command_id;
    uint8_t version_num;                                            /* L2 version number */
    uint8_t first_key;                                                      /* first key of L2 payload*/
    uint16_t first_value_length;            /* length of first value */

    command_id      = (BLUETOOTH_COMMUNICATE_COMMAND)data[0];
    version_num = data[1];
    version_num = version_num;                      /*current not use it*/
#ifdef DEBUG_LOG

    {

        //  char str[64];
        if(private_bond_machine==PRIVATE_NOT_BOND)
        {
            LOG(LEVEL_INFO,"PRIVATE_NOT_BOND\n");
            //    sprintf(str,"PRIVATE_NOT_BOND\r\n");
        } else if(private_bond_machine==PRIVATE_BOND_SUCCESS)
        {
            LOG(LEVEL_INFO,"PRIVATE_BOND_SUCCESS\n");
            //    sprintf(str,"PRIVATE_BOND_SUCCESS\r\n");
        }
        // simple_uart_putstring((const uint8_t *)str);
        //    sprintf(str,"command_id:%d\r\n",command_id);
        //    simple_uart_putstring((const uint8_t *)str);
        LOG(LEVEL_INFO,"command_id:%d\n",command_id);
    }
#endif
    switch (private_bond_machine) {
        case PRIVATE_NOT_BOND: { //wait bond
                if(command_id == BOND_COMMAND_ID) {
                    first_key = data[2];
                    first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);

                    resolve_private_bond_command(first_key,data+ L2_FIRST_VALUE_POS,first_value_length);

                } else if (FACTORY_TEST_COMMAND_ID == command_id) {
                    uint16_t offset = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE;
                    //     uint16_t payload = 0;
                    uint16_t v_length = 0;
                    uint8_t *key = data + L2_HEADER_SIZE;
                    while(length - offset >= 0) {
                        v_length = ((*(key + 1)) << 8) + (*(key + 2));

                        do_test(key, v_length);
                        key += (v_length + L2_PAYLOAD_HEADER_SIZE);
                        offset = offset + v_length + L2_PAYLOAD_HEADER_SIZE;
                    }
                } else if(command_id == FIRMWARE_UPDATE_CMD_ID) {
                    first_key = data[2];
                    resolve_firmware_update_command((FIRMWARE_UPDATE_KEY)first_key);
                } else if(command_id == GET_STACK_DUMP) {
                    first_key = data[2];
                    first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);
                    get_stack_dump_command_resolve(first_key);
                } else if(command_id == BLUETOOTH_LOG_COMMAND_ID) {
                    first_key = data[2];
                    resolve_log_command_id((log_command_key_t)first_key);
                }
            }
            break;
        case PRIVATE_BOND_SUCCESS: { //bond success

                //resolve other command
                switch (command_id) {
                    case FIRMWARE_UPDATE_CMD_ID:
                        first_key = data[2];
                        resolve_firmware_update_command((FIRMWARE_UPDATE_KEY)first_key);
                        break;
                        /****************************************************************************************************
                        //should not resolve bond command
                        case BOND_COMMAND_ID:
                                           first_key = data[2];
                                           first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);

                                           resolve_private_bond_command(first_key,data+ L2_FIRST_VALUE_POS,first_value_length);
                                           break;
                        *********************************************************************************************************/
                    case SET_CONFIG_COMMAND_ID:

                        first_key = data[2];
                        first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);
                        // here not handle the ret value
                        resolve_settings_config_command(first_key,data+ L2_FIRST_VALUE_POS,first_value_length);
                        break;
                    case NOTIFY_COMMAND_ID:
                        first_key = data[2];
                        first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);
                        // here not handle the ret value
                        resolve_notify_command(first_key,data+ L2_FIRST_VALUE_POS,first_value_length);
                        break;
#ifdef TESTFLASH

                    case TEST_FLASH_READ_WRITE:
                        first_key = data[2];
                        first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);

                        resolve_test_flash_command(first_key,data+ L2_FIRST_VALUE_POS,first_value_length);
                        break;
#endif

                    case HEALTH_DATA_COMMAND_ID:
#ifdef DEBUG_LOG

                        {
                            LOG(LEVEL_INFO,"len:%d - %d:%d:%d \n",length,data[4],data[5],data[6]);
                            //    char str[32];
                            //    sprintf(str,"len:%d - %d:%d:%d \t",length,data[4],data[5],data[6]);
                            //     simple_uart_putstring((const uint8_t *)str);
                        }
#endif
                        first_key = data[2];
                        first_value_length = (((data[3]<< 8) |data[4]) & 0x1FF);
                        resolve_HealthData_command(first_key,data+ L2_FIRST_VALUE_POS,first_value_length);
                        break;
                    case BLUETOOTH_LOG_COMMAND_ID:
                        first_key = data[2];
                        resolve_log_command_id((log_command_key_t)first_key);
                        break;
                    case TEST_COMMAND_ID:
                        if(length == 1) {
                            switch (data[3]) {
                                case 1:
                                    led_action_control(NOTIFICATION_TEST,SERIAL_FLASH,1);
                                    break;
                                case 2:
                                    led_action_control(NOTIFICATION_TEST,SERIAL_CLOSE,1);
                                    break;
                                case 3:
                                    led_action_control(NOTIFICATION_TEST,CELEBRATE,1);
                                    break;
                                default:
                                    break;
                            }

                        }
                        break;
                        /*********************************************************************************************
                         //should not resolve factory test command
                          case FACTORY_TEST_COMMAND_ID:
                          {
                           uint16_t offset = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE;
                           //     uint16_t payload = 0;
                           uint16_t v_length = 0;
                           uint8_t *key = data + L2_HEADER_SIZE;
                           while(length - offset >= 0){
                            v_length = ((*(key + 1)) << 8) + (*(key + 2));

                            do_test(key, v_length);
                            key += (v_length + L2_PAYLOAD_HEADER_SIZE);
                            offset = offset + v_length + L2_PAYLOAD_HEADER_SIZE;
                           }
                          }
                          break;
                         ************************************************************************************************/
                    default:
                        break;
                }
            }
            break;
    }


    /*resolve complete and restart receive*/
    *resolve_state = WAIT_START;
    return NRF_SUCCESS;

}

/******************************************************************************
* direct send response package without any sync op
*******************************************************************************/
static L1Header_t* construct_response_package(uint16_t sequence_id, bool check_success)
{
    static L1Header_t response_header;
    L1_version_value_t version_ack;


    response_header.magic = L1_HEADER_MAGIC;

    version_ack.version_def.version = L2_HEADER_VERSION;
    version_ack.version_def.ack_flag = 1;
    version_ack.version_def.err_flag = (check_success ? 0 : 1);
    version_ack.version_def.reserve = 0;

    response_header.version =  version_ack.value;
    response_header.payload_len = 0;
    response_header.crc16 = 0;
    response_header.sequence_id = ((sequence_id & 0xFF) << 8) | ((sequence_id >> 8) & 0xFF); //big engian

    return &response_header;
}

/*************************************************************************
* L1 receive a package and will send a response
* para des:
*               sequence_id : the received sequence id
*               check_success: crc check result
**************************************************************************/
uint32_t L1_receive_response(uint16_t sequence_id, bool check_success)
{
    //just use the new response request update the older one
    g_ack_package_buffer.check_success = (check_success == true) ? 1 :0;
    g_ack_package_buffer.sequence_id = sequence_id;
    g_ack_package_buffer.isUsed = 1;


    schedule_async_send(&g_ack_package_buffer,TASK_ACK);
    return NRF_SUCCESS;
}


/*************************************************************************
* receive bad package
* This function used to schedule crc error callback
**************************************************************************/
void schedule_crc_error_handle(void * para,uint16_t event_size)
{
    para = para;
    event_size = event_size;

    LOG(LEVEL_INFO,"received data crc check error\n");
}

/*************************************************************************
* check the crc16 value for the received package
**************************************************************************/
static uint32_t L1_crc_check(uint16_t crc_value,uint8_t *data,uint16_t length)
{
    uint16_t crc = bd_crc16(0x0000,data,length);
    if(crc == crc_value) {
        return NRF_SUCCESS;
    }

    return NRF_ERROR_INVALID_DATA;

}

static RECEIVE_STATE receive_state = WAIT_START;
static uint8_t received_buffer[GLOBAL_RECEIVE_BUFFER_SIZE];
static uint16_t received_content_length = 0;
static int16_t length_to_receive;
//////////////////////////////////////////////////////////////////////////
/*****************************************************************************
* received content
*****************************************************************************/
void L1_receive_data(ble_nus_t * p_nus, uint8_t * data, uint16_t length)
{

    if(app_query_timer(receive_time_out_timer) == NRF_SUCCESS) {
        app_timer_stop(receive_time_out_timer);
    }

    L1_version_value_t inner_version;

    switch (receive_state) {
        case WAIT_START: // we need package start
            if(data[0] != L1_HEADER_MAGIC) {
                //not a start package, so just igore
                break;
            }
            //get correct header
            received_content_length = 0;
            memcpy(&received_buffer[received_content_length],data,length);
            received_content_length = length;

            length_to_receive = (received_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] | (received_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] << 8)) + L1_HEADER_SIZE;
            length_to_receive -= length;

            if(length_to_receive <= 0) { // just one package


                inner_version.value = received_buffer[L1_HEADER_PROTOCOL_VERSION_POS];

                if(inner_version.version_def.ack_flag == RESPONSE_PACKAGE) { //response package

                    LOG(LEVEL_INFO,"receive a ack package\n");

                    receive_state = WAIT_START; //restart receive state machine
                    response_package_handle((received_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (received_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),inner_version.version_def.err_flag);
                    return;
                }

                //data package
                receive_state = MESSAGE_RESOLVE;
                received_content_length = 0;

                uint16_t crc16_value = (received_buffer[L1_HEADER_CRC16_HIGH_BYTE_POS] << 8 | received_buffer[L1_HEADER_CRC16_LOW_BYTE_POS]);
                if(L1_crc_check(crc16_value,received_buffer+L1_HEADER_SIZE,(received_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] | (received_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] << 8))) == NRF_SUCCESS) { //check crc for received package
                    //LOG(LEVEL_INFO,"will send success response\n");
                    //send response
                    L1_receive_response((received_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (received_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),true);
                    /*throw data to uppder layer*/
                    L2_frame_resolve(received_buffer+L1_HEADER_SIZE,(received_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] | (received_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] << 8)),&receive_state);
                } else { //receive bad package
                    //restart receive state machine
                    receive_state = WAIT_START;

                    LOG(LEVEL_INFO,"will send crc fail response\n");
                    //send response
                    L1_receive_response((received_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (received_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),false);
                    //schedule error handler
                    app_sched_event_put(NULL,0, schedule_crc_error_handle);
                    return;
                }

            } else { // more than one package

                receive_state = WAIT_MESSAGE;

                app_timer_start(receive_time_out_timer,RECEIVE_TIMEOUT,&receive_state);
            }
            break;
        case WAIT_MESSAGE:
            memcpy(&received_buffer[received_content_length],data,length);
            received_content_length += length;
            length_to_receive -= length;

            if(length_to_receive <= 0) {

                /* Stop timer */
                inner_version.value = received_buffer[L1_HEADER_PROTOCOL_VERSION_POS];

                if(inner_version.version_def.ack_flag == RESPONSE_PACKAGE) { //response package
                    receive_state = WAIT_START; //restart receive state machine
                    response_package_handle((received_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (received_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),inner_version.version_def.err_flag);
                    return;
                }

                receive_state = MESSAGE_RESOLVE;
                received_content_length = 0;

                uint16_t crc16_value = (received_buffer[L1_HEADER_CRC16_HIGH_BYTE_POS] << 8 | received_buffer[L1_HEADER_CRC16_LOW_BYTE_POS]);
                if(L1_crc_check(crc16_value,received_buffer+L1_HEADER_SIZE,(received_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] | (received_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] << 8))) == NRF_SUCCESS) { //check crc for received package
                    LOG(LEVEL_INFO,"will send success response\n");
                    //send response
                    L1_receive_response((received_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (received_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),true);
                    /*throw data to uppder layer*/
                    L2_frame_resolve(received_buffer+L1_HEADER_SIZE,(received_buffer[L1_PAYLOAD_LENGTH_LOW_BYTE_POS] | (received_buffer[L1_PAYLOAD_LENGTH_HIGH_BYTE_POS] << 8)),&receive_state);
                } else { //receive bad package
                    //restart receive state machine
                    receive_state = WAIT_START;

                    LOG(LEVEL_INFO,"will send crc fail response\n");
                    //send response
                    L1_receive_response((received_buffer[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (received_buffer[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),false);
                    //schedule error handler
                    app_sched_event_put(NULL,0, schedule_crc_error_handle);
                    return;
                }

            } else {
                /* start receive time out timer */
                app_timer_start(receive_time_out_timer,RECEIVE_TIMEOUT,&receive_state);
            }

            break;

        case MESSAGE_RESOLVE:
            //in this situation , can only receive a ack package
            //Note: ack package must small than 20 bytes

            inner_version.value = data[L1_HEADER_PROTOCOL_VERSION_POS];
            if(inner_version.version_def.ack_flag == RESPONSE_PACKAGE) { //response package
                LOG(LEVEL_INFO,"receive a ack package during MESSAGE_RESOLVE\n");

                response_package_handle((data[L1_HEADER_SEQ_ID_LOW_BYTE_POS] | (data[L1_HEADER_SEQ_ID_HIGH_BYTE_POS] << 8)),inner_version.version_def.err_flag);
            }

            /* because there's no buffer to contain these data, so just ignore these package */

            break;
        default:
            break;
    }
}

/******************************************************************
* to control whether to check dev loss when bluetooth disconnected   
*******************************************************************/
static bool should_checkdev_loss_on_disconnect = false;

bool get_should_checkdev_loss_on_disconnect(void)
{
    return should_checkdev_loss_on_disconnect;
}

void set_should_checkdev_loss_on_disconnect(bool value)
{
    should_checkdev_loss_on_disconnect = value;
}

uint32_t bluetooth_l0_reset(void)
{
    uint32_t i = 0;

    //first stop async send event
    SendCompletePara sendPara;
    sendPara.callback = NULL;
    sendPara.context = NULL;
    sendPara.task_type = TASK_NONE;
    set_complete_callback(sendPara);

    //reset L2_frame_resolve machine
    if(private_bond_machine == PRIVATE_BOND_SUCCESS) {//bluetooth disconnected from bonded state so should check dev loss
        set_should_checkdev_loss_on_disconnect(true);
    }

    private_bond_machine = PRIVATE_NOT_BOND;

    g_next_L1_send_content_buffer = 0;

    current_task_type = TASK_NONE;
    next_task_type = TASK_NONE;


    for( ; i< MAX_SEND_TASK; ++i) {
        sendContent[i].isUsed = 0;
        sendContent[i].resendCount = 0;
    }

    if(NULL != current_package_wait_response) {
        current_package_wait_response->isUsed = 0;
        if(current_package_wait_response->callback) {
            current_package_wait_response->callback(SEND_FAIL);
        }
    }

    receive_state = WAIT_START;
    received_content_length = 0;
    length_to_receive = 0;

    //disalbe bluetooth log when bluetooth disconnect
    set_bluetooth_log_state(false);

    memset(&g_ack_package_buffer,0,sizeof(struct Response_Buff_Type_t));


    if(app_query_timer(delay_send_wait_timer) == NRF_SUCCESS) {
        app_timer_stop(delay_send_wait_timer);
    }

    if(app_query_timer(receive_time_out_timer) == NRF_SUCCESS) {
        app_timer_stop(receive_time_out_timer);
    }

    return NRF_SUCCESS;
}
