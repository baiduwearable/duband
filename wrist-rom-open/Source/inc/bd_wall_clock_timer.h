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
#ifndef WALL_CLOCK_TIMER_H_
#define WALL_CLOCK_TIMER_H_

#include <stdint.h>
#include "bd_communicate_protocol.h"

#define IsLeapYear(yr) (!((yr) % 400) || (((yr) % 100) && !((yr) % 4)))

#define YearLength(yr) (IsLeapYear(yr) ? 366 : 365)

// number of seconds since 0 hrs, 0 minutes, 0 seconds, on the
// 1st of January 2000 UTC

typedef uint32_t UTCTime; /* used to store the second counts for RTC */

#define BEGYEAR         2000     // UTC started at 00:00:00 January 1, 2000

#define DAY             86400UL  // 24 hours * 60 minutes * 60 seconds


// To be used with
typedef struct
{
    uint16_t year;    // 2000+
    uint8_t month;    // 0-11
    uint8_t day;      // 0-30
    uint8_t seconds;  // 0-59
    uint8_t minutes;  // 0-59
    uint8_t hour;     // 0-23
}
UTCTimeStruct;
typedef enum {
    MOn  = 0,
    Tues  = 1,
    Wed  = 2,
    Thur = 3,
    Fri  = 4,
    Sat  = 5,
    Sun  = 6
}DAY_OF_WEEK;


extern void system_clock_init(void);
void set_system_clock(time_union_t time);
UTCTimeStruct * get_wall_clock_time(void);
DAY_OF_WEEK get_day_of_week(UTCTime secTime);//used to calculate day of week
void return_alarm_list(void);
void set_alarm_list(void);

uint8_t monthLength( uint8_t lpyr, uint8_t mon );
UTCTime get_wall_clock_time_counter(void);
void set_wall_clock_time_counter(UTCTime counter);
bool is_systme_clock_valid(void);
UTCTime convert_time_to_Second(time_union_t time);
void ConvertToUTCTime( UTCTimeStruct *tm, UTCTime secTime );


//2000-01-01 is sat
#define SYSTEM_ORIGIN_DAY_OF_WEEK (Sat)
#define LENGTH_OF_WEEK      (7)

#endif //WALL_CLOCK_TIMER_H_
