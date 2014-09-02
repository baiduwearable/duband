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
#ifndef __BD_SYNC_DATA_H__
#define __BD_SYNC_DATA_H__
#include "health-algorithm.h"
#include "step_counter.h"

#include "bd_communicate_protocol.h"

#include "bd_data_manager.h"

#define SPORTS_DATA_PAGE_NUM  15
#define SLEEP_DATA_PAGE_NUM   15

#define SPORTS_DATA_BASE_PAGE  FLASH_PAGE_HEALTH_DATA
#define SLEEP_DATA_BASE_PAGE  (FLASH_PAGE_HEALTH_DATA + SPORTS_DATA_PAGE_NUM)

#define SPORTS_DATA_END_PAGE  (SPORTS_DATA_BASE_PAGE + SPORTS_DATA_PAGE_NUM)
#define SLEEP_DATA_END_PAGE   (SLEEP_DATA_BASE_PAGE + SLEEP_DATA_PAGE_NUM)

#define SPORTS_DATA_BASE_ADDR  (FLASH_PAGE_HEALTH_DATA * FLASH_PAGE_SIZE)
#define SLEEP_DATA_BASE_ADDR  (SLEEP_DATA_BASE_PAGE * FLASH_PAGE_SIZE)

#define SPORTS_MAX_GROUP_NUM   8    
#define ONE_SPORTS_GROUP_SIZE  (8+ 4 + SPORTS_MAX_GROUP_NUM * 8)
#define SLEEP_MAX_GROUP_NUM   8    
#define ONE_SLEEP_GROUP_SIZE   (8+ 4 + SLEEP_MAX_GROUP_NUM * 4)

#define SPORTS_SEND_BUFFER_SIZE    ((ONE_SPORTS_GROUP_SIZE/4+1) *4)
#define SLEEP_SEND_BUFFER_SIZE   ((ONE_SLEEP_GROUP_SIZE/4+1)*4 )

typedef struct
{
    uint8_t reservedA;
    uint8_t reservedB;
    uint8_t cmdID;
    uint8_t key;
    uint16_t L1_length; // L1 value length
    uint16_t L2_length; // L2 value length
}
L1L2Protocal_Head_t;

typedef struct
{
    uint32_t *  p_read_addr;
    uint32_t *  p_write_addr;  
}
Saved_Health_Data_Info_t;

typedef enum {
    NONE_DATA               = 0, //send nothing
    SLEEP_SETTING_DATA      = 1, //send sleep setting data 
    SPORTS_DATA_IN_RAM      = 2, //send sport data in ram 
    SPORTS_DATA_IN_FLASH    = 3, //send sport data in flash 
    SLEEP_DATA_IN_RAM       = 4, //send sleep data in ram 
    SLEEP_DATA_IN_FLASH     = 5, //send sleep data in flash
    DATA_SYNC_START     = 6, 
    DATA_SYNC_END    = 7
}send_data_type_t;

uint32_t save_sport_group_data(uint8_t counts);
uint32_t save_sleep_group_data(uint8_t counts);

bool send_sport_data_from_ram(void);
void reset_sport_data(void);
void reset_sleep_data(void);

void sleep_evt_handler(algorithm_event_t *event);
void save_daily_target_into_flash(void);
void update_sleep_head(void);
void update_sports_head(void);

void reset_sleep_head(void);
void reset_cur_sports_data(void);
void reset_sports_head(void);


void sync_profile(userprofile_union_t profile);
void sync_alarm(uint8_t alarm_count, alarm_union_t * alarmList);
void sync_daily_target(uint32_t target);
void sync_data_to_flash(uint8_t flash_page_num);

void clear_prev_user_data(void);
void save_curr_health_data(void);

bool send_sport_group_data(void);
bool send_sleep_group_data(void);

void check_sports_data_in_flash(void);
void check_sleep_data_in_flash(void);
bool have_sleep_data_in_ram_to_send(void);


void check_need_erase_sleep_page_for_when_boot (void);
void check_need_erase_sport_page_for_when_boot (void) ;


void send_all_data_callback(SEND_STATUS status);
void send_all_data(bool is_from_cb);

void start_send_data_timer(void);

void create_send_data_timer(void);


void set_data_sending_flag(bool value);
bool get_data_sending_flag(void);

void get_current_sleep_data( algorithm_event_sleep_t * sleep_event );

void set_sport_data_sync_process_send(bool value);

#ifdef TESTFLASH
void test_flash_write(void);
void test_flash_send_1_group(void);
void test_flash_send_2_group(void);
void test_group_data_write(uint8_t group_cnt);
#endif


#endif
