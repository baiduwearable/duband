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
#include "nrf51.h"
#include "app_timer.h"
#include "bd_communicate_protocol.h"
#include "bd_wall_clock_timer.h"
#include "bd_level_drive_motor.h"
#include "bd_led_flash.h"
#include "bd_interaction.h"
#include "bd_battery.h"
#include "bd_wdt.h"
#ifdef DEBUG_LOG
#include "simple_uart.h"
#else
#define simple_uart_putstring(x) void(x)
#endif

#include "bd_sync_data.h"
#include "bd_data_manager.h"

uint8_t alarm_later;
#define APP_TIMER_PRESCALER 0
//wall clock id
static app_timer_id_t                 wallClockID;
#define ONE_MINUTE_INTERVAL         APP_TIMER_TICKS(1000*60, APP_TIMER_PRESCALER)
#define ONE_SECOND_INTERVAL         APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

UTCTimeStruct Global_Time;
static UTCTime SecondCountRTC;      /*This value should be stored in flash*/
extern uint8_t global_reponse_buffer[];

/*********************************************************************
 * Get month length
 *********************************************************************/
uint8_t monthLength( uint8_t lpyr, uint8_t mon )
{
    uint8_t days = 31;

    if ( mon == 1 ) // feb
    {
        days = ( 28 + lpyr );
    } else {
        if ( mon > 6 ) // aug-dec
        {
            mon--;
        }

        if ( mon & 1 ) {
            days = 30;
        }
    }

    return ( days );
}

/**************************************************************************
* Calculte UTCTime
***************************************************************************/
void ConvertToUTCTime( UTCTimeStruct *tm, UTCTime secTime )
{
    // calculate the time less than a day - hours, minutes, seconds
    {
        uint32_t day = secTime % DAY;
        tm->seconds = day % 60UL;
        tm->minutes = (day % 3600UL) / 60UL;
        tm->hour = day / 3600UL;
    }

    // Fill in the calendar - day, month, year
    {
        uint16_t numDays = secTime / DAY;
        tm->year = BEGYEAR;
        while ( numDays >= YearLength( tm->year ) )
        {
            numDays -= YearLength( tm->year );
            tm->year++;
        }

        tm->month = 0;
        while ( numDays >= monthLength( IsLeapYear( tm->year ), tm->month ))
        {
            numDays -= monthLength( IsLeapYear( tm->year ), tm->month );
            tm->month++;
        }

        tm->day = numDays;
    }
}
void check_alarm(void)
{
    uint8_t index;
    static UTCTime  pre_SecondCountRTC;
    DAY_OF_WEEK day_of_week;
    UTCTimeStruct *tm;

    tm = get_wall_clock_time();
    if(alarm_later) {
        alarm_later --;
        if(pre_SecondCountRTC == SecondCountRTC - 180) {
            pre_SecondCountRTC = SecondCountRTC;
            notification_start(NOTIFICATION_ALARM,0);
        }
    }

    for(index =0; index < get_alarm_number(); index ++) {
        if(tm->hour == get_alarm(index)->alarm.hour && tm->minutes == get_alarm(index)->alarm.minute) // time
        {
            day_of_week = get_day_of_week(SecondCountRTC); // day of week
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO,"rtc: %d \r alarms:%d-%d-%d\n",SecondCountRTC,get_alarm(index)->alarm.year,get_alarm(index)->alarm.month,get_alarm(index)->alarm.day);
            LOG(LEVEL_INFO,"Global_Time:%d-%d-%d\n",tm->year,tm->month,tm->day);
            LOG(LEVEL_INFO," %d:%d|||dayofWeek:%d\n",tm->hour,tm->minutes,day_of_week);
#endif

            if((get_alarm(index)->alarm.day_repeat_flag & (1 << day_of_week)) ||
               (tm->day == get_alarm(index)->alarm.day && tm->month == get_alarm(index)->alarm.month && tm->year == get_alarm(index)->alarm.year + 2000))
            {
                pre_SecondCountRTC = SecondCountRTC;
                notification_start(NOTIFICATION_ALARM,0);
                alarm_later = 0;
#ifdef DEBUG_LOG

                LOG(LEVEL_INFO,"***alarming:%d:%d||dofW:%d\n",tm->hour,tm->minutes,day_of_week);
#endif

            }
        }
    }

}

/**********************************************
* Time out handle
***********************************************/
static void update_wall_clock(void * p_context)
{
    (void)p_context;

    /* Here we should use RTC attributes */
    SecondCountRTC++;
    
    wdt_feed();

    uint8_t s = SecondCountRTC%60;
    
    if( s == 0 || s == 30 ){    //update battery 30 secs.
        battery_start();
    }
    
    if(s) {
        return;
    }
    if(true == is_algorithm_started())
        minute_timer_handler(NULL);

    check_alarm();
}


/**************************************************************************
* system clock init
***************************************************************************/
void system_clock_init(void)
{
    uint32_t err_code;

    /* set a default value */
    Global_Time.year = 2000;
    Global_Time.month = 0;
    Global_Time.day = 0;
    Global_Time.hour = 0;
    Global_Time.minutes = 0;
    Global_Time.seconds = 0;

    /* Init ticks from RTC */
    SecondCountRTC = 0;       /* This should read from flash */
    //  00 42 00 40 01
    //  1000010000000000100000000000001
    err_code = app_timer_create(&wallClockID, APP_TIMER_MODE_REPEATED, update_wall_clock);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(wallClockID, ONE_SECOND_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}
UTCTime convert_time_to_Second(time_union_t time)
{
    uint32_t i = 0;
    UTCTime offset = 0;

    //day time
    offset += time.time.seconds;
    offset += time.time.minute * 60;
    offset += time.time.hours * 60 * 60;

    uint8_t leapYear = IsLeapYear(time.time.year + 2000);

    offset += DAY * (time.time.day - 1);

    for(i = 0; i < time.time.month - 1; ++i) { //month start from 1
        offset += monthLength(leapYear,i) * DAY;
    }

    for(i = 0; i< time.time.year ;++i) {
        if(IsLeapYear(i + 2000)) {
            offset += DAY * 366;
        } else {
            offset += DAY * 365;
        }
    }

    return offset;

}
void set_system_clock(time_union_t time)
{
    uint32_t i = 0;
    UTCTime offset = 0;

    //day time
    offset += time.time.seconds;
    offset += time.time.minute * 60;
    offset += time.time.hours * 60 * 60;

    uint8_t leapYear = IsLeapYear(time.time.year + 2000);

    offset += DAY * (time.time.day - 1);

    for(i = 0; i < time.time.month - 1; ++i) { //month start from 1
        offset += monthLength(leapYear,i) * DAY;
    }

    for(i = 0; i< time.time.year ;++i) {
        if(IsLeapYear(i + 2000)) {
            offset += DAY * 366;
        } else {
            offset += DAY * 365;
        }
    }

    SecondCountRTC = offset;

    /************************************************************
    * restart timer
    ************************************************************/
    app_timer_stop(wallClockID);
    app_timer_start(wallClockID, ONE_SECOND_INTERVAL, NULL);

#ifdef DEBUG_LOG

    {
        //    char str[64];
        UTCTimeStruct * tm = get_wall_clock_time();
        // sprintf(str,"set_system_clock: [%d/%02d/%02d %02d:%02d:%02d]\r\n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
        // simple_uart_putstring((const uint8_t *)str);
        LOG(LEVEL_INFO,"set_system_clock: [%d/%02d/%02d %02d:%02d:%02d]\n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
    }
#endif

}

UTCTimeStruct * get_wall_clock_time(void)
{
    ConvertToUTCTime(&Global_Time,SecondCountRTC);
    Global_Time.month += 1; //calibration
    Global_Time.day += 1; //calibration
    return &Global_Time;
}

/* calculate day of week */
DAY_OF_WEEK get_day_of_week(UTCTime secTime)
{
    uint32_t day = secTime / DAY;

    DAY_OF_WEEK today = (DAY_OF_WEEK)(((day %LENGTH_OF_WEEK) + SYSTEM_ORIGIN_DAY_OF_WEEK) %LENGTH_OF_WEEK);

    return today;
}
void return_alarm_list(void)
           {
               L2_Send_Content sendContent;
               uint8_t index;

               global_reponse_buffer[0] = SET_CONFIG_COMMAND_ID;    /*command id*/
               global_reponse_buffer[1] = L2_HEADER_VERSION;   /*L2 header version */
               global_reponse_buffer[2] = KEY_RETURN_ALARM_SETTINGS;         /*first key, */
               global_reponse_buffer[3] = 0;
               global_reponse_buffer[4] = get_alarm_number()*5;           /* length  = 1 */
               for(index = 0; index < get_alarm_number(); index ++) {
                   global_reponse_buffer[5 + index * 5] =  get_alarm(index)->data >> 32;          /* bond success */
                   global_reponse_buffer[6 + index * 5] =  get_alarm(index)->data >> 24;
                   global_reponse_buffer[7 + index * 5] =  get_alarm(index)->data >> 16;
                   global_reponse_buffer[8 + index * 5] =  get_alarm(index)->data >> 8;
                   global_reponse_buffer[9 + index * 5] =  get_alarm(index)->data;
               }

               sendContent.callback  = NULL;
               sendContent.content  = global_reponse_buffer;
               sendContent.length   = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4]; /*length of whole L2*/

               L1_send(&sendContent);
           }

           UTCTime get_wall_clock_time_counter(void)
           {
               return SecondCountRTC;
           }

           /******************************************************
           * update utctime only used in restart set time 
           *******************************************************/
           void set_wall_clock_time_counter(UTCTime counter)
           {
               SecondCountRTC = counter;

               /************************************************************
               * restart timer
               ************************************************************/
               app_timer_stop(wallClockID);
               app_timer_start(wallClockID, ONE_SECOND_INTERVAL, NULL);
           }

           bool is_systme_clock_valid(void)
           {
               UTCTimeStruct *tm =  get_wall_clock_time();
               if(2000 == tm->year)
                   return false;
               else
                   return true;

           }
