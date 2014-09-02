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
#ifndef __BD_DATA_MANAGER_H__
#define __BD_DATA_MANAGER_H__

#include <stdint.h>
#include "bd_communicate_protocol.h"


/* alarm clock bit field */
typedef struct
{
uint64_t day_repeat_flag    :
    7;
uint64_t reserved   :
    4;
uint64_t id     :
    3;
uint64_t minute     :
    6;
uint64_t hour       :
    5;
uint64_t day            :
    5;
uint64_t month      :
    4;
uint64_t year           :
    6;
}
alarm_clock_bit_field_type_t;

typedef union {
    uint64_t data;
    alarm_clock_bit_field_type_t alarm;
} alarm_union_t;


typedef struct
{
uint32_t reserved   :
    5;
uint32_t weight     :
    10; /** accuracy: 0.5 kg, */
uint32_t hight      :
    9;  /** hight accuracy : 0.5 m */
uint32_t age        :
    7;  /**age 0~127*/
uint32_t gender     :
    1;  /**0: female, 1: male*/
}
userprofile_bit_field_type_t;

typedef union {
    uint32_t data;
    userprofile_bit_field_type_t bit_field;
} userprofile_union_t;


uint8_t get_alarm_number(void);
int set_alarm_number(uint8_t num);
alarm_union_t* get_alarm(uint8_t index);
int set_alarm(uint8_t index,alarm_union_t *alarm);
void save_alarm(void );
void load_alarm(void);
userprofile_union_t * get_user_profile(void) ;
int set_user_profile(userprofile_union_t *profile);
void save_user_profile(void );
void load_user_profile(void);
uint32_t get_daily_target(void);
int set_daily_target(uint32_t target);
void save_daily_target(void );
void load_daily_target(void);
void sync_data_to_flash(uint8_t flash_page_num);
void reset_user_data_for_new_bond(void);

void set_global_dev_loss_controller_data(uint16_t data);
uint16_t get_global_dev_loss_controller_data(void);
void set_global_dev_loss_should_alert(uint8_t alert);
uint8_t get_global_dev_loss_should_alert(void);
void set_global_dev_loss_alert_level(DEV_LOSS_ALERT_LEVEL alert_level);
DEV_LOSS_ALERT_LEVEL get_global_dev_loss_alert_level(void);
void set_global_step_counts_today(uint32_t steps);
uint32_t get_global_step_counts_today(void);

#endif //__BD_DATA_MANAGER_H__
