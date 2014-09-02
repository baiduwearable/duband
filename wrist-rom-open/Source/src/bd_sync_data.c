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
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "bd_sync_data.h"
#include "step_counter.h"
#include "simple_uart.h"
#include "bd_led_flash.h"
#include "ble_flash.h"
#include "bd_wall_clock_timer.h"
#include "bd_interaction.h"
#include "nrf_soc.h"
#include "bd_communicate_protocol.h"
#include "bd_switch_sleep.h"

extern SleepHead_t mSleepHead;
extern SportsHead_t mSportHead;
extern SportsData_U mSportsData;
extern SleepData_U mSleepData;
extern bool notify_steps_enable;

uint32_t g_sports_send_buffer_align[(SPORTS_SEND_BUFFER_SIZE +3) /sizeof(uint32_t)];
uint8_t *g_sports_send_buffer = (uint8_t *)g_sports_send_buffer_align;
uint32_t g_sleep_send_buffer_align[(SLEEP_SEND_BUFFER_SIZE + 3)/sizeof(uint32_t)];
uint8_t *g_sleep_send_buffer = (uint8_t *)g_sleep_send_buffer_align;


bool sending_flag = false;
Saved_Health_Data_Info_t g_saved_SportsData, g_saved_SleepData;


uint16_t sending_steps = 0;

static algorithm_event_sleep_t current_sleep_event;


//time for send data
app_timer_id_t  send_data_timer;
#define SEND_DATA_INTERVAL          APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)


#define NVRAM_ERR_FULL 1;
void clear_prev_user_data(void)
{
    reset_sport_data();
    reset_sleep_data();
    reset_sleep_setting();

}
void reset_sport_data(void)
{
    g_saved_SportsData.p_read_addr = (uint32_t *)SPORTS_DATA_BASE_ADDR;
    g_saved_SportsData.p_write_addr = (uint32_t *)SPORTS_DATA_BASE_ADDR;

    uint32_t * address = NULL;
    ble_flash_page_addr(SPORTS_DATA_BASE_PAGE,&address);

    if(*address != 0xFFFFFFFF) {
        ble_flash_page_delay_erase(SPORTS_DATA_BASE_PAGE);
    }
}
void reset_sleep_data(void)
{
    g_saved_SleepData.p_read_addr = (uint32_t *)SLEEP_DATA_BASE_ADDR;
    g_saved_SleepData.p_write_addr = (uint32_t *)SLEEP_DATA_BASE_ADDR;

    uint32_t * address = NULL;
    ble_flash_page_addr(SLEEP_DATA_BASE_PAGE,&address);

    if(*address != 0xFFFFFFFF) {
        ble_flash_page_delay_erase(SLEEP_DATA_BASE_PAGE);
    }
}


uint32_t get_flash_page_number(uint32_t * address);
void check_need_erase_sleep_page_for_when_boot (void)
{
    uint32_t *address = NULL;
    uint32_t offset = 0;

    if((g_saved_SleepData.p_write_addr == 0)  || (g_saved_SleepData.p_read_addr == 0)) {
        g_saved_SleepData.p_write_addr = (uint32_t *)SLEEP_DATA_BASE_ADDR;
        g_saved_SleepData.p_read_addr = (uint32_t *)SLEEP_DATA_BASE_ADDR;
    }


     //we change sleep group size in 1.2.2, so we need to reset sleep data if we update to 1.2.2,
    //after we re-org storage strategy, we should not face the same problem
    if(g_saved_SleepData.p_write_addr != (uint32_t *)(BLE_FLASH_PAGE_SIZE * (get_flash_page_number(g_saved_SleepData.p_write_addr)))) {
        offset = g_saved_SleepData.p_write_addr  - (uint32_t *)(BLE_FLASH_PAGE_SIZE * (get_flash_page_number(g_saved_SleepData.p_write_addr)));

        if((offset % ONE_SLEEP_GROUP_SIZE) != 0) {
            g_saved_SleepData.p_write_addr = (uint32_t *)SLEEP_DATA_BASE_ADDR;
            g_saved_SleepData.p_read_addr = (uint32_t *)SLEEP_DATA_BASE_ADDR;
        }
    }

    address = g_saved_SleepData.p_write_addr;

    if((*address != 0xFFFFFFFF) || (*(address + 1) != 0xFFFFFFFF)) {
        ble_flash_page_erase(get_flash_page_number(g_saved_SleepData.p_write_addr));

        g_saved_SleepData.p_write_addr = (uint32_t *)(BLE_FLASH_PAGE_SIZE * (get_flash_page_number(g_saved_SleepData.p_write_addr)));

        if(get_flash_page_number(g_saved_SleepData.p_write_addr) == get_flash_page_number(g_saved_SleepData.p_read_addr)) {
            g_saved_SleepData.p_read_addr = g_saved_SleepData.p_write_addr;
        }

    }


   

}


void check_need_erase_sport_page_for_when_boot (void)
{
    uint32_t *address = NULL;
    uint32_t offset = 0;

    if((g_saved_SportsData.p_write_addr == 0) || (g_saved_SportsData.p_read_addr == 0)) {
        g_saved_SportsData.p_write_addr = (uint32_t *)SPORTS_DATA_BASE_ADDR;
        g_saved_SportsData.p_read_addr  = (uint32_t *)SPORTS_DATA_BASE_ADDR;
    }

     //we change sleep group size in 1.2.2, so we need to reset sleep data if we update to 1.2.2,
    //after we re-org storage strategy, we should not face the same problem
    if(g_saved_SportsData.p_write_addr != (uint32_t *)(BLE_FLASH_PAGE_SIZE * (get_flash_page_number(g_saved_SportsData.p_write_addr)))) {
        offset = g_saved_SportsData.p_write_addr  - (uint32_t *)(BLE_FLASH_PAGE_SIZE * (get_flash_page_number(g_saved_SportsData.p_write_addr)));

        if((offset % ONE_SPORTS_GROUP_SIZE) != 0) {
            g_saved_SportsData.p_write_addr = (uint32_t *)SPORTS_DATA_BASE_ADDR;
            g_saved_SportsData.p_read_addr = (uint32_t *)SPORTS_DATA_BASE_ADDR;
        }
    }

    address = g_saved_SportsData.p_write_addr;

    if((*address != 0xFFFFFFFF) || *(address + 1) != 0xFFFFFFFF) {
        ble_flash_page_erase(get_flash_page_number(g_saved_SportsData.p_write_addr));
        g_saved_SportsData.p_write_addr = (uint32_t *)(BLE_FLASH_PAGE_SIZE * (get_flash_page_number(g_saved_SportsData.p_write_addr)));

        if(get_flash_page_number(g_saved_SportsData.p_write_addr) == get_flash_page_number(g_saved_SportsData.p_read_addr)) {
            g_saved_SportsData.p_read_addr = g_saved_SportsData.p_write_addr;
        }
    }

}


uint32_t get_flash_page_number(uint32_t * address)
{
    return ((uint32_t)address/BLE_FLASH_PAGE_SIZE);
}

bool have_sleep_group_to_send()
{
    //no data to read
    if(g_saved_SleepData.p_read_addr == g_saved_SleepData.p_write_addr) {
#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"no sleep data in flash for send \n");
#endif

        return false;
    } else {
        return true;
    }
}

bool have_sleep_data_in_ram_to_send(void)
{
    return (mSleepHead.length != 0);
}

uint32_t* get_next_sleep_read_group_address(void )
{
    uint32_t* next_read_address = NULL;

#ifdef DEBUG_LOG
    //        LOG(LEVEL_INFO,"get_next_sleep_read_group_address \n");;
    LOG(LEVEL_INFO," g_saved_SleepData.p_read_addr:0x%x  \n",g_saved_SleepData.p_read_addr);
#endif

    //if the next read group is in same page
    if(get_flash_page_number(g_saved_SleepData.p_read_addr)
       == get_flash_page_number(g_saved_SleepData.p_read_addr + ((2*ONE_SLEEP_GROUP_SIZE)/4) )) {
        next_read_address =  g_saved_SleepData.p_read_addr + (ONE_SLEEP_GROUP_SIZE/4);
    } else {
        if(get_flash_page_number(g_saved_SleepData.p_read_addr) + 1 < SLEEP_DATA_END_PAGE ) {
            next_read_address = (uint32_t* )( BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SleepData.p_read_addr) + 1));
        } else {
            next_read_address =  (uint32_t* )( BLE_FLASH_PAGE_SIZE*SLEEP_DATA_BASE_PAGE);
        }
    }

#ifdef DEBUG_LOG
    LOG(LEVEL_INFO," next_read_address:0x%x  \n",next_read_address);
#endif

    return next_read_address;


}


uint32_t* get_next_sleep_write_group_address(void)
{
    uint32_t* next_write_address = NULL;

#ifdef DEBUG_LOG
    //         LOG(LEVEL_INFO,"get_next_sleep_write_group_address \n");
    LOG(LEVEL_INFO,"p_write_addr:0x%x  \n",g_saved_SleepData.p_write_addr);
#endif

    //if the page only have space for one gourp data ,we should erase next page
    if(get_flash_page_number(g_saved_SleepData.p_write_addr + (3*ONE_SLEEP_GROUP_SIZE)/4 )
       != get_flash_page_number(g_saved_SleepData.p_write_addr)
       && get_flash_page_number(g_saved_SleepData.p_write_addr + (2*ONE_SLEEP_GROUP_SIZE)/4 )
       == get_flash_page_number(g_saved_SleepData.p_write_addr )) {

#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"need to erase next page: p_write_addr:0x%x \n",g_saved_SleepData.p_write_addr);
#endif

        if(get_flash_page_number(g_saved_SleepData.p_write_addr) + 1 < SLEEP_DATA_END_PAGE ) {
            ble_flash_page_delay_erase(get_flash_page_number(g_saved_SleepData.p_write_addr) + 1);

            if((uint32_t)g_saved_SleepData.p_read_addr >= BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SleepData.p_write_addr) + 1)
               && (uint32_t)g_saved_SleepData.p_read_addr < BLE_FLASH_PAGE_SIZE + BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SleepData.p_write_addr) + 1)  ) {
                if(get_flash_page_number(g_saved_SleepData.p_read_addr) + 1 == SLEEP_DATA_END_PAGE) {
                    g_saved_SleepData.p_read_addr = (uint32_t* )(BLE_FLASH_PAGE_SIZE*SLEEP_DATA_BASE_PAGE);
                } else {
                    g_saved_SleepData.p_read_addr =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SleepData.p_read_addr) + 1));
                }
            }
        } else {
            ble_flash_page_delay_erase(SLEEP_DATA_BASE_PAGE);
            if((uint32_t )g_saved_SleepData.p_read_addr >= BLE_FLASH_PAGE_SIZE*SLEEP_DATA_BASE_PAGE
               && (uint32_t)g_saved_SleepData.p_read_addr < BLE_FLASH_PAGE_SIZE + BLE_FLASH_PAGE_SIZE*SLEEP_DATA_BASE_PAGE  ) {
                g_saved_SleepData.p_read_addr =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SleepData.p_read_addr) + 1));
            }
        }


    }
    //if the next read group is in same page
    if(get_flash_page_number(g_saved_SleepData.p_write_addr)
       == get_flash_page_number(g_saved_SleepData.p_write_addr + ((2*ONE_SLEEP_GROUP_SIZE)/4) ) ) {

        next_write_address =  (uint32_t* )(g_saved_SleepData.p_write_addr + (ONE_SLEEP_GROUP_SIZE/4));
    } else {


        if(get_flash_page_number(g_saved_SleepData.p_write_addr) + 1 < SLEEP_DATA_END_PAGE ) {
            next_write_address =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SleepData.p_write_addr) + 1));
        } else {
            next_write_address =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*SLEEP_DATA_BASE_PAGE);
        }
    }

#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"next_write_address:0x%x  \n",next_write_address);
#endif

    return next_write_address;
}



bool have_sport_group_to_send()
{
    //no data to send
    if(g_saved_SportsData.p_read_addr == g_saved_SportsData.p_write_addr) {
#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"no sport data in flash for send \n");
#endif

        return false;
    } else {
        return true;
    }
}

//check if there's data in ram to send
bool have_sport_data_in_ram_to_send(void)
{
    return ((mSportHead.length != 0) && (mSportsData.bits.steps != sending_steps));
}


bool have_history_sport_data_in_ram_to_send(void)
{
    return (mSportHead.length > 1);
}


uint32_t* get_next_sport_read_group_address(void )
{

    uint32_t* next_read_address = NULL;

#ifdef DEBUG_LOG
    //        LOG(LEVEL_INFO,"get_next_sport_read_group_address \n");
    LOG(LEVEL_INFO," p_read_addr:0x%x  \n",g_saved_SportsData.p_read_addr);
#endif

    //if the next read group is in same page
    if(get_flash_page_number(g_saved_SportsData.p_read_addr)
       == get_flash_page_number(g_saved_SportsData.p_read_addr + ((2*ONE_SPORTS_GROUP_SIZE)/4) )) {

        next_read_address =  (uint32_t* )(g_saved_SportsData.p_read_addr + (ONE_SPORTS_GROUP_SIZE/4));
    } else {


        if(get_flash_page_number(g_saved_SportsData.p_read_addr) + 1 < SPORTS_DATA_END_PAGE ) {
            next_read_address =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SportsData.p_read_addr) + 1));
        } else {
            next_read_address =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*SPORTS_DATA_BASE_PAGE);
        }
    }

#ifdef DEBUG_LOG
    LOG(LEVEL_INFO," next_read_address:0x%x  \n",next_read_address);
#endif

    return next_read_address;

}


uint32_t* get_next_sport_write_group_address(void)
{

    uint32_t* next_write_address = NULL;

#ifdef DEBUG_LOG
    //         LOG(LEVEL_INFO,"get_next_sport_write_group_address");
    LOG(LEVEL_INFO," p_write_addr:0x%x  \n",g_saved_SportsData.p_write_addr);
#endif

    //if the page only have space for one gourp data ,we should erase next page
    if(get_flash_page_number(g_saved_SportsData.p_write_addr + ((3*ONE_SPORTS_GROUP_SIZE)/4) )
       != get_flash_page_number(g_saved_SportsData.p_write_addr)
       && get_flash_page_number(g_saved_SportsData.p_write_addr + ((2*ONE_SPORTS_GROUP_SIZE)/4) )
       == get_flash_page_number(g_saved_SportsData.p_write_addr)) {

#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"need to erase next page: p_write_addr:0x%x \n",g_saved_SportsData.p_write_addr);
#endif

        if(get_flash_page_number(g_saved_SportsData.p_write_addr) + 1 < SPORTS_DATA_END_PAGE ) {
            ble_flash_page_delay_erase(get_flash_page_number(g_saved_SportsData.p_write_addr) + 1);

            if((uint32_t)(g_saved_SportsData.p_read_addr) >= BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SportsData.p_write_addr) + 1)
               && (uint32_t)(g_saved_SportsData.p_read_addr) < BLE_FLASH_PAGE_SIZE + BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SportsData.p_write_addr) + 1)  ) {
                if(get_flash_page_number(g_saved_SportsData.p_read_addr) + 1 == SPORTS_DATA_END_PAGE) {
                    g_saved_SportsData.p_read_addr = (uint32_t* )(BLE_FLASH_PAGE_SIZE*SPORTS_DATA_BASE_PAGE);
                } else {
                    g_saved_SportsData.p_read_addr =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SportsData.p_read_addr) + 1));
                }
            }

        } else {
            ble_flash_page_delay_erase(SPORTS_DATA_BASE_PAGE);

            if((uint32_t)(g_saved_SportsData.p_read_addr) >= BLE_FLASH_PAGE_SIZE*SPORTS_DATA_BASE_PAGE
               && (uint32_t)(g_saved_SportsData.p_read_addr) < BLE_FLASH_PAGE_SIZE + BLE_FLASH_PAGE_SIZE*SPORTS_DATA_BASE_PAGE  ) {
                g_saved_SportsData.p_read_addr =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SportsData.p_read_addr) + 1));
            }
        }
    }
    //if the next read group is in same page
    if(get_flash_page_number(g_saved_SportsData.p_write_addr)
       == get_flash_page_number(g_saved_SportsData.p_write_addr + ((2*ONE_SPORTS_GROUP_SIZE)/4) )) {

        next_write_address = (uint32_t* )(g_saved_SportsData.p_write_addr + (ONE_SPORTS_GROUP_SIZE/4));
    } else {
        if(get_flash_page_number(g_saved_SportsData.p_write_addr) + 1 < SPORTS_DATA_END_PAGE ) {
            next_write_address = (uint32_t* )(BLE_FLASH_PAGE_SIZE*(get_flash_page_number(g_saved_SportsData.p_write_addr) + 1));
        } else {
            next_write_address =  (uint32_t* )(BLE_FLASH_PAGE_SIZE*SPORTS_DATA_BASE_PAGE);
        }
    }

#ifdef DEBUG_LOG
    LOG(LEVEL_INFO," next_write_address:0x%x  \n",next_write_address);
#endif

    return next_write_address;
}

static int set_sport_data_buffer_header(char * buffer, SportsHead_t* header)
{
    uint16_t sportsDataLen;
    L2DataHeader_t mSportsDataHeader;

    if(NULL== buffer || NULL == header) {
        return -1;
    }

    sportsDataLen = (header->length<<3) + sizeof(SportsHead_t);// mSportHead.length * 8 + 4

    mSportsDataHeader.cmd_ID = HEALTH_DATA_COMMAND_ID;
    mSportsDataHeader.version = L2_HEADER_VERSION;
    mSportsDataHeader.key = KEY_RETURN_SPORTS_DATA;
    mSportsDataHeader.key_header.data= sportsDataLen & 0x01ff; // 9bit

    buffer[0] = mSportsDataHeader.cmd_ID; //L2 commandID
    buffer[1] = mSportsDataHeader.version; //
    buffer[2] = mSportsDataHeader.key;
    buffer[3] = mSportsDataHeader.key_header.data>>8;
    buffer[4] = mSportsDataHeader.key_header.data&0xff;
    buffer[5] = header->Date.data>>8;
    buffer[6] = header->Date.data;
    buffer[7] = header->key;
    buffer[8] = header->length;

    return (sportsDataLen + 5);

}

static int set_sleep_data_buffer_header(char * buffer, SleepHead_t* header)
{
    uint16_t sleepDataLen;
    L2DataHeader_t mSleepDataHeader;

    if(NULL== buffer || NULL == header) {
        return -1;
    }

    sleepDataLen = (header->length<<2) + sizeof(SleepHead_t);

    mSleepDataHeader.cmd_ID = HEALTH_DATA_COMMAND_ID;
    mSleepDataHeader.version = L2_HEADER_VERSION;
    mSleepDataHeader.key = KEY_RETURN_SLEEP_DATA;
    mSleepDataHeader.key_header.data= sleepDataLen & 0x01ff; // 9bit length

    buffer[0] = mSleepDataHeader.cmd_ID; //L2 commandID
    buffer[1] = mSleepDataHeader.version; //
    buffer[2] = mSleepDataHeader.key;
    buffer[3] = mSleepDataHeader.key_header.data>>8;
    buffer[4] = mSleepDataHeader.key_header.data&0xff;
    buffer[5] = header->Date.data>>8;
    buffer[6] = header->Date.data;
    buffer[7] = header->length >> 8;
    buffer[8] = header->length;

    return (sleepDataLen + 5);
    ;

}

uint32_t save_sport_group_data(uint8_t counts)
{
    int L1_payload_length = 0;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"save_sport_group_data \n");
#endif

    if(counts == 0)
        return NRF_SUCCESS;
    if(counts > SPORTS_MAX_GROUP_NUM)
        return NRF_ERROR_INVALID_LENGTH;


    L1_payload_length = set_sport_data_buffer_header((char *)g_sports_send_buffer, &mSportHead);

    if(NRF_SUCCESS != ble_flash_block_write(g_saved_SportsData.p_write_addr,(uint32_t* )g_sports_send_buffer,(L1_payload_length+3)/4)) {
        return NVRAM_ERR_FULL;
    }

    g_saved_SportsData.p_write_addr = get_next_sport_write_group_address();


    mSportHead.length = 0;
    mSportsData.bits.steps = 0;
    mSportsData.bits.mode = 0;
    mSportsData.bits.Calory = 0;
    mSportsData.bits.Distance = 0;

    return NRF_SUCCESS;
}
bool send_sport_group_data(void)
{
    uint32_t ret;
    uint16_t sentLen = 0;
    L2_Send_Content sendContent;

    uint8_t *read_address = NULL;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"send_sport_group_data \n");
#endif

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"p_read_addr: 0x%x \n",g_saved_SportsData.p_read_addr);
#endif

    read_address = (uint8_t*)(g_saved_SportsData.p_read_addr);

#ifdef DEBUG_LOG

    int i =0;
    for(i=0; i<9; i++) {
        LOG(LEVEL_INFO,"read_address[%d]: 0x%x \n",i,read_address[i]);
    }
#endif


    //command id,command version,key
    sentLen = read_address[3];
    sentLen = sentLen<<8;
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"sentLen hight value %d \n",sentLen);
#endif

    sentLen += read_address[4];


    //command id,command version,key,key_length
    sentLen += 5;

    if(sentLen > ONE_SPORTS_GROUP_SIZE || read_address[0]  != HEALTH_DATA_COMMAND_ID 
        ||read_address[1]  != L2_HEADER_VERSION ||read_address[2]  != KEY_RETURN_SPORTS_DATA) {
         g_saved_SportsData.p_read_addr = get_next_sport_read_group_address();
         return false;
    }

    sendContent.callback  = send_all_data_callback;
    sendContent.content  = (uint8_t *)read_address;
    sendContent.length  = sentLen;
    ret = L1_send(&sendContent);
    if(ret == NRF_SUCCESS) {
        return  true;
    } else {
        return false;
    }

}

uint32_t *  p_last_send_sport_addr = 0;
uint8_t sport_send_error_count = 0;
#define SPORT_SEND_ERROR_TORRENT  4
static void send_sport_flash_cb(bool success)
{
    if(true == success) {
        g_saved_SportsData.p_read_addr = get_next_sport_read_group_address();
    } else {
        if(p_last_send_sport_addr != g_saved_SportsData.p_read_addr) {
            p_last_send_sport_addr = g_saved_SportsData.p_read_addr;
            sport_send_error_count = 0;
        } else {
            sport_send_error_count ++;
        }

        if(sport_send_error_count >= SPORT_SEND_ERROR_TORRENT) {
            g_saved_SportsData.p_read_addr = get_next_sport_read_group_address();
        }
    }
}

bool send_sleep_group_data(void)
{
    uint32_t ret;
    uint16_t sentLen = 0;
    L2_Send_Content sendContent;

    uint8_t *read_address = NULL;


#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"send_sleep_group_data \n");
#endif

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"p_read_addr: 0x%x \n",g_saved_SleepData.p_read_addr);
#endif

    read_address =(uint8_t*)( g_saved_SleepData.p_read_addr);


#ifdef DEBUG_LOG

    int i =0;
    for(i=0; i<9; i++) {
        LOG(LEVEL_INFO,"read_address[%d]: 0x%x \n",i,read_address[i]);
    }
#endif


    //command id,command version,key
    sentLen = read_address[3];
    sentLen = sentLen<<8;
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"sentLen hight value %d \n",sentLen);
#endif

    sentLen += read_address[4];



    //command id,command version,key,key_length
    sentLen += 5;


    if(sentLen > ONE_SLEEP_GROUP_SIZE || read_address[0]  != HEALTH_DATA_COMMAND_ID 
        ||read_address[1]  != L2_HEADER_VERSION ||read_address[2]  != KEY_RETURN_SLEEP_DATA ) {
         g_saved_SleepData.p_read_addr = get_next_sleep_read_group_address();
         return false;
    }

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"sentLen %d \n",sentLen);
#endif

    sendContent.callback  = send_all_data_callback;
    sendContent.content  = (uint8_t *)read_address;
    sendContent.length  = sentLen;
    ret = L1_send(&sendContent);
    if(ret == NRF_SUCCESS) {
        return true;
    } else {
        return false;
    }

}
void update_sleep_head(void)
{
    UTCTimeStruct * tm = get_wall_clock_time();
    mSleepHead.Date.date.year = tm->year - 2000;
    mSleepHead.Date.date.month= tm->month;
    mSleepHead.Date.date.day = tm->day;
    mSleepHead.length = 0;

}
void reset_sleep_head(void)
{
    update_sleep_head();
}


uint32_t *  p_last_send_sleep_addr = 0;
uint8_t sleep_send_error_count = 0;
#define SLEEP_SEND_ERROR_TORRENT  4
static void send_sleep_flash_cb(bool success)
{
    if(true == success) {
         g_saved_SleepData.p_read_addr = get_next_sleep_read_group_address();
    } else {
        if(p_last_send_sleep_addr != g_saved_SleepData.p_read_addr) {
            p_last_send_sleep_addr = g_saved_SleepData.p_read_addr;
            sleep_send_error_count = 0;
        } else {
            sleep_send_error_count ++;
        }

        if(sleep_send_error_count >= SLEEP_SEND_ERROR_TORRENT) {
            g_saved_SleepData.p_read_addr = get_next_sleep_read_group_address();
        }
    }
}

void update_sports_head(void)
{
    UTCTimeStruct * tm = get_wall_clock_time();

    mSportHead.Date.date.day = tm->day;
    mSportHead.Date.date.month = tm->month;
    mSportHead.Date.date.year = tm->year-2000;
    mSportHead.length = 0;

}


uint8_t sync_process_buffer[5];

bool sync_process_start_send = false;

#define SYNC_PROCESS_SEND_INTERVAL 60



static bool need_send_sync_process(void )
{
    if(false == sync_process_start_send && (have_sleep_group_to_send() ||  have_sleep_data_in_ram_to_send()
                                            || sleep_setting_count()  || have_sport_group_to_send() || have_history_sport_data_in_ram_to_send())) {
        return true;
    } else {
        return false;
    }
}

static void send_sport_data_sync_start_cb(void)
{
    sync_process_start_send = true;
}

static void send_sport_data_sync_end_cb(void)
{
    sync_process_start_send = false;
}

void set_sport_data_sync_process_send(bool value)
{
    sync_process_start_send = value;
}

static bool send_sport_data_sync_start( )
{

    uint32_t  ret = NRF_SUCCESS;

    L2_Send_Content sendContent;

    sync_process_buffer[0] = HEALTH_DATA_COMMAND_ID; //L2 commandID
    sync_process_buffer[1] = L2_HEADER_VERSION; //L2 commandID
    sync_process_buffer[2] = KEY_DATA_SYNC_START; //key
    sync_process_buffer[3] = 0;
    sync_process_buffer[4] = 0;


    sendContent.callback  = send_all_data_callback;
    sendContent.content  = (uint8_t *)sync_process_buffer;
    sendContent.length  = 5;
    ret = L1_send(&sendContent);
    if(ret == NRF_SUCCESS) {
        return true;
    } else {
        return false;
    }

}


static bool send_sport_data_sync_end( )
{

    uint32_t  ret = NRF_SUCCESS;
    L2_Send_Content sendContent;

    sync_process_buffer[0] = HEALTH_DATA_COMMAND_ID; //L2 commandID
    sync_process_buffer[1] = L2_HEADER_VERSION; //L2 commandID
    sync_process_buffer[2] = KEY_DATA_SYNC_END; //key
    sync_process_buffer[3] = 0;
    sync_process_buffer[4] = 0;


    sendContent.callback  = send_all_data_callback;
    sendContent.content  = (uint8_t *)sync_process_buffer;
    sendContent.length  = 5;
    ret = L1_send(&sendContent);
    if(ret == NRF_SUCCESS) {
        return true;
    } else {
        return false;
    }

}


void update_cur_sports_data(void)
{
    reset_cur_sports_data();
}

void reset_cur_sports_data(void)
{
    UTCTimeStruct * tm = get_wall_clock_time();

    mSportsData.data = 0;
    mSportsData.bits.offset = tm->hour*4 + tm->minutes/15;
}

void reset_sports_head(void)
{
    UTCTimeStruct * tm = get_wall_clock_time();

    mSportHead.Date.date.day = tm->day;
    mSportHead.Date.date.month = tm->month;
    mSportHead.Date.date.year = tm->year-2000;
    mSportHead.length = 0;
}

static void send_sport_data_from_ram_cb(void)
{
    update_sports_head();

    update_cur_sports_data();
}

bool send_sport_data_from_ram(void)
{
    int L1_payload_length = 0;
    L2_Send_Content sendContent;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"send_sport_data_from_ram \n");
#endif

    if((mSportsData.bits.steps) != 0) {// current steps is not 0, put current sport data at the end of buffer
        // when comming here, mSportHead.length shall not be 0
        mSportsData.bits.Distance = (uint16_t)(get_quarter_distance()/10000);
        mSportsData.bits.Calory = ((uint32_t)(get_quarter_calories()/10) & 0x7FFFF);

        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 0] = mSportsData.data >> 56; //OFFSET
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 1] = mSportsData.data >> 48;
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 2] = mSportsData.data >> 40;
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 3] = mSportsData.data >> 32;
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 4] = mSportsData.data >> 24;
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 5] = mSportsData.data >> 16;
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 6] = mSportsData.data >> 8;
        g_sports_send_buffer[SPORT_HEAD_LEN + ((mSportHead.length-1) <<3) + 7] = mSportsData.data;

    }
    /******SportHead******/
    L1_payload_length = set_sport_data_buffer_header((char *)g_sports_send_buffer,&mSportHead);

    sendContent.callback  = send_all_data_callback;
    sendContent.content  = g_sports_send_buffer;
    sendContent.length   = L1_payload_length;
    if (NRF_SUCCESS == L1_send(&sendContent)) {
        sending_steps = mSportsData.bits.steps;
        return true;
    } else {
        return false;
    }

}

static void send_sleep_ram_data_cb(void)
{
    update_sleep_head();
}


static bool send_sleep_data_in_ram(void)
{
    int  L1_payload_length = 0;
    L2_Send_Content sendContent;

    /******SportHead******/
    L1_payload_length = set_sleep_data_buffer_header((char *)g_sleep_send_buffer, &mSleepHead);

    sendContent.callback  = send_all_data_callback;
    sendContent.content  = g_sleep_send_buffer;
    sendContent.length   = L1_payload_length;

    if (NRF_SUCCESS == L1_send(&sendContent)) {
        return true;
    } else {
        return false;
    }

}


uint32_t save_sleep_group_data(uint8_t counts)
{
    int L1_payload_length = 0;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"save_sleep_group_data \n");
#endif


    if(counts == 0)
        return NRF_SUCCESS;
    if(counts > SLEEP_MAX_GROUP_NUM)
        return NRF_ERROR_INVALID_LENGTH;

    L1_payload_length = set_sleep_data_buffer_header((char *)g_sleep_send_buffer, &mSleepHead);
#ifdef DEBUG_LOG

    int i =0;
    for(i=0; i<9; i++) {
        LOG(LEVEL_INFO,"buffer[%d]: 0x%x \n",i,g_sleep_send_buffer[i]);
    }
#endif

#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"p_write_addr: 0x%x \n",i,g_saved_SleepData.p_write_addr);
#endif

    if(NRF_SUCCESS != ble_flash_block_write(g_saved_SleepData.p_write_addr ,(uint32_t* )g_sleep_send_buffer,(L1_payload_length+3)/4)) {
        return NVRAM_ERR_FULL;
    }



    g_saved_SleepData.p_write_addr = get_next_sleep_write_group_address();
    mSleepHead.length = 0;

    return NRF_SUCCESS;
}

void get_current_sleep_data( algorithm_event_sleep_t * sleep_event )
{
    sleep_event->mode = current_sleep_event.mode;
    sleep_event->starting_time_stamp = current_sleep_event.starting_time_stamp;
}
/*
Handle sleep events, save sleep status in to buffer
*/
void sleep_evt_handler(algorithm_event_t *event)
{
    int sleepHead_minutes;
    int sleep_minutes = event->sleep.starting_time_stamp/60;
    time_union_t sleep_head_time;
    sleep_head_time.data = 0;
    sleep_head_time.time.year = mSleepHead.Date.date.year;
    sleep_head_time.time.month= mSleepHead.Date.date.month;
    sleep_head_time.time.day= mSleepHead.Date.date.day;

    sleepHead_minutes = convert_time_to_Second(sleep_head_time)/60;

    mSleepData.bits.timeStamp = (uint16_t)(sleep_minutes - sleepHead_minutes);

    current_sleep_event.mode = event->sleep.mode;
    current_sleep_event.starting_time_stamp = event->sleep.starting_time_stamp;
    mSleepData.bits.mode= event->sleep.mode;
    mSleepHead.length ++;

    g_sleep_send_buffer[SPORT_HEAD_LEN + ((mSleepHead.length-1) <<2) + 0] = mSleepData.data >> 24; //OFFSET
    g_sleep_send_buffer[SPORT_HEAD_LEN + ((mSleepHead.length-1) <<2) + 1] = mSleepData.data >> 16; // reserved
    g_sleep_send_buffer[SPORT_HEAD_LEN + ((mSleepHead.length-1) <<2) + 2] = mSleepData.data >> 8; // reserved
    g_sleep_send_buffer[SPORT_HEAD_LEN + ((mSleepHead.length-1) <<2) + 3] = mSleepData.data;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"sleep_head: %d-%d-%d",mSleepHead.Date.date.year, mSleepHead.Date.date.month, mSleepHead.Date.date.day);
    LOG(LEVEL_INFO,"mSleepData.bits.timeStamp %d", mSleepData.bits.timeStamp);
    LOG(LEVEL_INFO,"sleepStart_time:%d",event->sleep.starting_time_stamp);
    LOG(LEVEL_INFO,"sleep_minutes : %d-sleepHead_minutes %d", sleep_minutes, sleepHead_minutes);
#endif

    if(mSleepHead.length >= SLEEP_MAX_GROUP_NUM) {
        save_sleep_group_data((uint8_t)mSleepHead.length);
    }

}

void save_curr_health_data(void)
{
    save_quarter_distance_data(sports_mode());
    save_sleep_settings_to_flash();
    save_sleep_group_data((uint8_t)mSleepHead.length);
    save_sport_group_data(mSportHead.length);
}
#ifdef TESTFLASH
void test_send_status_callback(SEND_STATUS status )
{
    if(status == SEND_SUCCESS) {
#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"send success \n");
#else

        led_action_control(NOTIFICATION_DEBUG,CELEBRATE,1);
#endif

    } else {
#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"send fail \n");
#else

        led_action_control(NOTIFICATION_DEBUG,ALL_FLASH,1);
#endif

    }
}
typedef union {
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
    uint32_t value;
} Word_u;
void test_group_data_write(uint8_t group_cnt)
{
    uint16_t index,g_count;
    for(index = 0; index < SPORTS_SEND_BUFFER_SIZE; index ++)
        g_sports_send_buffer[index] = index;

    for(g_count = group_cnt; g_count > 0; g_count --)
        save_sport_group_data(g_count);
}
void test_flash_write(void)
{
    uint32_t pg_num = NRF_FICR->CODESIZE - 100; // Use last 100 pages in flash
    uint32_t *p_address = (uint32_t *)(FLASH_PAGE_SIZE * FLASH_PAGE_HEALTH_DATA);
    uint32_t *flash_addr = NULL;
    uint8_t *addr = NULL;
    uint16_t value;
    L2_Send_Content sendContent;
#ifdef DEBUG_LOG
    // simple_uart_putstring((const uint8_t *)" test_flash_write\r\n");
    LOG(LEVEL_INFO,"test_flash_write \n");
#endif

    ble_flash_page_erase(pg_num);
    for(flash_addr = p_address,value = 0; value< FLASH_PAGE_SIZE/2; value ++) {
        ble_flash_word_write(flash_addr++,value);
        //ble_nus_send_string(&m_nus,(uint8_t *) p_address,&len);
    }

    sendContent.callback = test_send_status_callback;
    sendContent.content  = (uint8_t *)p_address;
    sendContent.length  = FLASH_PAGE_SIZE*2;
    L1_send(&sendContent);

    for(addr = (uint8_t *)p_address ; addr < (uint8_t *)p_address + 128; addr ++)
        simple_uart_put(*addr);
}
void test_flash_send_1_group(void)
{}


void test_flash_send_2_group(void)
{}

#endif


void check_sports_data_in_flash(void)
{
    uint32_t *  address = NULL;
    uint8_t     page_num = SPORTS_DATA_BASE_PAGE;

    uint8_t header[4];

    for( ; page_num < SPORTS_DATA_BASE_PAGE + SPORTS_DATA_PAGE_NUM ; ++page_num) {
        /****************************************************************
        * A page contains several groups of data, if the first grout error then
        * all error ,if the first group success then all success
        *****************************************************************/
        ble_flash_page_addr(page_num , &address);

        memcpy((char *)header,(char *)address,4);
        LOG(LEVEL_INFO,"sports data paga %d first word :%x,%x,%x,%x",page_num,header[0],header[1],header[2],header[3]);

        //these page write from first word ,if first word is not writted , whole
        //page is erased
        if((header[0] == 0xff) && (header[1] == 0xff) && (header[2] == 0xff) && (header[3] == 0xff)) {
            continue;
        }

        if((header[0] != HEALTH_DATA_COMMAND_ID) ||  (header[1] != L2_HEADER_VERSION ) || (header[2] != KEY_RETURN_SPORTS_DATA)) {
            ble_flash_page_erase(page_num);
        }
    }


}

void check_sleep_data_in_flash(void)
{
    uint32_t *  address = NULL;
    uint8_t     page_num = SLEEP_DATA_BASE_PAGE;

    uint8_t header[4];

    for( ; page_num < SLEEP_DATA_BASE_PAGE + SLEEP_DATA_PAGE_NUM ; ++page_num) {
        /****************************************************************
        * A page contains several groups of data, if the first grout error then
        * all error ,if the first group success then all success
        *****************************************************************/
        ble_flash_page_addr(page_num , &address);

        memcpy((char *)header,(char *)address,4);
        LOG(LEVEL_INFO,"sleep data paga %d first word :%x,%x,%x,%x",page_num,header[0],header[1],header[2],header[3]);

        //these page write from first word ,if first word is not writted , whole
        //page is erased
        if((header[0] == 0xff) && (header[1] == 0xff) && (header[2] == 0xff) && (header[3] == 0xff)) {
            continue;
        }

        if((header[0] != HEALTH_DATA_COMMAND_ID) ||  (header[1] != L2_HEADER_VERSION ) || (header[2] != KEY_RETURN_SLEEP_DATA)) {
            ble_flash_page_erase(page_num);
        }
    }
}

/***************************************************************************
* rewrite send content
****************************************************************************/

static send_data_type_t last_send_type = NONE_DATA;

void send_all_data_callback(SEND_STATUS status)
{

    sending_flag = false;

    if(status != SEND_SUCCESS) {
        LOG(LEVEL_INFO,"send data fial");
        start_send_data_timer();
        switch(last_send_type) {
            case SPORTS_DATA_IN_RAM:
                sending_steps = 0;
                break;
            case SPORTS_DATA_IN_FLASH:
                send_sport_flash_cb(false);
                break;

            case SLEEP_DATA_IN_FLASH:
                send_sleep_flash_cb(false);
                break;

            default:
                break;
        };

        last_send_type = NONE_DATA;
        return;
    }

    //status must be success, so should update status
    switch(last_send_type) {
        case SLEEP_SETTING_DATA:
            send_sleep_setting_data_cb();
            break;
        case SPORTS_DATA_IN_RAM:
            send_sport_data_from_ram_cb();
            break;
        case SPORTS_DATA_IN_FLASH:
            send_sport_flash_cb(true);
            break;
        case SLEEP_DATA_IN_RAM:
            send_sleep_ram_data_cb();
            break;
        case SLEEP_DATA_IN_FLASH:
            send_sleep_flash_cb(true);
            break;
        case DATA_SYNC_START:
            send_sport_data_sync_start_cb();
            break;
        case DATA_SYNC_END:
            send_sport_data_sync_end_cb();
            break;
        default:
            break;
    };

    //deinit last send type
    last_send_type = NONE_DATA;


    send_all_data(true);

}

extern BLUETOOTH_BOND_STATE private_bond_machine;

void send_all_data(bool is_from_cb)
{

    if((private_bond_machine != PRIVATE_BOND_SUCCESS) ) {
        return;
    }

    if(sending_flag) {
        start_send_data_timer();

        return;
    }

    ASSERT(NONE_DATA == last_send_type);



    //proiority : sleep in flash > sleep  in ram > sleep setting in ram > sport in flash > sport in ram
    if(need_send_sync_process()) {
        if(true == send_sport_data_sync_start()) {
            last_send_type = DATA_SYNC_START;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if(have_sleep_group_to_send()) {
        if(true == send_sleep_group_data()) {
            last_send_type = SLEEP_DATA_IN_FLASH;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if(have_sleep_data_in_ram_to_send()) {
        if(true ==  send_sleep_data_in_ram()) {
            last_send_type = SLEEP_DATA_IN_RAM;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if(sleep_setting_count()) {
        if(true == send_sleep_setting_data()) {
            last_send_type = SLEEP_SETTING_DATA;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if(have_sport_group_to_send()) {
        if(true == send_sport_group_data()) {
            last_send_type = SPORTS_DATA_IN_FLASH;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if( have_history_sport_data_in_ram_to_send()) {

        if(true == send_sport_data_from_ram()) {
            last_send_type = SPORTS_DATA_IN_RAM;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if( true == notify_steps_enable
               && have_sport_data_in_ram_to_send()) {

        if(true == send_sport_data_from_ram()) {
            last_send_type = SPORTS_DATA_IN_RAM;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }
    } else if(true == sync_process_start_send) {
        if(true == send_sport_data_sync_end()) {
            last_send_type = DATA_SYNC_END;
            sending_flag = true;
        } else {
            start_send_data_timer();
        }

    }
}


bool send_time_started = false;

void send_data_timeout(void * val)
{
    send_time_started = false;
    send_all_data(true);
}

void create_send_data_timer(void)
{
    uint32_t err_code;
    err_code = app_timer_create(&send_data_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                send_data_timeout);
    APP_ERROR_CHECK(err_code);

    return;
}

void start_send_data_timer(void)
{
    uint32_t err_code;

    if(send_time_started) {

        return;
    }

    err_code = app_timer_start(send_data_timer, SEND_DATA_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    send_time_started = true;

    return;
}

void set_data_sending_flag(bool value)
{
    sending_flag = value;
}

bool get_data_sending_flag(void)
{
    return sending_flag;
}
