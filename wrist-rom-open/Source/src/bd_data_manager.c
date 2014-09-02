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
#include <string.h>
#include "config.h"
#include "simple_uart.h"
#include "bd_data_manager.h"
#include "bd_communicate_protocol.h"
#include "step_counter.h"
#include "ble_flash.h"

static alarm_union_t alarms[MAX_ALARM_NUM];
static uint8_t alarm_num = 0;

static uint32_t  daily_step_target= DEFAULT_STEP_TASK;
static bool  daily_target_changed = false;

static userprofile_union_t user_profile;

//dev loss alert control block
static dev_loss_control global_dev_loss_control = {
            .data = 0,
        };


//total step counts one day
static uint32_t   global_step_counts_today = 0;


uint8_t get_alarm_number(void)
{
    return alarm_num;
}

int set_alarm_number(uint8_t num)
{
    if(num > MAX_ALARM_NUM) {
        return -1;
    }

    alarm_num = num;

    return 0;
}


alarm_union_t* get_alarm(uint8_t index)
{
    if(index >= MAX_ALARM_NUM) {
        return NULL;
    }

    return &(alarms[index]);
}

int set_alarm(uint8_t index,alarm_union_t *alarm)
{
    if(index >= MAX_ALARM_NUM) {
        return -1;
    }

    if(NULL == alarm) {
        return -1;
    }

    memcpy(&(alarms[index]),alarm,sizeof(alarm_union_t));

    return 0;
}

//pre declear
static void save_alarms_into_flash(void);
void save_alarm(void )
{
    uint32_t    alarm_mem[1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM];
    uint32_t    err_code;
    uint8_t     read_word_len = 1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM;
    uint8_t     alarm_count_stored;

    int         ret;

    err_code = bd_flash_read_page(FLASH_PAGE_ALARM_SETTINGS,alarm_mem,&read_word_len);
    if(err_code == NRF_SUCCESS) {
        alarm_count_stored = (uint8_t)alarm_mem[0];
        if(alarm_count_stored == alarm_num) {
            ret = memcmp((char *)alarms,(char *)(alarm_mem + 1),sizeof(alarm_union_t) * alarm_count_stored);
            if(ret == 0) { //do not need sync
                return ;
            }
        }
    }

    uint32_t *alarms_addr = NULL;
    ble_flash_page_addr((uint8_t) FLASH_PAGE_ALARM_SETTINGS,&alarms_addr);

    if(check_is_flash_erased(alarms_addr,(FLASH_PAGE_HEADER_LEN + 1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM)) != NRF_SUCCESS) { //page not erased
        ble_flash_page_delay_erase((uint8_t) FLASH_PAGE_ALARM_SETTINGS);
    } else {
        save_alarms_into_flash();
    }

}

void load_alarm(void)
{
    uint32_t    alarm_mem[1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM];

    uint32_t    err_code;
    uint8_t     read_word_len = 1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM;

    err_code = bd_flash_read_page(FLASH_PAGE_ALARM_SETTINGS,alarm_mem,&read_word_len);
    if(err_code != NRF_SUCCESS) {
        alarm_num = 0;
        memset((char *)alarms,0,sizeof(alarm_union_t) * MAX_ALARM_NUM);
    } else {
        alarm_num = (uint8_t) alarm_mem[0];
        memcpy((char*)alarms,(char *)(alarm_mem + 1),sizeof(alarm_union_t) * MAX_ALARM_NUM);
    }
}

userprofile_union_t * get_user_profile(void)
{
    return &user_profile;
}

int set_user_profile(userprofile_union_t *profile)
{
    if(NULL == profile) {
        return -1;
    }

    memcpy(&user_profile,profile,sizeof(userprofile_union_t));
#ifdef DEBUG_LOG1

    LOG(LEVEL_INFO,"s%d a:%d h%d w%d\r\n",user_profile.bit_field.gender,user_profile.bit_field.age,user_profile.bit_field.hight,user_profile.bit_field.weight);
#endif

    return 0;

}

//pre declear
static void save_profile_into_flash(void);
void save_user_profile(void )
{
    uint32_t    origin_profile;
    uint8_t     read_word_len = 1;
    uint32_t    err_code;

    err_code = bd_flash_read_page(FLASH_PAGE_USER_PROFILE,&origin_profile,&read_word_len);
    if((err_code == NRF_SUCCESS) && (read_word_len == 1)) {
        if(user_profile.data == origin_profile) {
            return;
        }
    }

    uint32_t *userProfile_addr;
    ble_flash_page_addr((uint8_t) FLASH_PAGE_USER_PROFILE,&userProfile_addr);

    if(check_is_flash_erased(userProfile_addr,1 + FLASH_PAGE_HEADER_LEN) != NRF_SUCCESS) { //the page is not erased
        ble_flash_page_delay_erase(FLASH_PAGE_USER_PROFILE);
    } else {
        save_profile_into_flash();
    }

}


void load_user_profile(void)
{
    uint32_t err_code;

    uint8_t read_word_len = 1;
    err_code = bd_flash_read_page(FLASH_PAGE_USER_PROFILE,&user_profile.data,&read_word_len);
    if((err_code != NRF_SUCCESS) || (read_word_len != 1)) {
        LOG(LEVEL_ERROR,"read user profile fail reset to default value");
        user_profile.bit_field.age = (uint8_t)DEFAULT_AGE;
        user_profile.bit_field.weight =(uint8_t)DEFAULT_WEIGHT_CM;
        user_profile.bit_field.hight = (uint8_t)DEFAULT_HIGHT_CM;
        user_profile.bit_field.gender = (uint8_t)DEFAULT_GENDER;
    }
}



uint32_t get_daily_target(void)
{
    return daily_step_target;

}

int set_daily_target(uint32_t target)
{
    if(target != daily_step_target) {
        daily_step_target = target;
        daily_target_changed = true;
    }

    return 0;
}

void save_daily_target( void)
{
    uint32_t *dailyTarget_addr;

    if(daily_target_changed) {
        ble_flash_page_addr(FLASH_PAGE_DAILY_TARGET, &dailyTarget_addr);
#ifdef DEBUG_LOG

        LOG(LEVEL_INFO,"oldTarget:%p:%x\r\n",dailyTarget_addr, *dailyTarget_addr);
#endif

        ble_flash_page_delay_erase((uint8_t) FLASH_PAGE_DAILY_TARGET);

        daily_target_changed = false;
    }

}


void load_daily_target(void)
{
    uint32_t err_code;

    uint8_t read_word_len = 1;
    err_code = bd_flash_read_page(FLASH_PAGE_DAILY_TARGET,&daily_step_target,&read_word_len);
    if((err_code != NRF_SUCCESS) || (read_word_len != 1) || (daily_step_target == 0)) {
        daily_step_target = DEFAULT_STEP_TASK;
    }

}

static void save_alarms_into_flash(void)
{
    uint32_t alarm_mem[1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM];

    alarm_mem[0] = (uint32_t)alarm_num;

    memcpy((char *)(alarm_mem + 1),(char *)alarms, sizeof(alarm_union_t) * MAX_ALARM_NUM);

    bd_flash_write_erased_page(FLASH_PAGE_ALARM_SETTINGS,alarm_mem,(1 + sizeof(alarm_union_t)/sizeof(uint32_t) * MAX_ALARM_NUM));
}


static void save_profile_into_flash(void)
{

    bd_flash_write_erased_page(FLASH_PAGE_USER_PROFILE,&user_profile.data,1);

}

static void save_daily_target_into_flash(void)
{
    bd_flash_write_erased_page(FLASH_PAGE_DAILY_TARGET,&daily_step_target,1);
}

void sync_data_to_flash(uint8_t flash_page_num)
{
#ifdef DEBUG_LOG
    LOG(3,"flash_page_num:%d ready to sync\r\n",flash_page_num);
#endif

    if (flash_page_num == FLASH_PAGE_DAILY_TARGET )
        save_daily_target_into_flash();
    else if (flash_page_num == FLASH_PAGE_USER_PROFILE)
        save_profile_into_flash();
    else if (flash_page_num == FLASH_PAGE_ALARM_SETTINGS)
        save_alarms_into_flash();
    else { // steps or sleep data


    }

}

void reset_user_data_for_new_bond(void)
{
    alarm_num = 0;
    user_profile.data = 0;
    daily_step_target = DEFAULT_STEP_TASK;
}


/**************************** Get and Set operation ********************************/

void set_global_dev_loss_controller_data(uint16_t data)
{
    global_dev_loss_control.data = data;
}

uint16_t get_global_dev_loss_controller_data(void)
{
    return global_dev_loss_control.data;
}

void set_global_dev_loss_should_alert(uint8_t alert)
{
    global_dev_loss_control.controller.should_alert = alert;
}

uint8_t get_global_dev_loss_should_alert(void)
{
    return global_dev_loss_control.controller.should_alert;
}

void set_global_dev_loss_alert_level(DEV_LOSS_ALERT_LEVEL alert_level)
{
    global_dev_loss_control.controller.alert_level = (uint8_t) alert_level;
}

DEV_LOSS_ALERT_LEVEL get_global_dev_loss_alert_level(void)
{
    return (DEV_LOSS_ALERT_LEVEL)global_dev_loss_control.controller.alert_level;
}


void set_global_step_counts_today(uint32_t steps)
{
    global_step_counts_today = steps;
}

uint32_t get_global_step_counts_today(void)
{
    return global_step_counts_today;
}
