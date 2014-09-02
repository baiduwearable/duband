/* Copyright (c) [2013 Baidu]. All Rights Reserved.
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

#include "config.h"
#include "bd_sync_data.h"
#include "step_counter.h"
#include "bd_led_flash.h"
#include "bd_interaction.h"
#include "bd_level_drive_motor.h"
#include "ble_flash.h"
#include "bd_charging.h"
#include "bd_wall_clock_timer.h"
#include <string.h>


#if defined(DEBUG_LOG) || defined(DEBUG_ACC) || defined(DEBUG_PHILL)
#include "simple_uart.h"
#else
#define LOG(level, format, ...) 
#endif

#define SLEEP_SETTING_UNIT_SIZE 4
#define MAX_SLEEP_EVT_LEN 20
extern BLUETOOTH_BOND_STATE private_bond_machine;
SleepData_U manual_sleep_event;

static uint32_t sleep_events_aligned_buffer[(9 + MAX_SLEEP_EVT_LEN*4 +3)/sizeof(uint32_t)]; // flash  word align
uint8_t * sleep_events_buff = (uint8_t *)sleep_events_aligned_buffer;

SleepHead_t manual_sleep_events_head_data;
SleepHead_t *manual_sleep_events_head = &manual_sleep_events_head_data;

static bool curr_sleep_flag = false;
static uint8_t sleep_evts_sent_cnt;

static int sleepSetting_minutes = 0;   //minutes of the last sleep event
static uint32_t auto_wakeup_steps = 0;


static SleepHead_t last_sleep_setting_event_head;
static SleepData_U last_sleep_setting_event;

void update_last_time_sleep_mode(void)
{
    memcpy(&last_sleep_setting_event_head, &manual_sleep_events_head_data, sizeof(SleepHead_t));
    last_sleep_setting_event.data = manual_sleep_event.data;

    LOG(LEVEL_INFO, "%d, day:%d", last_sleep_setting_event_head.length, last_sleep_setting_event_head.Date.date.day);
    LOG(LEVEL_INFO, "flag:%d,mode: %d,ts %d", last_sleep_setting_event.bits.sleeping_flag,last_sleep_setting_event.bits.mode, last_sleep_setting_event.bits.timeStamp);
}

SleepData_U * get_last_sleep_setting_event(void)
{
    return &last_sleep_setting_event;
}
SleepHead_t * get_last_sleep_setting_event_head(void)
{
    return &last_sleep_setting_event_head;
}
uint16_t prepare_sleep_event_header(void)
{
    L2DataHeader_t mSleepEventHeader;

    uint16_t sentDataLen;

    sentDataLen = (manual_sleep_events_head->length * SLEEP_SETTING_UNIT_SIZE) + sizeof(SleepHead_t);

    mSleepEventHeader.cmd_ID = HEALTH_DATA_COMMAND_ID;
    mSleepEventHeader.version = L2_HEADER_VERSION;
    mSleepEventHeader.key = KEY_RETURN_SLEEP_SETTING;
    mSleepEventHeader.key_header.data= sentDataLen & 0x01ff; // 9bit length

    sleep_events_buff[0] = mSleepEventHeader.cmd_ID; //L2 commandID
    sleep_events_buff[1] = mSleepEventHeader.version; //
    sleep_events_buff[2] = mSleepEventHeader.key;
    sleep_events_buff[3] = mSleepEventHeader.key_header.data>>8;
    sleep_events_buff[4] = mSleepEventHeader.key_header.data&0xff;

    return sentDataLen;
}

void send_last_time_sleep_mode(void)
{
    LOG(LEVEL_INFO, "length %d, day:%d", last_sleep_setting_event_head.length, last_sleep_setting_event_head.Date.date.month, last_sleep_setting_event_head.Date.date.day);
    LOG(LEVEL_INFO, "flag:%d,mode: %d,ts %d", last_sleep_setting_event.bits.sleeping_flag,last_sleep_setting_event.bits.mode, last_sleep_setting_event.bits.timeStamp);

    if(last_sleep_setting_event.bits.mode == 0)
        return;

    manual_sleep_event.data = last_sleep_setting_event.data;
    memcpy(&manual_sleep_events_head_data, &last_sleep_setting_event_head, sizeof(SleepHead_t));
    manual_sleep_events_head->length++;

    LOG(LEVEL_INFO,"sleep_evt: %d,mode: %d; %d,%d th\n",manual_sleep_event.bits.sleeping_flag, manual_sleep_event.bits.mode,manual_sleep_event.bits.timeStamp, manual_sleep_events_head->length);

    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 0] = manual_sleep_event.data >> 24; //OFFSET
    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 1] = manual_sleep_event.data >> 16; // reserved
    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 2] = manual_sleep_event.data >> 8; // reserved
    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 3] = manual_sleep_event.data;
    
    prepare_sleep_event_header();
    send_all_data(false);
}

void switch_sleep_state( int sleep_time_by_minutes );

/*********************************************************************
* Bond action triggerd by user
**********************************************************************/
void set_curr_sleep_flag(bool sleeping)
{
    curr_sleep_flag = sleeping;
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO," sleeping flag: %d",curr_sleep_flag);
#endif
}
bool get_curr_sleep_flag(void)
{
    return curr_sleep_flag;
}

void update_sleep_status(void)
{
    int sleeping_time = 0;  //minutes of sleeping
    if( !get_curr_sleep_flag() ){
#ifdef FEATURE_AUTO_SLEEP
        if(charger_status()!= NoCharge) return; //skip when charging.
        //Auto Sleep
        UTCTimeStruct * tm = get_wall_clock_time();
        if( tm->hour >= 22 || tm->hour <= 4 ){  // 10PM - 5AM
            algorithm_event_sleep_t sleep_event;
            get_current_sleep_data( &sleep_event );
            if( ( sleep_event.mode == DEEP_SLEEP ) && ( ((int)get_wall_clock_time_counter() - sleep_event.starting_time_stamp ) > (30*60)) ){   //30 minutes
                if( sleep_event.starting_time_stamp/60 <= sleepSetting_minutes + 30 ) return;    //in 30 minutes, just skip
                if(manual_sleep_events_head->length > MAX_SLEEP_EVT_LEN)
                    return; // ignore more
                switch_sleep_state( sleep_event.starting_time_stamp/60 - 10 );  //15 minutes before.                    
            }
        }
#endif
        return;
    }
#ifdef FEATURE_AUTO_WAKEUP
    sleeping_time = get_wall_clock_time_counter()/60 - sleepSetting_minutes;
#ifdef DEBUG_PHILL
    LOG(LEVEL_INFO,"sleeping_time: %d, RTC: %d\n",sleeping_time,get_wall_clock_time_counter());
#endif
    if( sleeping_time >= (12*60) ){   //12 hours
        //auto wakeup
        two_Double_tap_handle();
    }else if( sleeping_time >= (4*60) ){    //4 hours
        algorithm_event_sleep_t sleep_event;
        get_current_sleep_data( &sleep_event );
#ifdef DEBUG_PHILL
    LOG(LEVEL_INFO,"sleep_evt: mode: %d; time_stamp: %d; diff: %d\n",sleep_event.mode, sleep_event.starting_time_stamp,get_wall_clock_time_counter() - sleep_event.starting_time_stamp);
#endif
        if( sleep_event.mode == NONE_SLEEP ){
            if( auto_wakeup_steps == 0 ) auto_wakeup_steps = get_global_step_counts_today();
            
            if ( (get_global_step_counts_today() > auto_wakeup_steps + 200 ) || ( ((int)get_wall_clock_time_counter() - sleep_event.starting_time_stamp ) > (10*60)) ){
#ifdef DEBUG_PHILL
                LOG(LEVEL_INFO,"Auto Wakeup, after 4 hours!\n");
#endif
                //auto wakeup
                two_Double_tap_handle();
                auto_wakeup_steps = 0;
            }
        }else{
            auto_wakeup_steps = 0;
        }
    }
#endif
}
void switch_sleep_state( int sleep_time_by_minutes )
{
    int sleepSetting_Head_minutes;
    sleepSetting_minutes = sleep_time_by_minutes;
    time_union_t sleep_setting_head_time;
    sleep_setting_head_time.data = 0;
    sleep_setting_head_time.time.year = manual_sleep_events_head->Date.date.year;
    sleep_setting_head_time.time.month= manual_sleep_events_head->Date.date.month;
    sleep_setting_head_time.time.day= manual_sleep_events_head->Date.date.day;

    sleepSetting_Head_minutes = convert_time_to_Second(sleep_setting_head_time)/60;

    set_curr_sleep_flag(!get_curr_sleep_flag());

    manual_sleep_events_head->length++;

    manual_sleep_event.bits.timeStamp = (uint16_t)(sleepSetting_minutes - sleepSetting_Head_minutes);
    manual_sleep_event.bits.mode= curr_sleep_flag;

#ifdef DEBUG_ACC
    LOG(LEVEL_INFO,"sleep_evt: 0x%x,mode: %d; %d,%d th\n",manual_sleep_event.data, manual_sleep_event.bits.mode,manual_sleep_event.bits.timeStamp, manual_sleep_events_head->length);
#endif

    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 0] = manual_sleep_event.data >> 24; //OFFSET
    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 1] = manual_sleep_event.data >> 16; // reserved
    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 2] = manual_sleep_event.data >> 8; // reserved
    sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 3] = manual_sleep_event.data;

}
void two_Double_tap_handle(void)
{
    if(manual_sleep_events_head->length > MAX_SLEEP_EVT_LEN)
        return; // ignore more
    notification_start( NOTIFICATION_SWITCH, !curr_sleep_flag );
    switch_sleep_state( get_wall_clock_time_counter()/60 );
}
void update_sleep_setting_head(void)
{
    UTCTimeStruct * tm = get_wall_clock_time();
    manual_sleep_events_head->Date.date.year = tm->year - 2000;
    manual_sleep_events_head->Date.date.month= tm->month;
    manual_sleep_events_head->Date.date.day = tm->day;
}

void reset_sleep_setting_head(void)
{
    memset(manual_sleep_events_head,0,sizeof(SleepHead_t));
    update_sleep_setting_head();

}
void reset_sleep_setting(void)
{
    reset_sleep_setting_head();
    manual_sleep_event.data = 0;
    curr_sleep_flag = false;
    sleepSetting_minutes = 0;
    auto_wakeup_steps = 0;

    uint32_t * address = NULL;
    ble_flash_page_addr(FLASH_PAGE_SLEEP_SETTINGS,&address);

    if(*address != 0xFFFFFFFF) {
        ble_flash_page_delay_erase(FLASH_PAGE_SLEEP_SETTINGS);
    }


}

void send_sleep_setting_data_cb(void)
{
    uint8_t index;

    manual_sleep_events_head->length -= sleep_evts_sent_cnt;

    for(index = 1; index <= manual_sleep_events_head->length; index ++) {
        sleep_events_buff[SPORT_HEAD_LEN + ((index-1)<<2) + 0] = sleep_events_buff[SPORT_HEAD_LEN + ((sleep_evts_sent_cnt + index -1)<<2) + 0];
        sleep_events_buff[SPORT_HEAD_LEN + ((index-1)<<2) + 1] = sleep_events_buff[SPORT_HEAD_LEN + ((sleep_evts_sent_cnt + index -1)<<2) + 1];
        sleep_events_buff[SPORT_HEAD_LEN + ((index-1)<<2) + 2] = sleep_events_buff[SPORT_HEAD_LEN + ((sleep_evts_sent_cnt + index -1)<<2) + 2];
        sleep_events_buff[SPORT_HEAD_LEN + ((index-1)<<2) + 3] = sleep_events_buff[SPORT_HEAD_LEN + ((sleep_evts_sent_cnt + index -1)<<2) + 3];
    }

    if(0 == manual_sleep_events_head->length){
        update_last_time_sleep_mode();
        update_sleep_setting_head();
    }

#ifdef DEBUG_ACC
    LOG(LEVEL_INFO, "send success,still have: %d\r\n", manual_sleep_events_head->length);
#endif

}

bool save_sleep_settings_to_flash(void)
{
    uint16_t sentDataLen;
    L2DataHeader_t mSleepEventHeader;

    if(0 == manual_sleep_events_head->length){
        return true;
    }
    /******SportHead******/
    sentDataLen = (manual_sleep_events_head->length<<2) + sizeof(SleepHead_t);

    mSleepEventHeader.cmd_ID = HEALTH_DATA_COMMAND_ID;
    mSleepEventHeader.version = L2_HEADER_VERSION;
    mSleepEventHeader.key = KEY_RETURN_SLEEP_SETTING;
    mSleepEventHeader.key_header.data= sentDataLen & 0x01ff; // 9bit length

    sleep_events_buff[0] = mSleepEventHeader.cmd_ID; //L2 commandID
    sleep_events_buff[1] = mSleepEventHeader.version; //
    sleep_events_buff[2] = mSleepEventHeader.key;
    sleep_events_buff[3] = mSleepEventHeader.key_header.data>>8;
    sleep_events_buff[4] = mSleepEventHeader.key_header.data&0xff;

    //change save sleep_setting to use memcpy
    memcpy(&(sleep_events_buff[5]),manual_sleep_events_head,sizeof(SleepHead_t));
#ifdef DEBUG_ACC
    LOG(LEVEL_INFO,"length:%d", manual_sleep_events_head->length);
    for(int ii = 0; ii <= (SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 3); ii ++)
    {
       LOG(LEVEL_INFO, "%x", sleep_events_buff[ii]);
    }

    LOG(LEVEL_INFO,"date:%d-%d-%d", manual_sleep_events_head->Date.date.year, manual_sleep_events_head->Date.date.month, manual_sleep_events_head->Date.date.day);

    LOG(LEVEL_INFO,"sleeping_flag: %d", manual_sleep_event.bits.mode);
    LOG(LEVEL_INFO,"timeStamp:%d", manual_sleep_event.bits.timeStamp);
#endif

    if(NRF_SUCCESS == bd_flash_write_erased_page(FLASH_PAGE_SLEEP_SETTINGS,(uint32_t * )sleep_events_buff,(sentDataLen + 5 +3)/4)){
        return true;
    }
    else{
        return false;
    }
}
void restore_SleepSettingsFromFlash(void)
{
    uint8_t word_count;
    ble_flash_page_read(FLASH_PAGE_SLEEP_SETTINGS,(uint32_t * )sleep_events_buff,&word_count);

    memcpy(manual_sleep_events_head,&(sleep_events_buff[5]),sizeof(SleepHead_t));
    if(manual_sleep_events_head_data.length != 0){
        manual_sleep_event.data = sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 0]; //OFFSET
        manual_sleep_event.data <<= 8;
        manual_sleep_event.data |= sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 1]; // reserved
        manual_sleep_event.data <<= 8;
        manual_sleep_event.data |= sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 2]; // reserved
        manual_sleep_event.data <<= 8;
        manual_sleep_event.data |= sleep_events_buff[SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 3];
        curr_sleep_flag = manual_sleep_event.bits.mode;

#ifdef DEBUG_LOG        
        LOG(LEVEL_INFO,"curr_sleep_flag:%d", curr_sleep_flag);
        LOG(LEVEL_INFO,"length:%d", manual_sleep_events_head->length);
        for(int ii = 0; ii <= (SPORT_HEAD_LEN + ((manual_sleep_events_head->length-1) <<2) + 3); ii ++)
         {
           LOG(LEVEL_INFO, "%d", sleep_events_buff[ii]);
         }
        LOG(LEVEL_INFO,"date:%d-%d-%d", manual_sleep_events_head->Date.date.year, manual_sleep_events_head->Date.date.month, manual_sleep_events_head->Date.date.day);

        LOG(LEVEL_INFO,"sleeping mode: %d", manual_sleep_event.bits.mode);
        LOG(LEVEL_INFO,"sleeping_flag: %d", manual_sleep_event.bits.mode);
        LOG(LEVEL_INFO,"timeStamp:%d", manual_sleep_event.bits.timeStamp);
#endif

    }

    ble_flash_page_erase(FLASH_PAGE_SLEEP_SETTINGS);
}
bool send_sleep_setting_data_in_ram(void)
{

    uint16_t sentDataLen;
    L2_Send_Content sendContent;
    L2DataHeader_t mSleepEventHeader;
 
    sentDataLen = (manual_sleep_events_head->length<<2) + sizeof(SleepHead_t);

    mSleepEventHeader.cmd_ID = HEALTH_DATA_COMMAND_ID;
    mSleepEventHeader.version = L2_HEADER_VERSION;
    mSleepEventHeader.key = KEY_RETURN_SLEEP_SETTING;
    mSleepEventHeader.key_header.data= sentDataLen & 0x01ff; // 9bit length

    sleep_events_buff[0] = mSleepEventHeader.cmd_ID; //L2 commandID
    sleep_events_buff[1] = mSleepEventHeader.version; //
    sleep_events_buff[2] = mSleepEventHeader.key;
    sleep_events_buff[3] = mSleepEventHeader.key_header.data>>8;
    sleep_events_buff[4] = mSleepEventHeader.key_header.data&0xff;

    sleep_events_buff[5] = manual_sleep_events_head->Date.data>>8;
    sleep_events_buff[6] = manual_sleep_events_head->Date.data;
    sleep_events_buff[7] = manual_sleep_events_head->length >> 8;
    sleep_events_buff[8] = manual_sleep_events_head->length;

    sendContent.callback = send_all_data_callback;
    sendContent.content  = sleep_events_buff;
    sendContent.length  = sentDataLen + 5;

    if (NRF_SUCCESS == L1_send(&sendContent)) {
        sleep_evts_sent_cnt = manual_sleep_events_head->length;
        return true;
    } else {
        return false;
    }

}


bool send_sleep_setting_data(void)
{
    return send_sleep_setting_data_in_ram();
}

uint16_t sleep_setting_count(void)
{
    return manual_sleep_events_head->length;
}

