/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
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
#include "ble_flash.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "nrf_soc.h"
#include "nordic_common.h"
#include "nrf_error.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_assert.h"
#include "app_util.h"
#include "app_timer.h"
#include "config.h"
#include "simple_uart.h"
#include "bd_sync_data.h"
#include "nrf_sdm.h"
#include "nrf_delay.h"
#include "bd_wall_clock_timer.h"
#include "ble_radio_notification.h"

#define ERASABLE_PAGE_CHECK(page_num)    ASSERT((page_num >= ERASABLE_BLOCK_START) && (page_num < ERASABLE_BLOCK_END))

app_timer_id_t m_erase_flash_timer_id;

static volatile bool m_radio_active = false;  /**< TRUE if radio is active (or about to become active), FALSE otherwise. */
static volatile bool erase_op_scheduled = false;    /* delay erase op has scheduled or not */

uint16_t ble_flash_crc16_compute(uint8_t * p_data, uint16_t size, uint16_t * p_crc)
{
    uint16_t i;
    uint16_t crc = (p_crc == NULL) ? 0xffff : *p_crc;

    for (i = 0; i < size; i++) {
        crc  = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= p_data[i];
        crc ^= (unsigned char)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }
    return crc;
}


/**@brief Function for erasing a page in flash.
 *
 * @param[in]  p_page  Pointer to first word in page to be erased.
 */
static void flash_page_erase(uint32_t * p_page)
{

    // Turn on flash erase enable and wait until the NVMC is ready.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }

    // Erase page.
    NRF_NVMC->ERASEPAGE = (uint32_t)p_page;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }

    // Turn off flash erase enable and wait until the NVMC is ready.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing
    }
}


/**@brief Function for writing one word to flash. Unprotected write, which can interfere with radio communication.
 *
 * @details This function DOES NOT use the m_radio_active variable, but will force the write even
 *          when the radio is active. To be used only from @ref ble_flash_page_write.
 *
 * @note Flash location to be written must have been erased previously.
 *
 * @param[in]  p_address   Pointer to flash location to be written.
 * @param[in]  value       Value to write to flash.
 */
static void flash_word_unprotected_write(uint32_t * p_address, uint32_t value)
{
    // Turn on flash write enable and wait until the NVMC is ready.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }
    *p_address = value;

    // Wait flash write to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }

    // Turn off flash write enable and wait until the NVMC is ready.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }
}


/**@brief Function for writing one word to flash.
 *
 * @note Flash location to be written must have been erased previously.
 *
 * @param[in]  p_address   Pointer to flash location to be written.
 * @param[in]  value       Value to write to flash.
 */
static void flash_word_write(uint32_t * p_address, uint32_t value)
{
    // If radio is active, wait for it to become inactive.
    while (m_radio_active) {
        // Do nothing (just wait for radio to become inactive).
        (void) sd_app_event_wait();
    }

    // Turn on flash write enable and wait until the NVMC is ready.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }

    *p_address = value;
    // Wait flash write to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing.
    }
    // Turn off flash write enable and wait until the NVMC is ready.
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        // Do nothing
    }
}


uint32_t ble_flash_word_write(uint32_t * p_address, uint32_t value)
{
    flash_word_write(p_address, value);
    return NRF_SUCCESS;
}


uint32_t ble_flash_block_write(uint32_t * p_address, uint32_t * p_in_array, uint16_t word_count)
{
    uint16_t i;

    for (i = 0; i < word_count; i++) {
        flash_word_write(p_address, p_in_array[i]);
        p_address++;
    }

    return NRF_SUCCESS;
}


uint32_t ble_flash_page_erase(uint8_t page_num)
{
    uint32_t * p_page = (uint32_t *)(BLE_FLASH_PAGE_SIZE * page_num);
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"page_num:[%d]\n",page_num);
#endif
    ERASABLE_PAGE_CHECK(page_num);

    flash_page_erase(p_page);

    return NRF_SUCCESS;
}

/*******************************************************************************************
* This function is used to write content to a flash page which has already been
* erased, this is the main difference to ble_flash_page_write
********************************************************************************************/
uint32_t bd_flash_write_erased_page(uint8_t page_num ,uint32_t *p_in_array,uint8_t word_count)
{
    //check is page already erased
    ERASABLE_PAGE_CHECK(page_num);

    ASSERT(word_count <= (BLE_FLASH_PAGE_SIZE/sizeof(uint32_t) - FLASH_PAGE_HEADER_LEN));

    uint8_t     i = 0;
    uint32_t *  address = 0;
    uint16_t    flash_crc;
    uint32_t    flash_header;

    ble_flash_page_addr(page_num,&address);

    for(i = 0; i< (word_count + FLASH_PAGE_HEADER_LEN); ++i ) {
        if(*(address + i) != 0xFFFFFFFF) {
            break;
        }
    }

    if( i< (word_count + FLASH_PAGE_HEADER_LEN) ) {
        LOG(LEVEL_ERROR,"want to write a flash page which is not erased");
        return NRF_ERROR_INVALID_ADDR;
    }

    for(i = 0; i < word_count ; ++i) {
        ble_flash_word_write((address + i + FLASH_PAGE_HEADER_LEN), p_in_array[i] );
    }

    //write flash header
    flash_crc = ble_flash_crc16_compute((uint8_t *)p_in_array,word_count * sizeof(uint32_t),NULL);

    flash_header = BLE_FLASH_MAGIC_NUMBER | (uint32_t)flash_crc;
    ble_flash_word_write(address,flash_header);

    //write flash word_count
    ble_flash_word_write(address + 1, word_count);

    return NRF_SUCCESS;

}

/***********************************************************************************
* cooperated with the above Function
************************************************************************************/
uint32_t bd_flash_read_page(uint8_t page_num, uint32_t * p_out_array, uint8_t * p_word_count)
{
    return  ble_flash_page_read(page_num,p_out_array,p_word_count);
}


/***********************************************************************************
* this function is used to check whether the given flash block is erased
************************************************************************************/
uint32_t check_is_flash_erased(uint32_t * address ,uint16_t wordCount)
{
    uint16_t index = 0;

    for( ; index < wordCount; index ++) {
        if(*(address + index) != 0xFFFFFFFF) {
            return NRF_ERROR_INVALID_STATE;
        }
    }

    return NRF_SUCCESS;
}


uint32_t ble_flash_page_write(uint8_t page_num, uint32_t * p_in_array, uint8_t word_count)
{
    int        i;
    uint32_t * p_page;
    uint32_t * p_curr_addr;
    uint16_t   in_data_crc;
    uint16_t   flash_crc;
    uint32_t   flash_header;

    p_page      = (uint32_t *)(BLE_FLASH_PAGE_SIZE * page_num);
    p_curr_addr = p_page;

    // Calculate CRC of the data to write.
    in_data_crc = ble_flash_crc16_compute((uint8_t *)p_in_array,
                                          word_count * sizeof(uint32_t),
                                          NULL);

    // Compare the calculated to the one in flash.
    flash_header = *p_curr_addr;
    flash_crc    = (uint16_t)flash_header;

    if (flash_crc == in_data_crc) {
        // Data is the same as the data already stored in flash, return without modifying flash.
        return NRF_SUCCESS;
    }

    // Erase flash page
    flash_page_erase(p_page);

    // Reserve space for magic number (for detecting if flash content is valid).
    p_curr_addr++;

    // Reserve space for saving word_count.
    p_curr_addr++;

    // Write data
    for (i = 0; i < word_count; i++) {
        flash_word_unprotected_write(p_curr_addr, p_in_array[i]);
        p_curr_addr++;
    }

    // Write number of elements.
    flash_word_write(p_page + 1, (uint32_t)(word_count));

    // Write magic number and CRC to indicate that flash content is valid.
    flash_header = BLE_FLASH_MAGIC_NUMBER | (uint32_t)in_data_crc;
    flash_word_write(p_page, flash_header);

    return NRF_SUCCESS;
}


uint32_t ble_flash_page_read(uint8_t page_num, uint32_t * p_out_array, uint8_t * p_word_count)
{
    int        byte_count;
    uint32_t * p_page;
    uint32_t * p_curr_addr;
    uint32_t   flash_header;
    uint32_t   calc_header;
    uint16_t   calc_crc;
    uint32_t   tmp;

    p_page      = (uint32_t *)(BLE_FLASH_PAGE_SIZE * page_num);
    p_curr_addr = p_page;

    // Check if block is valid
    flash_header = *p_curr_addr;
    tmp = flash_header & 0xFFFF0000;
    if (tmp != BLE_FLASH_MAGIC_NUMBER) {
        *p_word_count = 0;
        return NRF_ERROR_NOT_FOUND;
    }
    p_curr_addr++;

    // Read number of elements
    *p_word_count = (uint8_t)(*(p_curr_addr));
    p_curr_addr++;

    // Read data
    byte_count = (*p_word_count) * sizeof(uint32_t);
    memcpy(p_out_array, p_curr_addr, byte_count);

    // Check CRC
    calc_crc  = ble_flash_crc16_compute((uint8_t *)p_out_array,
                                        (*p_word_count) * sizeof(uint32_t),
                                        NULL);
    calc_header = BLE_FLASH_MAGIC_NUMBER | (uint32_t)calc_crc;

    if (calc_header != flash_header) {
        return NRF_ERROR_NOT_FOUND;
    }

    return NRF_SUCCESS;
}


uint32_t ble_flash_page_addr(uint8_t page_num, uint32_t ** pp_page_addr)
{
    *pp_page_addr = (uint32_t *)(BLE_FLASH_PAGE_SIZE * page_num);
    return NRF_SUCCESS;
}

#define ERASE_Q_MAX_LEN 8
typedef struct flash_page_to_erase {
    uint8_t page_num;
} flash_page_to_erase_t;

flash_page_to_erase_t flash_page_to_erase_Q[ERASE_Q_MAX_LEN]; // [page_num][delay_time]

uint8_t flash_page_to_erase_Q_len = 0;

bool ble_flash_page_delay_erase_finished(uint8_t page_num)
{
    uint8_t index;
    for(index = 0; index < flash_page_to_erase_Q_len; index ++) {

#ifdef DEBUG_LOG
        LOG(LEVEL_INFO,"--check erase_finished: %d/%d\r\n",index,flash_page_to_erase_Q_len);
#endif
        if(page_num == flash_page_to_erase_Q[index].page_num)
            return false;
    }

    return true;
}

uint32_t ble_flash_page_delay_erase(uint8_t page_num)
{
    uint8_t index;

    for(index = 0; index < flash_page_to_erase_Q_len; index ++) { // if this page aready exist here, ignore new request
        if(page_num == flash_page_to_erase_Q[index].page_num)
            return NRF_SUCCESS;
    }

    if(!((page_num >= ERASABLE_BLOCK_START) && (page_num < ERASABLE_BLOCK_END))) {
        LOG(LEVEL_ERROR, "want to erase illegal page num is %d",page_num);
        return NRF_ERROR_INVALID_ADDR;
    }

    if(get_global_bluetooth_status() == NotWork) { //if bluetooth not work just erase the page 
        return ble_flash_page_erase(page_num);
    }

    //will put it into the queue
    if(flash_page_to_erase_Q_len == ERASE_Q_MAX_LEN) {
        return NRF_ERROR_NO_MEM;
    }

    flash_page_to_erase_Q[flash_page_to_erase_Q_len ++ ].page_num= page_num;

#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"-------delay_erase page:%d len: %d\r\n",flash_page_to_erase_Q[flash_page_to_erase_Q_len-1].page_num,flash_page_to_erase_Q_len);
#endif

    return NRF_SUCCESS;
}


uint32_t radio_inactive_time;

/************************************************************************************
* erase when slow advertising & long connection interval 
*************************************************************************************/
#define ERASE_LIMIT    APP_TIMER_TICKS(400, APP_TIMER_PRESCALER) 
#define MAX_RTC_COUNTER_VAL     0x00FFFFFF 

void ble_flash_erase_data_page(void * p_event_data, uint16_t event_size)
{
    uint32_t inactive_last_time;

    //do not need lock
    erase_op_scheduled = false;

    //add this jusge for assurance, because this function is scheduled while
    //radio is working, so   get_global_bluetooth_status() == NotWork may never
    //occure
    if(get_global_bluetooth_status() == NotWork) {
        
        ble_flash_page_erase(flash_page_to_erase_Q[flash_page_to_erase_Q_len - 1].page_num);

        sync_data_to_flash(flash_page_to_erase_Q[flash_page_to_erase_Q_len - 1].page_num);
        flash_page_to_erase_Q_len --;

        if(flash_page_to_erase_Q_len >0 ) {
            app_sched_event_put(&(flash_page_to_erase_Q_len),sizeof(flash_page_to_erase_Q_len),ble_flash_erase_data_page);
        }

        return;

    }

    if((get_global_bluetooth_status() == SlowAdvertising) || (get_global_bluetooth_status() == LongConnectInterval)) {

        if(NRF_RTC1->COUNTER > radio_inactive_time)
            inactive_last_time = NRF_RTC1->COUNTER - radio_inactive_time;
        else
            inactive_last_time = NRF_RTC1->COUNTER - radio_inactive_time + MAX_RTC_COUNTER_VAL;

        if (inactive_last_time > ERASE_LIMIT ) {
            return;
        } else {
#ifdef DEBUG_LOG
            LOG(LEVEL_INFO,"inactive_time:%d\r\n",inactive_last_time);
            LOG(LEVEL_INFO,"erase needs: %d\r\n",NRF_RTC1->COUNTER);
            LOG(LEVEL_INFO,"len:%d,page: %d\r\n",flash_page_to_erase_Q_len, flash_page_to_erase_Q[flash_page_to_erase_Q_len - 1].page_num);
#endif
            ble_flash_page_erase(flash_page_to_erase_Q[flash_page_to_erase_Q_len - 1].page_num);

            sync_data_to_flash(flash_page_to_erase_Q[flash_page_to_erase_Q_len - 1].page_num);
            flash_page_to_erase_Q_len --;
        }
    }

}

bool ble_radio_active(void)
{
    return m_radio_active;
}

void ble_flash_on_radio_active_evt(bool radio_active)
{
    m_radio_active = radio_active;
    
    if( !m_radio_active && flash_page_to_erase_Q_len && ((get_global_bluetooth_status() == SlowAdvertising) || (get_global_bluetooth_status() == LongConnectInterval))) {
        
        if(erase_op_scheduled == false) {
            erase_op_scheduled = true;
            radio_inactive_time = NRF_RTC1->COUNTER;

            LOG(LEVEL_INFO,"-m_radio_active: %d,rtc: %d\r\n",m_radio_active,radio_inactive_time);
            app_sched_event_put(&(flash_page_to_erase_Q_len),sizeof(flash_page_to_erase_Q_len),ble_flash_erase_data_page);
        }
    }
}

void radio_disable(void)
{
    sd_softdevice_disable();

    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TEST            = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
        nrf_delay_ms(5);
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
    
    //should only be called here
    set_radio_active_flag(false);
}

/*******************************************************************************
* This function should only be called after stop radio 
********************************************************************************/
void set_radio_active_flag(bool value)
{
    if(value == false) {
        m_radio_active = false;
    }
}
