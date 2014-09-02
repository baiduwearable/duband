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
#include <stdint.h>
#include "config.h"
#include "bd_led_flash.h"
#include "bd_interaction.h"
#include "app_timer.h"
#include "step_counter.h"
#include "bd_level_drive_motor.h"
#include "bd_data_manager.h"
#include "simple_uart.h"
#include "bd_switch_sleep.h"
extern SleepData_U      manual_sleep_event;

static void show_step_task_complete_status(void );
static void show_is_in_sleep_mode(void);
static void  show_animation_status(bool sleeping);

void notification_start( uint8_t type, uint16_t value )
{
    switch(type) {
        case NOTIFICATION_BONDING:
            led_action_control(NOTIFICATION_BONDING,SERIAL_FLASH,0);
            break;
        case NOTIFICATION_BONDED:
            led_action_control(NOTIFICATION_BONDED,ALL_FLASH,1);       //bond success notify
            motor_action_control_start(SLOW_VIBRATE,1);
            break;
        case NOTIFICATION_CALLING:
            if(led_action_control(NOTIFICATION_CALLING,FIRST_LED_FLASH,15)) {
                motor_action_control_start(SLOW_VIBRATE,6);
            }
            break;
        case NOTIFICATION_ALARM:
            if(led_action_control(NOTIFICATION_ALARM,LED_BLANK,15)) {
                motor_action_control_start(FAST_VIBRATE,6);// alarm
            }
            break;
        case NOTIFICATION_LOSE:
            if( value ) {
                if(led_action_control(NOTIFICATION_LOSE,SERIAL_FLASH,1)) {
                    motor_action_control_start(FAST_VIBRATE,3);
                }
            } else {
                led_action_control(NOTIFICATION_LOSE,SERIAL_FLASH,10);
            }
            break;
        case NOTIFICATION_SWITCH:
            if( value ) {
                if(led_action_control(NOTIFICATION_SWITCH,FIFTH_LED_FLASH,3)) {
                    motor_action_control_start(SLOW_VIBRATE,1);
                }
            } else {
                if(led_action_control(NOTIFICATION_SWITCH,THIRD_LED_FLASH,3)) {
                    motor_action_control_start(SLOW_VIBRATE,1);
                }
            }
            break;
        case NOTIFICATION_TARGET:
            if(led_action_control(NOTIFICATION_TARGET,BORDER_TO_CENTRAL,3)) {
                motor_action_control_start(SLOW_VIBRATE,1);
            }
            break;
        case NOTIFICATION_STATE:
            if(get_curr_sleep_flag()) {
                show_is_in_sleep_mode();
            } else {
                show_step_task_complete_status();
            }
            break;            
        case NOTIFICATION_ANIMATE:
            if(get_curr_sleep_flag()) {
                show_animation_status(SETTING_SLEEP_MODE);
            } else {
                show_animation_status(SETTING_STEP_MODE);
            }
            break;

        default:
            break;
    }
}

void notification_stop( )
{
    motor_action_control_stop();
    led_action_stop();
}

//#define  TIME_INTERVAL     APP_TIMER_TICKS(500, 0)
#define  ANIMATION_TIMER_PERIOD     APP_TIMER_TICKS(100, 0)
static void  show_animation_status(bool mode )
{
    if(mode == SETTING_SLEEP_MODE){
        led_action_control(NOTIFICATION_STATE,FIFTH_LED_FLASH,1);
    }
    else{
    //    led_action_control(NOTIFICATION_STATE,ANIMATED_FLASH,1);
        led_action_control_with_interval(NOTIFICATION_STATE,ANIMATED_FLASH,1,ANIMATION_TIMER_PERIOD);

    }
}
static void  show_step_task_complete_status(void )
{
    uint8_t temp = 0;
    
    if( get_global_step_counts_today() > get_daily_target() ){
        temp = 100;
    }else{
        temp = (get_global_step_counts_today() * 100) /get_daily_target();
    }

    /* Àƒ…·ŒÂ»Î */
    if(temp < 20) {
        led_action_control(NOTIFICATION_STATE,STEP_PERCENTAGE_20,6);
    } else if(temp < 40) {
        led_action_control(NOTIFICATION_STATE,STEP_PERCENTAGE_40,6);
    } else if(temp < 60) {
        led_action_control(NOTIFICATION_STATE,STEP_PERCENTAGE_60,6);
    } else if(temp < 80) {
        led_action_control(NOTIFICATION_STATE,STEP_PERCENTAGE_80,6);
    } else if(temp < 100) {
        led_action_control(NOTIFICATION_STATE,STEP_PERCENTAGE_100,6);
    } else {
        led_action_control(NOTIFICATION_STATE,BORDER_TO_CENTRAL,3);
    }
#ifdef DEBUG_LOG
    //    char str[48];
    //    sprintf(str,"show_step_task_complete_status:[%d]\r\n",temp);
    //    simple_uart_putstring((const uint8_t *)str);
//    LOG(LEVEL_INFO,"show_step_task_complete_status:[%d]\n",temp);
#endif
}

static void show_is_in_sleep_mode(void)
{
    led_action_control(NOTIFICATION_STATE,FIFTH_LED_FLASH,3);
#ifdef DEBUG_LOG
    //    char str[48];
    //    sprintf(str,"show_is_in_sleep_mode\r\n");
    //    simple_uart_putstring((const uint8_t *)str);
//    LOG(LEVEL_INFO,"show_is_in_sleep_mode\n");
#endif
}
