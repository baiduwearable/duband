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
#include "string.h"
#include "stdint.h"

#include "config.h"
#include "bd_input_event_source.h"
#include "board_config_pins.h"
#include "bd_ble_nus.h"
#include "bd_buzzer.h"
#include "app_button.h"
#include "app_timer.h"
#include "bd_communicate_protocol.h"
#include "step_counter.h"
#include "bd_wall_clock_timer.h"
#include "string.h"
#include "simple_uart.h"
#include "bd_private_bond.h"
#include "bd_switch_sleep.h"
#include "bd_interaction.h"
#include "bd_level_drive_motor.h"
#include "bd_charging.h"
#include "bd_led_flash.h"
#include "bd_sync_data.h"


extern ble_nus_t                        m_nus;
#ifdef HAS_BUTTON
extern app_timer_id_t  btn_identi;
#endif

extern BLUETOOTH_BOND_STATE private_bond_machine;

/////////////////////////////////////////////////////////////////
#ifdef TEST_SHOW_WALL_CLOCK

void report_current_time(void)
{
    // uint8_t outputArr[30];
    UTCTimeStruct * tm = get_wall_clock_time();
    // sprintf((char *)outputArr,"%d-%d-%d %d:%d:%d \r\n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
    // simple_uart_putstring((const uint8_t *)outputArr);
    LOG(LEVEL_INFO,"%d-%d-%d %d:%d:%d \n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
}

#endif

void input_event_handle_schedule(void * p_event_data, uint16_t event_size)
{
    if(event_size != sizeof(PressType)) {
        return;
    }

    PressType * type = (PressType * )p_event_data;

    switch(*type) {

        case SHORT_PRESS: {
                uint8_t notif = notification_status();
#ifdef DEBUG_LOG
                LOG(LEVEL_INFO,"input_event_handle_schedule:[%d]\n",notif);
#endif

                if(notif == NOTIFICATION_BONDING ) { // bond wait knock
                    bond_press_handle();
                } else if(!check_has_bonded()) {
                    break;
                } else if(notif == NOTIFICATION_CALLING) { //phone comming, knock to stop
                    notification_stop();
                } else if(notif == NOTIFICATION_ALARM) { //clock alarm ,knock to stop
                    notification_stop();
                } else if(notif == NOTIFICATION_STATE) { //knock to view step conter status
#ifdef DEBUG_LOG
//                    LOG(LEVEL_INFO,"NOTIFICATION_STATE\n");
#endif

                    notification_start(NOTIFICATION_STATE,0);
                }
                //else just igore
#ifdef DEBUG_LOG
//                LOG(LEVEL_INFO,"input_event_handle_schedule:[%d]\n",notif);
#endif

            }
            break;

        case LONG_PRESS: {
#ifdef DEBUG_LOG
//                LOG(LEVEL_INFO,"input_event_handle_schedule:LONG_PRESS\n");
#endif

                if(check_has_bonded()) {
                    //can not switch sleep mode, when charging
                    if( charger_status() != NoCharge ) {
                        break;
                    }
                    uint8_t notif = notification_status();
                    if(notif == NOTIFICATION_CALLING) { //phone comming, knock to stop
                        notification_stop();
                    } else if(notif == NOTIFICATION_ALARM) { //clock alarm ,knock to stop
                        notification_stop();
                    } else { //knock to view step conter status
                        two_Double_tap_handle();
                        send_all_data(false);
                    }
                }
            }
            break;
        default:
            break;


    }
}
#ifdef HAS_BUTTON
/* button identifier timer  hanler*/
void btn_timeout_handler(void * p_context)
{
    uint8_t *key = (uint8_t *)p_context;

    if(*key == NO_PRESS) { // long press time out handler
        *key = LONG_PRESS; // Long press identify

        PressType type = LONG_PRESS;
        app_sched_event_put(&type,sizeof(PressType),input_event_handle_schedule);

        return;
    }
}
#endif

/* button event handler*/
#ifdef FEATURE_BUTTON
void function_button_event_handler(uint8_t pin_no)
{

    static uint8_t pressType = NO_PRESS;

    if(pin_no != BAIDU_BUTTON_0) {
        return;
    }
    /*
    app_button_cfg_t *  btn_cfg = app_get_btn_cfg(FUNCTION_BUTTON);

    if(btn_cfg->isPushed){//press
     
      if(app_query_timer(btn_identi) == NRF_SUCCESS) {
        app_timer_stop(btn_identi);
      }
     
      pressType = NO_PRESS;//init

      if(pressType == NO_PRESS){
        //start identify timer
        app_timer_start(btn_identi, THREE_SECOND_INTERVAL, (void*)(&pressType));
      }
     
    } else {//release
     
      if(app_query_timer(btn_identi) == NRF_SUCCESS) {
        app_timer_stop(btn_identi);
      }
     
      if( pressType != LONG_PRESS ) {//no time out
      
        PressType type =SHORT_PRESS; 
        app_sched_event_put(&type,sizeof(PressType),input_event_handle_schedule);
       
      } else {//long press release
        pressType = NO_PRESS;
      }
     
    }*/

}
#endif


