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
#include "bd_switch_sleep.h"

#include "step_counter.h"
#include "ble_flash.h"
#include "nordic_common.h"
#include "nrf_gpio.h"
#include "board_config_pins.h"
#include "bd_lis3dh_driver.h"
#include "hal_acc.h"
#include "bd_spi_master.h"

#include "spi_master_config.h" // This file must be in the application folder
#include "nrf_error.h"
#include <stdlib.h>

#include "twi_master.h"
#include "hal_vibrate.h"
#include "bd_communicate_protocol.h"
#include "bd_wall_clock_timer.h"
#include "bd_sync_data.h"
#include "bd_square_root.h"
#include "bd_level_drive_motor.h"
#include "bd_led_flash.h"
#include "bd_input_event_source.h"
#include "bd_interaction.h"
#include "bd_data_manager.h"
#include "bd_led_flash.h"
#include "bd_interaction.h"
#include "bd_private_bond.h"

#include "simple_uart.h"

#include "click-algorithm.h"


#define DEEP_SLEEP_INTERVAL 5
//static uint8_t sensor_started_minutes = 0;
static uint32_t acc_index = 0;


extern uint8_t *g_sports_send_buffer;
extern uint8_t *g_sleep_send_buffer;
extern BLUETOOTH_BOND_STATE private_bond_machine;

app_timer_id_t m_sensor_timer_id;
static bool sensor_timer_status = false;

static long long algorithm_timestamp = 0;

static bool algorithm_started = false;

#define FIFOMODE 1

static bool daily_target_achieved = false;
//static uint16_t s_tap_time;

static uint16_t quarter_steps = 0;
static int quarter_distance = 0,quarter_calories = 0;
static uint16_t walkingSlow = 0,walkingQuick = 0,running = 0,maxMode = 0;
extern AxesRaw_t accData[32]; // Read acc data from FIFO
extern SleepHead_t *manual_sleep_events_head;
SportsData_U mSportsData;
SportsHead_t mSportHead;
SleepData_U  mSleepData;
SleepHead_t  mSleepHead;

static uint8_t mSleep_mode = NONE_SLEEP;

#define TAP_INTERVAL_1          APP_TIMER_TICKS(500, APP_TIMER_PRESCALER) 
#define TAP_INTERVAL_2          APP_TIMER_TICKS(700, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). */
#define SENSOR_INTERVAL          APP_TIMER_TICKS(280, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). */

bool notify_steps_enable;


//static uint8_t m_is_waving = 0;


void set_quarter_steps(uint16_t steps)
{
    quarter_steps = steps;
    LOG(LEVEL_INFO,"%d",quarter_steps);
}
void set_quarter_distance(int distance)
{
    quarter_distance = distance;
    LOG(LEVEL_INFO,"%d",quarter_distance);

}
void set_quarter_calories(int calories)
{
    quarter_calories = calories;
    LOG(LEVEL_INFO,"%d",quarter_calories);

}
void set_quater_sport_mode(uint8_t mode)
{
    mSportsData.bits.mode = mode & 0x03;
    LOG(LEVEL_INFO,"%d",mSportsData.bits.mode);

}
void set_quater_active_time(uint8_t active_time)
{
    mSportsData.bits.active_time = active_time & 0x0f;
    LOG(LEVEL_INFO,"%d",mSportsData.bits.active_time);
}
uint16_t get_quarter_steps(void)
{
    return quarter_steps;
}
int get_quarter_distance(void)
{
    return quarter_distance;
}
int get_quarter_calories(void)
{
    return quarter_calories;
}

/**< the setted step task for one day */

/**@brief Start timers.
*/
void sensor_timers_stop(void)
{
    uint32_t   err_code;
    if(sensor_timer_status) {

#if defined(DEBUG_LOG) || defined (DEBUG_ACC)
        LOG(LEVEL_INFO," stop sensor timer \r\n");
#endif

        err_code = app_timer_stop(m_sensor_timer_id);
        APP_ERROR_CHECK(err_code);
        sensor_timer_status = false;
//        sensor_started_minutes = 0;
    }
}
void sensor_timers_start(void)
{
    uint32_t               err_code;
    if(!sensor_timer_status) {

#if defined(DEBUG_LOG) || defined (DEBUG_ACC)
        LOG(LEVEL_INFO,">>startSensor:  %d\r\n",mSleep_mode);
#endif

        err_code = app_timer_start(m_sensor_timer_id, SENSOR_INTERVAL, NULL);
        APP_ERROR_CHECK(err_code);

        sensor_timer_status = true;
//        sensor_started_minutes = 0;
    }
}
uint8_t sports_mode(void)
{
    uint8_t sportMode;

    maxMode = walkingSlow > walkingQuick?walkingSlow:walkingQuick;//STEP_MODE_WALK_SLOW;
    maxMode = maxMode > running?maxMode:running;

    if(maxMode == walkingSlow) {
        sportMode = STEP_MODE_WALK_SLOW;
    } else if(maxMode == walkingQuick) {
        sportMode = STEP_MODE_WALK_QUICK;
    } else {
        sportMode = STEP_MODE_RUN;
    }
    return sportMode;
}

void handle_distance_event(algorithm_event_t *event)
{
    ASSERT(event->event_common.type == DISTANCE_EVENT);


    if(event->distance.new_steps > 0) {
        uint8_t sportMode;
        if(quarter_steps == 0) { //except the first frame
            mSportHead.length ++; // every time total steps change from 0 to non-zero, a new frame data
        }
        if(mSportHead.length == 0) {
            mSportHead.length = 1;
        }

        quarter_steps += event->distance.new_steps;
        set_global_step_counts_today(get_global_step_counts_today() + event->distance.new_steps);

#if defined(DEBUG_LOG) || defined (DEBUG_ACC1)

        LOG(LEVEL_INFO,"\r\n-----g:%d:tSteps:%d status:%d\r\n",get_global_step_counts_today(),quarter_steps, is_bluetooth_connected());
#endif

        if(event->distance.new_distances >0) {
            quarter_distance += event->distance.new_distances;
        }
        if(event->distance.new_calory > 0) {
            quarter_calories += event->distance.new_calory;
        }

        switch(event->distance.mode) {
            case STEP_MODE_WALK_SLOW :
                walkingSlow ++;
                break;
            case STEP_MODE_WALK_QUICK:
                walkingQuick ++;
                break;
            case STEP_MODE_RUN:
                running ++;
                break;
            default:
                break;
        }
        sportMode = sports_mode();
        mSportsData.bits.mode = sportMode;
        mSportsData.bits.steps = quarter_steps;

        if( get_global_step_counts_today() >= get_daily_target() && !daily_target_achieved) {
            daily_target_achieved = true;
            notification_start(NOTIFICATION_TARGET,0);
        } else if( get_global_step_counts_today() < get_daily_target() ) {
            daily_target_achieved = false;
        }

        send_all_data(false);

    }

}

void handle_sleep_event(algorithm_event_t *event)
{
    ASSERT(event->event_common.type == SLEEP_EVENT);

    mSleep_mode = event->sleep.mode;
    LOG(LEVEL_INFO, "ts: %lld | %d ", algorithm_timestamp, event->sleep.starting_time_stamp);
#ifdef DEBUG_LOG

    UTCTimeStruct * tm = get_wall_clock_time();
    LOG(LEVEL_INFO,"system_clock: [%d/%02d/%02d %02d:%02d:%02d]\r\n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
#endif

    sleep_evt_handler(event);
    
    send_all_data(false);

}

#ifdef SPORTS_DATA_MOCK_TEST
void mock_minute_timer_handler(UTCTimeStruct *tm);
UTCTime get_wall_clock_time_counter(void);
#endif

void health_algorithm_cb_implement(algorithm_event_t *event, void* user_data)
{

#ifdef SPORTS_DATA_MOCK_TEST
    static int inner_step_count = 1;
    static UTCTime init_time =0;
    if(init_time == 0) {
        init_time = get_wall_clock_time_counter(); //get current time
    }

    init_time = (((init_time - 1) / 60) + 1) * 60;    //round up to 60

    uint8_t j = 0;

    time_union_t sport_head_time;
    
    sport_head_time.data = 0;
    sport_head_time.time.year = mSportHead.Date.date.year;
    sport_head_time.time.month = mSportHead.Date.date.month;
    sport_head_time.time.day = mSportHead.Date.date.day;

    uint32_t sport_head_seconds = convert_time_to_Second(sport_head_time);

    algorithm_event_t sport_event;
    
    for(j = 0;(j<15) && (inner_step_count < 30000);++j,inner_step_count++) {
       
        sport_event.event_common.type = DISTANCE_EVENT;
        sport_event.distance.new_steps = 1;
        sport_event.distance.new_distances = 0.5;
        sport_event.distance.new_calory = 0.1;
        sport_event.distance.mode = (step_mode_t) (j%3);
        
        handle_distance_event(&sport_event);

        if(inner_step_count) {
            UTCTimeStruct time;
            ConvertToUTCTime(&time,init_time); 

            mock_minute_timer_handler(&time);

            init_time += 60;
            #if defined(DEBUG_LOG)
                LOG(LEVEL_INFO, "inner step_count is %d",inner_step_count);
            #endif
        }
    }

    

#else    

    UNUSED_VARIABLE(user_data);
    if(event->event_common.type == DISTANCE_EVENT) {
        handle_distance_event(event);
    }

#endif

#ifdef SLEEP_DATA_MOCK_TEST    
    static int count = 0; 
    uint8_t i;

    time_union_t sleep_head_time;
    
    sleep_head_time.data = 0;
    sleep_head_time.time.year = mSleepHead.Date.date.year;
    sleep_head_time.time.month= mSleepHead.Date.date.month;
    sleep_head_time.time.day= mSleepHead.Date.date.day;

    uint32_t sleepHead_seconds = convert_time_to_Second(sleep_head_time);

    algorithm_event_t sleep_event;
    for(i=0; (i<10) && (count < 30000) ; i++,count++) {
        sleep_event.event_common.type = SLEEP_EVENT;
        if(i%2 == 0) {
            sleep_event.sleep.mode = DEEP_SLEEP;
        } else { 
            sleep_event.sleep.mode = NONE_SLEEP;
        }

        sleep_event.sleep.starting_time_stamp = sleepHead_seconds + 60*i;
        handle_sleep_event(&sleep_event);
    }
    LOG(LEVEL_INFO,"Mock generate %d events",count);
#else 

    if(event->event_common.type == SLEEP_EVENT) {   //      SLEEP_EVENT
        handle_sleep_event(event);
    }


#endif

}

bool is_algorithm_started(void)
{
    return algorithm_started;
}

void stop_health_algorithm(void)
{
     algorithm_started = false;
     health_algorithm_finalize();
}

uint8_t start_health_algorithm(userprofile_union_t *user_profile)
{
    int err_code= 0;
    int hight_cm = user_profile->bit_field.hight;
    int weight_kg = user_profile->bit_field.weight;
    int age = user_profile->bit_field.age;
    char isfemale = user_profile->bit_field.gender;

    if(age <= 0) {
        age = DEFAULT_AGE;
    }
    if(weight_kg <= 0) {
        weight_kg = DEFAULT_WEIGHT_CM;
    }
    if(hight_cm <= 0) {
        hight_cm = DEFAULT_HIGHT_CM;
    }
    if(isfemale != GENDER_MALE && isfemale!=GENDER_FEMALE) {
        isfemale = DEFAULT_GENDER;
    }
#ifdef DEBUG_ACC
    LOG(LEVEL_INFO,"f:%d a:%d h%d w%d\r\n",isfemale,age,hight_cm,weight_kg);
#endif
#ifdef DEBUG_LOG

    UTCTimeStruct * tm = get_wall_clock_time();
    LOG(LEVEL_INFO,"new day: %d-%d-%d=%d:%d:%d\r\n",tm->year, tm->month, tm->day, tm->hour, tm->minutes, tm->seconds);
#endif

    if(algorithm_started == true) {
        stop_health_algorithm();
    }

    err_code = init_health_algorithm(25,hight_cm,weight_kg,age,isfemale);
    ASSERT(err_code == 0);

    algorithm_started = true;
    
    update_algorithm_timestamp();
    err_code = register_health_algorithm_callback(health_algorithm_cb_implement,(void *)NULL);
    ASSERT(err_code == 0);
    hal_acc_enable();
    return NRF_SUCCESS;
}

void restart_health_algorithm(void)
{

#ifdef DEBUG_LOG
    UTCTimeStruct * tm = get_wall_clock_time();
    LOG(LEVEL_INFO,"new day: %d-%d-%d=%d:%d:%d\r\n",tm->year, tm->month, tm->day, tm->hour, tm->minutes, tm->seconds);
#endif

#ifdef FEATURE_MOTOR_LRA

    hal_vibrate_init();
#endif  //FEATURE_MOTOR_LRA

    if(0 == mSportHead.length) {
        reset_sports_head();
        reset_cur_sports_data();
    }
    if(0 == mSleepHead.length) {
        reset_sleep_head();
    }
    if(0 == manual_sleep_events_head->length) {
        reset_sleep_setting_head();
    }

    start_health_algorithm(get_user_profile());

}

uint8_t reset_health_algorithm_data(void)
{
    reset_sports_head();
    reset_sleep_head();
    reset_sleep_setting_head();
    reset_cur_sports_data();

    quarter_steps= 0;
    quarter_distance=quarter_calories=0;
    walkingSlow=walkingQuick=running=0;

    set_global_step_counts_today(0);


    reset_sport_data();
    reset_sleep_data();

#ifdef FEATURE_MOTOR_LRA

    hal_vibrate_init();
#endif

    return 1;
}

void LIS3DH_INT1_event_handler(uint8_t pin_no)
{
    u8_t int1res;

    if(mSleep_mode != NONE_SLEEP) {        
        hal_acc_config_wake_int(false);
        sensor_timers_start();
    }

    spi_master_enable(SPI0);
    LIS3DH_GetInt1Src(&int1res);

    LIS3DH_RESET_MEM();
#ifdef DEBUG_LOG1

    char index,value;
    for(index = 0x07; index < 0x3D; index ++) {
        LIS3DH_ReadReg(index, &value);
        LOG(LEVEL_INFO,"config add:0x%x==v:0x%x \n",index,value);
    }
#endif
    spi_master_disable(SPI0);


}


void LIS3DH_INT2_event_handler(uint8_t pin_no)
{
    u8_t  click_res;
    spi_master_enable(SPI0);
    LIS3DH_GetClickResponse(&click_res);
    spi_master_disable(SPI0);

    switch(click_res) {
        case LIS3DH_DCLICK_Z_N:
        case LIS3DH_DCLICK_Z_P:

            break;
        default:
            break;
    }
}


app_timer_id_t m_double_tap_timer_id;

static uint8_t Taped_cnt = 0;

static bool first_tap_detected = false;

static long long click_algorithm_timestamp = 0;

void tap_timer_handler(void * val)
{

    PressType type;

    LOG(LEVEL_INFO,"tap_timer_handler Taped_cnt: %d \n",Taped_cnt);

    if(true == first_tap_detected) {

        if(Taped_cnt >= 2) {
            if(check_has_bonded()) {
                notification_start(NOTIFICATION_ANIMATE,1);
            }

            app_timer_start(m_double_tap_timer_id, TAP_INTERVAL_2, NULL);
        } else {
            Taped_cnt = 0;

        }
        first_tap_detected = false;
    } else {
        if(Taped_cnt == 2) {
            type = SHORT_PRESS;
            app_sched_event_put(&type,sizeof(PressType),input_event_handle_schedule);
        } else if(Taped_cnt >= 4 ) {
            type = LONG_PRESS;
            app_sched_event_put(&type,sizeof(PressType),input_event_handle_schedule);
        }
        Taped_cnt = 0;
    }

}




void sensor_timer_handle(void * val)
{
    int error_no = 0;

    uint8_t transfer_size;
    int32_t compose;
    int16_t accX,accY,accZ;
    uint8_t acc_ii = 0;
    
    hal_acc_GetFifoData(&transfer_size);

    for(; acc_ii < transfer_size; acc_ii ++, acc_index ++) {
        
            accX = (accData[acc_ii].AXIS_X>>3);  // mg
            accY = (accData[acc_ii].AXIS_Y>>3);
            accZ = (accData[acc_ii].AXIS_Z>>3);

            compose = SquareRoot(accX*accX + accY*accY + accZ*accZ);
            error_no = click_algorithm_accelerate_data_in(accX,accY,accZ,compose,click_algorithm_timestamp);
            ASSERT(0 == error_no);
            click_algorithm_timestamp += 10;

            if(acc_ii % 4 == 0 && true == algorithm_started ) {

                algorithm_timestamp += 39;
                error_no = health_algorithm_data_in_accelerate(accX,accY,accZ,compose,algorithm_timestamp);
                ASSERT(0 == error_no);

            }
    }

}

/*
when started, expire every 60 seconds
*/
void update_algorithm_timestamp(void)
{
    LOG(LEVEL_INFO, "ts: %lld : RTC: %d(%lld)", algorithm_timestamp, get_wall_clock_time_counter(),
        algorithm_timestamp - (long long)get_wall_clock_time_counter()*1000);
    algorithm_timestamp = get_wall_clock_time_counter();
    algorithm_timestamp *= 1000;
//    LOG(LEVEL_INFO, "ts: %lld : RTC: %d", algorithm_timestamp, get_wall_clock_time_counter());

}
void save_quarter_distance_data(uint8_t sportMode)
{
        if(quarter_steps) {
            mSportsData.bits.Calory = ((uint32_t)(quarter_calories/10) & 0x7ffff);
            mSportsData.bits.Distance = (uint16_t)(quarter_distance/10000);
            mSportsData.bits.mode = sportMode;
            mSportsData.bits.steps = quarter_steps;

            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 0] = mSportsData.data >> 56; //OFFSET
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 1] = mSportsData.data >> 48;
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 2] = mSportsData.data >> 40;
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 3] = mSportsData.data >> 32;
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 4] = mSportsData.data >> 24;
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 5] = mSportsData.data >> 16;
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 6] = mSportsData.data >> 8;
            g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 7] = mSportsData.data;

#ifdef DEBUG_LOG

            LOG(LEVEL_INFO,"mSportsData.mOffset:%x\n",mSportsData.bits.offset);
            LOG(LEVEL_INFO,"mOffsetH:%x\n",g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 0]);
            LOG(LEVEL_INFO,"mOffsetL:%x\n",g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 1]);
#endif


            if(mSportHead.length == SPORTS_MAX_GROUP_NUM) {
                if(save_sport_group_data(mSportHead.length) == NRF_SUCCESS) {
                    mSportHead.length = 0;
                } else {
                    // TODO:
                }
            }

        }
        // clear status
        mSportsData.bits.offset ++;
        quarter_steps= 0;
        quarter_distance=quarter_calories=0;
        walkingSlow=walkingQuick=running=0;
        mSportsData.bits.mode= 0;
        mSportsData.bits.steps= 0;
        mSportsData.bits.Calory = 0;
        mSportsData.bits.Distance = 0;
        mSportsData.bits.active_time = 0;
}

#ifdef SPORTS_DATA_MOCK_TEST

void minute_timer_handler(void *val)
{
    
}

void mock_minute_timer_handler(UTCTimeStruct *tm)
{
    uint8_t sportMode;
    
    sportMode = sports_mode();

    if( tm->hour == 0 && tm->minutes == 0 && tm->seconds ==0 ) {
#ifdef DEBUG_LOG
//        LOG(LEVEL_INFO,"new day: %d-%d-%d=%d:%d:%d\r\n",tm->year, tm->month, tm->day, tm->hour, tm->minutes, tm->seconds);
#endif
        set_global_step_counts_today(0);
        daily_target_achieved = false;
    }

    if(sportMode > walkingSlow)
        mSportsData.bits.active_time ++;

    if(mSleep_mode == DEEP_SLEEP) { // SLEEPING
        if(sensor_timer_status == false) { // sensor timer was stoped to save power
            update_algorithm_timestamp();
            health_algorithm_data_in_accelerate(0,0,0,0,algorithm_timestamp);
        } else { // movements triggered the sensor to work again, if after ten minutes still in deep sleep, stop it
#ifdef DEBUG_ACC
            LOG(3,"mSleep_mode%d,timer%d;/n",mSleep_mode, sensor_timer_status);
#endif
        }
    }
    if(tm->minutes%15 == 0) {
        save_quarter_distance_data(sportMode);
    }


}

#else //SPORTS_DATA_MOCK_TEST

void minute_timer_handler(void * val)
{
    uint8_t sportMode;
    UTCTimeStruct * tm = get_wall_clock_time();
    sportMode = sports_mode();
    update_algorithm_timestamp();  // calibrate timestamp to rtc

#ifdef DEBUG_ACC
        LOG(LEVEL_INFO,"algorithm_timestamp :RTC: %lld || %d \n",algorithm_timestamp, get_wall_clock_time_counter());
#endif

    if( tm->hour == 0 && tm->minutes == 0 && tm->seconds ==0 ) {
#ifdef DEBUG_LOG
//        LOG(LEVEL_INFO,"new day: %d-%d-%d=%d:%d:%d\r\n",tm->year, tm->month, tm->day, tm->hour, tm->minutes, tm->seconds);
#endif

        set_global_step_counts_today(0);
        daily_target_achieved = false;
    }

    if(sportMode > walkingSlow)
        mSportsData.bits.active_time ++;

    if(mSleep_mode == DEEP_SLEEP) { // SLEEPING
        if(sensor_timer_status == false) { // sensor timer was stoped to save power
            health_algorithm_data_in_accelerate(0,0,0,0,algorithm_timestamp);
        } else { // movements triggered the sensor to work again, if after ten minutes still in deep sleep, stop it
#ifdef DEBUG_ACC
            LOG(3,"mSleep_mode%d,timer%d;/n",mSleep_mode, sensor_timer_status);
#endif

        }
    }
    if(tm->minutes%15 == 0) {
        save_quarter_distance_data(sportMode);
    }
    
    update_sleep_status();

}

#endif //SPORTS_DATA_MOCK_TEST



void set_daily_target_achieved(bool value)
{
    daily_target_achieved = value;
}

bool get_daily_target_achieved(void)
{
    return daily_target_achieved; 
}


void click_algorithm_cb_implement(click_event_t *event, void* user_data)
{

    LOG(LEVEL_INFO,"click_algorithm_cb_implement \n");

    Taped_cnt ++;

    if(Taped_cnt == 1) {
        first_tap_detected = true;
        app_timer_start(m_double_tap_timer_id, TAP_INTERVAL_1, NULL);
    }

}


void start_click_algorithm(void )
{

    int err_code= 0;

    init_click_algorithm(100);

    err_code = register_click_algorithm_callback(click_algorithm_cb_implement,(void *)NULL);
    ASSERT(err_code == 0);

}
