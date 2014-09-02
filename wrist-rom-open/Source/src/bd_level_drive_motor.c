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
#include "config.h"
#include "board_config_pins.h"
#include "bd_led_flash.h"
#include "app_scheduler.h"
#include "app_error.h"
#include "app_timer.h"
#include "nrf_gpio.h"

#include "bd_level_drive_motor.h"
#include "hal_vibrate.h"

#define         MOTOR_FAST_INTERVAL1     APP_TIMER_TICKS(200,0)
#define         MOTOR_FAST_LAST_TIME     APP_TIMER_TICKS(300,0)
#define         MOTOR_FAST_INTERVAL2     APP_TIMER_TICKS(1200,0)

#define         MOTOR_SLOW_INTERVAL     APP_TIMER_TICKS(1700,0)
#define         MOTOR_SLOW_LAST_TIME    APP_TIMER_TICKS(300, 0)

/***************************************************************
* Fast and slow motion control period
****************************************************************/
static const uint32_t FAST_MOTION_PERIOD[] =
    {
        MOTOR_FAST_LAST_TIME,MOTOR_FAST_INTERVAL1,MOTOR_FAST_LAST_TIME,MOTOR_FAST_INTERVAL2
    };
static const uint32_t SLOW_MOTION_PERIOD[] =
    {
        MOTOR_SLOW_LAST_TIME,MOTOR_SLOW_INTERVAL
    };

//must be defined according to the order of FAST_MOTION_PERIOD or SLOW_MOTION_PERIOD
static const MOTOR_OP FAST_MOTION_PERIOD_OP[] =
    {
        VIBRATE,SLIENT,VIBRATE,SLIENT
    };
static const MOTOR_OP SLOW_MOTION_PERIOD_OP[] =
    {
        VIBRATE,SLIENT
    };


static  app_timer_id_t  motor_schedule_timer;

static  MOTOR_ACTION    current_motor_action  = VIBRATE_NONE;
static  uint16_t        motor_loop_timers  = 0;                       /* Motion loop control */
static  uint16_t        current_motor_loop = 0;


/************************************************************************************
* The following two function should be treated as hal layer for motor control
* Which means this framework just needs a start action and a stop action
*************************************************************************************/
/****************************************************************
* Motor virbrate start
*****************************************************************/
static void motor_virbrate_start(void)
{
#ifdef FEATURE_MOTOR_LRA
    hal_vibrate_once(1,1);
#endif
#ifdef FEATURE_MOTOR

    nrf_gpio_pin_set(BAIDU_MOTOR_0);
#endif //FEATURE_MOTOR_LRA
}

/****************************************************************
* Motor virbrate stop
****************************************************************/
static void motor_virbrate_stop(void)
{
#ifdef FEATURE_MOTOR
    nrf_gpio_pin_clear(BAIDU_MOTOR_0);
#endif
}


/**************************************************************
* Motion time out handle
***************************************************************/
static void motor_schedule_timer_handle(void * context)
{
    if(current_motor_loop >= motor_loop_timers) {

        current_motor_action  = VIBRATE_NONE;
        motor_loop_timers  = 0;
        current_motor_loop = 0;
        motor_virbrate_stop();
        return;
    }

    MOTOR_ACTION action = * ((MOTOR_ACTION*)context);
    uint32_t timer_interval = 0;
    MOTOR_OP op = SLIENT;

    switch (action) {
        case FAST_VIBRATE:
            timer_interval = FAST_MOTION_PERIOD[current_motor_loop % (ARRAY_LEN(FAST_MOTION_PERIOD))];
            op = FAST_MOTION_PERIOD_OP[current_motor_loop % (ARRAY_LEN(FAST_MOTION_PERIOD_OP))];
            break;
        case SLOW_VIBRATE:
            timer_interval = SLOW_MOTION_PERIOD[current_motor_loop % (ARRAY_LEN(SLOW_MOTION_PERIOD))];
            op = SLOW_MOTION_PERIOD_OP[current_motor_loop % (ARRAY_LEN(SLOW_MOTION_PERIOD_OP))];
            break;
        default:
            current_motor_action  = VIBRATE_NONE;
            break;
    }

    if(op == VIBRATE) {
        motor_virbrate_start();
    } else {
        motor_virbrate_stop();
    }

    app_timer_start(motor_schedule_timer,timer_interval,&current_motor_action);
    current_motor_loop++;
}

/****************************************************************
* Motor action control
* para des:
*       action          : virbrate types
*       virbrate_times  : how many times should motor virbrate
*****************************************************************/
void motor_action_control_start(MOTOR_ACTION action, uint16_t virbrate_times)
{
    uint32_t timer_interval = 0;

    //stop the timer already running
    app_timer_stop(motor_schedule_timer);
    motor_virbrate_stop();

    current_motor_loop      = 0;        //init
    current_motor_action    = action;

    switch (action) {
        case FAST_VIBRATE:
            motor_loop_timers = virbrate_times * ARRAY_LEN(FAST_MOTION_PERIOD);
            timer_interval = FAST_MOTION_PERIOD[current_motor_loop % (ARRAY_LEN(FAST_MOTION_PERIOD))];
            break;
        case SLOW_VIBRATE:
            motor_loop_timers = virbrate_times * ARRAY_LEN(SLOW_MOTION_PERIOD);
            timer_interval = SLOW_MOTION_PERIOD[current_motor_loop % (ARRAY_LEN(SLOW_MOTION_PERIOD))];
            break;
        default:
            current_motor_action  = VIBRATE_NONE;
            break;
    }


    motor_virbrate_start();

    current_motor_loop ++; //set to one
    app_timer_start(motor_schedule_timer,timer_interval,&current_motor_action);

}

/****************************************************************
* Motor action control
* para des:
*       action          : virbrate types
*       virbrate_times  : how many times should motor virbrate
*****************************************************************/
void motor_action_control_stop()
{
    app_timer_stop(motor_schedule_timer);
    motor_virbrate_stop();
    current_motor_action  = VIBRATE_NONE;
    motor_loop_timers  = 0;
    current_motor_loop = 0;
}

uint32_t motor_control_framework_init(void)
{
    return app_timer_create(&motor_schedule_timer,APP_TIMER_MODE_SINGLE_SHOT,motor_schedule_timer_handle);
}

/* MOTOR Init */
void motor_init(void)
{
#ifdef FEATURE_MOTOR_LRA
    GPIO_LED_CONFIG(LRA_EN_PIN);
#endif //FEATURE_MOTOR_LRA

#ifdef FEATURE_MOTOR

    GPIO_LED_CONFIG(BAIDU_MOTOR_0);
#endif //FEATURE_MOTOR
}
