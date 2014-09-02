/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#include "config.h"
#include "ble_debug_assert_handler.h"
#include <string.h>
#include "nrf51.h"
#include "nordic_common.h"
#include "ble_error_log.h"
#include "bd_wall_clock_timer.h"
#include "bd_communicate_protocol.h"
#include "ble_flash.h"
#include "simple_uart.h"
#include "step_counter.h"
#include "bd_sync_data.h"
#include "bd_data_manager.h"
#include "bd_switch_sleep.h"
#include "bd_private_bond.h"
#include "bd_wall_clock_timer.h"


#define MAX_LENGTH_FILENAME 16     /**< Max length of filename to copy for the debug error handlier. */

extern SportsData_U mSportsData;
extern SportsHead_t mSportHead;
extern SleepData_U  mSleepData;
extern SleepHead_t  mSleepHead;
extern Saved_Health_Data_Info_t g_saved_SportsData, g_saved_SleepData;
extern Saved_Health_Data_Info_t *g_saved_SleepSettings;
extern SleepHead_t *manual_sleep_events_head;

#define WORD_NUM_TO_BACK_UP     (15)

#define check_and_backup(dst,src,size,limit)                                                \
                    do {                                                                    \
                        if(dst + size > limit) {                                            \
                            LOG(LEVEL_ERROR,"backup data exceed size on line %d",__LINE__); \
                            return;                                                         \
                        }                                                                   \
                        memcpy(dst,src,size);                                               \
                        dst += size;                                                        \
                    } while(0);


#define check_and_restore(dst,src,size,limit)                                                 \
                    do {                                                                      \
                        if(src + size > limit) {                                              \
                            LOG(LEVEL_ERROR,"restore data exceed size on line %d",__LINE__);  \
                            goto ERROR_HANDLE;                                                \
                        }                                                                     \
                        memcpy(dst,src,size);                                                 \
                        src += size;                                                          \
                    }while(0);


void store_time_to_flash_v2(void)
{
    UTCTimeStruct * current_time = get_wall_clock_time();

    time_union_t time;
    memset(&time, 0, sizeof(time_union_t));
    time.time.year      = current_time->year - 2000;
    time.time.month     = current_time->month;
    time.time.day       = current_time->day;
    time.time.hours     = current_time->hour;
    time.time.minute    = current_time->minutes;
    time.time.seconds   = current_time->seconds;

    uint32_t * address =0;

    ble_flash_page_addr(FLASH_PAGE_STORE_TIME,&address);

    if(!address) { //Not use assert :)
        return;
    }

    uint16_t length = BLE_FLASH_PAGE_SIZE / sizeof(uint32_t);

    uint16_t i = 0;
    for(i = 0 ; i<length ; ++i,address++) {
        if(*address == 0xFFFFFFFF) {//flash init value
            break;
        }
    }

    if(i >= length) {
        //no space to write,so erase the page
        //Note: this operation should not occure because the page was erased in the read operation

        ble_flash_page_erase(FLASH_PAGE_STORE_TIME);
        ble_flash_page_addr(FLASH_PAGE_STORE_TIME,&address);
        if(!address) {
            return ;
        }
    }

    ble_flash_word_write(address,time.data);

}

time_union_t get_stored_time_from_flash_v2(void)
{
    uint32_t * address;

    ble_flash_page_addr(FLASH_PAGE_STORE_TIME,&address);

    uint16_t i = 0;
    uint16_t length = (BLE_FLASH_PAGE_SIZE /sizeof(uint32_t));

    for(i = 0 ; i < length ; ++i) {
        if(*address != 0xFFFFFFFF) {
            address++;
        }
    }

    //if whole page is not 0xfffff reflect that time stroed in last word
    address--;

    time_union_t time;
    time.data = * address;

    if(i >= length) { //erase the page for the next write operation
        ble_flash_page_erase(FLASH_PAGE_STORE_TIME);
    }

    return time;
}


/**********************************************************************************
* backup & restore step info & link loss info
***********************************************************************************/
uint32_t restore_backup_info_from_flash(void)
{
    uint32_t info_to_restore[WORD_NUM_TO_BACK_UP + FLASH_PAGE_HEADER_LEN];

    uint32_t *  address = 0;
    uint16_t    crc_header;
    uint16_t    cal_crc;
    uint32_t    header;
    uint32_t    word_count;
    uint8_t  *  current_pos;
    uint8_t  *  end_addr;

    SleepHead_t * sleep_setting_event_head = get_last_sleep_setting_event_head(); 
    SleepData_U * sleep_setting_event = get_last_sleep_setting_event();

    uint8_t i = 0;


    ble_flash_page_addr(FLASH_PAGE_STORE_TIME,&address);
    if(address) {
        memcpy((char *)info_to_restore,(char *)address,((WORD_NUM_TO_BACK_UP + FLASH_PAGE_HEADER_LEN) * sizeof(uint32_t)));
        word_count = info_to_restore[1];

        //do crc check
        header = info_to_restore[0];
        if((header & 0xFFFF0000U) == BLE_FLASH_MAGIC_NUMBER) {
            crc_header = (uint16_t) (header & 0x0000FFFFU);

            // Initialize CRC
            cal_crc = ble_flash_crc16_compute((uint8_t *) (info_to_restore + FLASH_PAGE_HEADER_LEN), (word_count * sizeof(uint32_t)), NULL);
            if(cal_crc == crc_header) {

                uint32_t data;
                
                current_pos = (uint8_t *)(info_to_restore + FLASH_PAGE_HEADER_LEN);
                end_addr    = (uint8_t *)(info_to_restore + FLASH_PAGE_HEADER_LEN + word_count);

                check_and_restore(&data,current_pos,sizeof(data),end_addr);
                set_wall_clock_time_counter(data);

                check_and_restore(&data,current_pos,sizeof(data),end_addr);
                set_global_step_counts_today(data);

                //XXX why put it here?
                check_and_restore(&data,current_pos,sizeof(data),end_addr);
                set_global_dev_loss_controller_data((uint16_t)(data & 0x0000FFFF));

                //restore mSportHead to Ram
                check_and_restore(&mSportHead,current_pos, sizeof(SportsHead_t),end_addr);

                //restore mSportHead to Ram
                check_and_restore(&mSportsData,current_pos,sizeof(mSportsData),end_addr);

                //restore mSleepHead to Ram
                check_and_restore(&mSleepHead, current_pos, sizeof(SleepHead_t),end_addr);

                //restore mSleepData to Ram
                check_and_restore(&mSleepData.data,current_pos,sizeof(mSleepData.data),end_addr);
                
                //g_saved_SportsData
                check_and_restore(&g_saved_SportsData,current_pos,sizeof(g_saved_SportsData),end_addr);

                //g_saved_SleepData
                check_and_restore(&g_saved_SleepData,current_pos,sizeof(g_saved_SleepData),end_addr);

                //restore mSleepHead to Ram
                check_and_restore(sleep_setting_event_head, current_pos, sizeof(SleepHead_t),end_addr);

                //restore mSleepData to Ram
                check_and_restore(&sleep_setting_event->data,current_pos,sizeof(uint32_t),end_addr);
                if(sleep_setting_event->bits.mode == 1){
                    set_curr_sleep_flag(true);
                }
                else{
                    set_curr_sleep_flag(false);
                }
#ifdef DEBUG_LOG

                LOG(LEVEL_INFO, "len: %d:date: %d-%d-%d",sleep_setting_event_head->length, sleep_setting_event_head->Date.date.year,sleep_setting_event_head->Date.date.month, 
sleep_setting_event_head->Date.date.day);



                for(int ii = 0; ii < word_count+FLASH_PAGE_HEADER_LEN; ii ++)
                    LOG(LEVEL_INFO,"restore[%d]:0x%08x\n",ii,info_to_restore[ii]);
#endif
                //erase the page
                ble_flash_page_erase(FLASH_PAGE_STORE_TIME);

                return NRF_SUCCESS;

            }
        }

ERROR_HANDLE:

        //page is proper erased
        for(i = 0 ; i< (WORD_NUM_TO_BACK_UP + FLASH_PAGE_HEADER_LEN); ++i) {
            if(*(address + i) != 0xFFFFFFFF) {
                break;
            }
        }

        //page has been writted but not correct time so erae the page
        if(i <  (WORD_NUM_TO_BACK_UP + FLASH_PAGE_HEADER_LEN)) {
            ble_flash_page_erase(FLASH_PAGE_STORE_TIME);
        }
    }

    //reset wall clock
    set_wall_clock_time_counter(0);
    set_global_step_counts_today(0);
    set_global_dev_loss_controller_data(0);

    memset(&mSportHead, 0, sizeof(SportsHead_t));
    memset(&mSportsData, 0, sizeof(SportsData_U));
    memset(&mSleepHead, 0, sizeof(SleepHead_t));
    memset(&mSleepData, 0, sizeof(SleepData_U));
    set_curr_sleep_flag(false);
    memset(&g_saved_SportsData, 0, sizeof(Saved_Health_Data_Info_t));
    memset(&g_saved_SleepData, 0, sizeof(Saved_Health_Data_Info_t));

    return NRF_ERROR_INVALID_ADDR;

}


static void pre_restart_info_store(backup_info_restart_type_t restart_type)
{
    uint32_t info_to_backup[WORD_NUM_TO_BACK_UP + FLASH_PAGE_HEADER_LEN];
    
    uint16_t flash_crc = 0;
    uint32_t flash_header;
    uint32_t data;

    //store private bondinfo to flash, this may be used while restart before
    //first time bluetooth disconnected

    uint32_t * address =0;
    uint8_t  * current_pos = (uint8_t *)(info_to_backup + FLASH_PAGE_HEADER_LEN); 
    uint8_t  * end_addr = (uint8_t *)(info_to_backup + FLASH_PAGE_HEADER_LEN + WORD_NUM_TO_BACK_UP);

    SleepHead_t * sleep_setting_event_head = get_last_sleep_setting_event_head(); 
    SleepData_U * sleep_setting_event = get_last_sleep_setting_event();

    if(!check_has_bonded()){
        return;
    }

    save_curr_health_data();
    ble_flash_page_addr(FLASH_PAGE_STORE_TIME,&address);

    if(!address) { //Not use assert :)
        return;
    }

    uint16_t i = 0;

    for(; i < WORD_NUM_TO_BACK_UP + FLASH_PAGE_HEADER_LEN ; ++i) {// 2 words for flash header : crc + word_count
        if(*(address + i) != 0xFFFFFFFF) {
            break;
        }
    }

    if( i < WORD_NUM_TO_BACK_UP) { //which means there's some words not erased
        //FIXME: here should stop radio, and erase the page syncronize
        return ;
    }

    /******************* Begin to store info into array *********************/
    //store time to flash
    UTCTime counter = get_wall_clock_time_counter();
    if(restart_type == OTA_RESTART ) { //1 represents ota restart
        counter +=  60; // plugs 60S

        UTCTimeStruct time1,time2;
        ConvertToUTCTime(&time1,counter);
        ConvertToUTCTime(&time2,counter + 60);
        
        if(time1.day != time2.day) {
           set_global_step_counts_today(0); 
           set_daily_target_achieved(false);
        }
    } 

    check_and_backup(current_pos,&counter,sizeof(UTCTime),end_addr);
    
    //store step conunt to flash

    data = get_global_step_counts_today();
    check_and_backup(current_pos,&data,sizeof(uint32_t),end_addr);

    //store link loss config to flash
    data = (uint32_t)(get_global_dev_loss_controller_data() & 0x0000FFFF);
    check_and_backup(current_pos,&data,sizeof(uint32_t),end_addr);
    
    //store mSportHead to flash
    check_and_backup(current_pos,&mSportHead,sizeof(SportsHead_t),end_addr);

    //store mSportHead to flash
    check_and_backup(current_pos,&mSportsData,sizeof(mSportsData),end_addr);

    //store mSleepHead to flash
    check_and_backup(current_pos,&mSleepHead, sizeof(SleepHead_t),end_addr);

    //store mSleepData to flash
    check_and_backup(current_pos,&mSleepData.data,sizeof(mSleepData.data),end_addr);

    //g_saved_SportsData
    check_and_backup(current_pos,&g_saved_SportsData,sizeof(g_saved_SportsData),end_addr);

    //g_saved_SleepData
    check_and_backup(current_pos,&g_saved_SleepData,sizeof(g_saved_SleepData),end_addr);

    // store sleep_setting_event_head
    check_and_backup(current_pos, sleep_setting_event_head, sizeof(SleepHead_t),end_addr);

    //store mSleepData to flash
    check_and_backup(current_pos,&sleep_setting_event->data,sizeof(sleep_setting_event->data),end_addr);

    uint32_t word_count = ((current_pos - (uint8_t *)(info_to_backup + FLASH_PAGE_HEADER_LEN)) - 1) / sizeof(uint32_t) + 1; // align to 4
    if(word_count > WORD_NUM_TO_BACK_UP) {
        return ; //serious error 
    }

    LOG(LEVEL_INFO,"word to backup is %d",word_count);

    // Calculate CRC of the data to write.
    flash_crc = ble_flash_crc16_compute((uint8_t *)(info_to_backup + FLASH_PAGE_HEADER_LEN), (word_count * sizeof(uint32_t)) , NULL);
    
    // Write magic number and CRC to indicate that flash content is valid.
    flash_header = BLE_FLASH_MAGIC_NUMBER | (uint32_t)flash_crc;
    
    info_to_backup[0] = flash_header;
    info_to_backup[1] = word_count;

    // Write data
    for (i = 0 ; i < word_count + FLASH_PAGE_HEADER_LEN; i++) { 
        ble_flash_word_write(address + i, info_to_backup[i]);
#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"backup[%d]:0x%08x",i,info_to_backup[i]);
#endif
    }




}

void low_power_info_store(void)
{
    pre_restart_info_store(LOW_POWER_RESTART);
}


/********************************************************************************
* assert pre store info to flash
*********************************************************************************/
void assert_pre_restart_info_store(void)
{
    pre_restart_info_store(ASSERT_RESTART);
}

/**********************************************************************************
* OTA pre store info into flash
***********************************************************************************/
void ota_pre_restart_info_store(void)
{
    pre_restart_info_store(OTA_RESTART);
}


uint32_t check_is_flash_erased(uint32_t * address ,uint16_t wordCount);

// WARNING - DO NOT USE THIS FUNCTION IN END PRODUCT. - WARNING
// WARNING -         FOR DEBUG PURPOSES ONLY.         - WARNING
void ble_debug_assert_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // Copying parameters to static variables because parameters may not be accessible in debugger.
    static volatile uint8_t  s_file_name[MAX_LENGTH_FILENAME];
    static volatile uint16_t s_line_num;
    static volatile uint32_t s_error_code;

    uint8_t file_name_len = sizeof((const char *)p_file_name);
    if(file_name_len > MAX_LENGTH_FILENAME)
        file_name_len -= MAX_LENGTH_FILENAME;

    strncpy((char *)s_file_name, (const char *)(p_file_name+file_name_len), MAX_LENGTH_FILENAME - 1);
    s_file_name[MAX_LENGTH_FILENAME - 1] = '\0';
    s_line_num                           = line_num;
    s_error_code                         = error_code;
    UNUSED_VARIABLE(s_file_name);
    UNUSED_VARIABLE(s_line_num);
    UNUSED_VARIABLE(s_error_code);

    //disable radio
    radio_disable();

    // WARNING: The PRIMASK register is set to disable ALL interrups during writing the error log.
    //
    // Do not use __disable_irq() in normal operation.
    __disable_irq();

    // This function will write error code, filename, and line number to the flash.
    // In addition, the Cortex-M0 stack memory will also be written to the flash.

    uint32_t * log_page_address;
    ble_flash_page_addr(FLASH_PAGE_ERROR_LOG,&log_page_address);
    uint16_t  log_word_count = CEIL_DIV(sizeof(ble_error_log_data_t), sizeof(uint32_t));

    //write if the page has already been erased
    if(check_is_flash_erased(log_page_address,log_word_count) == NRF_SUCCESS ) {
        (void) ble_error_log_write(error_code, p_file_name, line_num);
    }

    assert_pre_restart_info_store();

    /****************************************************************************
    // For debug purposes, this function never returns.
    // Attach a debugger for tracing the error cause.
    for (;;)
    {
        // Do nothing.
    }
    *****************************************************************************/
}

