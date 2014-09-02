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
#include "string.h"
#include "config.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "bd_private_bond.h"
#include "ble_flash.h"
#include "app_timer.h"
#include "bd_communicate_protocol.h"
#include "bd_input_event_source.h"
#include "bd_led_flash.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "bd_interaction.h"
#include "bd_level_drive_motor.h"
#include "bd_sync_data.h"

extern BLUETOOTH_BOND_STATE private_bond_machine;

#ifdef DEBUG_LOG
#include "simple_uart.h"
void simple_send_hex(uint8_t * content, uint8_t length);
#endif

extern void send_status_callback(SEND_STATUS status );
extern app_timer_id_t user_action_delay_timer;  /* wait for user action timeout timer */
extern uint8_t global_reponse_buffer[];
extern uint16_t global_connect_handle;

bool   should_update_prv_bond_info = false;
uint32_t user_id[(USER_ID_LENGTH - 1) / (4) + 1];

static bool global_device_has_bonded = false;
/**********************************************************************************
* check if already bonded to some phone
***********************************************************************************/
bool check_has_bonded(void)
{
    return global_device_has_bonded;
}

void set_device_has_bonded(bool value)
{
    global_device_has_bonded = value;
}

uint32_t bond_store_user_id(void)
{
    uint32_t  err_code;
    uint16_t  m_crc_bond_info;
    uint32_t  *mp_flash_bond_info;

    /* inialize crc check */
    m_crc_bond_info = ble_flash_crc16_compute(NULL, 0, NULL);

    // Find pointer to start of bond information flash block
    err_code = ble_flash_page_addr(FLASH_PAGE_USER_ID, &mp_flash_bond_info);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    /*FIXME: erase a page while bluetooth connected may cause disconnect, so this maybe stored when bluetooth disconnected*/
    /* Erase the whole page */
    ble_flash_page_erase(FLASH_PAGE_USER_ID);

    // Write private bonding Information
    err_code = ble_flash_block_write(mp_flash_bond_info + 1, //the first word is used to store crc
                                     (uint32_t *)user_id,
                                     ((USER_ID_LENGTH - 1) / (4) + 1));
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    m_crc_bond_info = ble_flash_crc16_compute((uint8_t *)user_id,
                      USER_ID_LENGTH,
                      &m_crc_bond_info);

    // Write header
    err_code = ble_flash_word_write(mp_flash_bond_info, BLE_FLASH_MAGIC_NUMBER | m_crc_bond_info);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return NRF_SUCCESS;
}

uint32_t bond_read_user_id(void)
{
    uint32_t  err_code;
    uint16_t  crc_header;
    uint16_t  m_crc_bond_info;
    uint32_t  *mp_flash_bond_info;
    uint32_t  header;


    // Initialize CRC
    m_crc_bond_info = ble_flash_crc16_compute(NULL, 0, NULL);

    // Find pointer to start of bond information flash block
    err_code = ble_flash_page_addr(FLASH_PAGE_USER_ID, &mp_flash_bond_info);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    //get crc
    header = *mp_flash_bond_info;

    if ((header & 0xFFFF0000U) == BLE_FLASH_MAGIC_NUMBER) {
        crc_header = (uint16_t)(header & 0x0000FFFFU);
    } else if (header == 0xFFFFFFFFU) {
        return NRF_ERROR_NOT_FOUND;
    } else {
        return NRF_ERROR_INVALID_DATA;
    }

    // Load master
    memcpy(user_id,mp_flash_bond_info + 1, USER_ID_LENGTH);

    // Check CRC
    m_crc_bond_info = ble_flash_crc16_compute((uint8_t *)user_id,
                      USER_ID_LENGTH,
                      &m_crc_bond_info);
    if (m_crc_bond_info == crc_header) {
        return NRF_SUCCESS;
    } else {
        return NRF_ERROR_INVALID_DATA;
    }
}

/************************************************************
* Tell the user bond success
*************************************************************/
void bond_success_event_dispatch(void)
{
#ifdef DEBUG_LOG
    //   simple_uart_putstring((const uint8_t *)"send bond success pack \r\n");
    LOG(LEVEL_INFO,"send bond success pack \n");
#endif

    L2_Send_Content sendContent;

    global_reponse_buffer[0] = BOND_COMMAND_ID;    /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;   /*L2 header version */
    global_reponse_buffer[2] = 0x02;         /*first key, bond response*/
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = 1;           /* length  = 1 */
    global_reponse_buffer[5] = 0;          /* bond success */

    sendContent.callback  = NULL;
    sendContent.content  = global_reponse_buffer;
    sendContent.length   = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4]; /*length of whole L2*/

    L1_send(&sendContent);
}

/**************************************************************************
* bond fail send callback
***************************************************************************/
static void bond_fail_callback(SEND_STATUS status )
{
    (void )status;

    //should disconnnectd with the peer
    sd_ble_gap_disconnect(global_connect_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

/************************************************************
* Tell the user bond fail
*************************************************************/
void bond_fail_event_dispatch(void)
{
    L2_Send_Content sendContent;

    global_reponse_buffer[0] = BOND_COMMAND_ID;    /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;   /*L2 header version */
    global_reponse_buffer[2] = 0x02;         /*first key, bond response*/
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = 1;           /* length  = 1 */
    global_reponse_buffer[5] = 0x01;         /* bond fail */

    sendContent.callback  = bond_fail_callback;
    sendContent.content  = global_reponse_buffer;
    sendContent.length   = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4]; /*length of whole L2*/

    L1_send(&sendContent);
}

/*********************************************************************
* Bond action triggerd by user
**********************************************************************/
void bond_press_handle(void )
{
    //stop timer
    app_timer_stop(user_action_delay_timer);
    //should store user id to flash when disconnected
    should_update_prv_bond_info = true;
    // erase flash, reset user profile,
    clear_prev_user_data();
    //stop led flash notify
    led_action_stop();

    bond_success_event_dispatch();

    notification_start(NOTIFICATION_BONDED,0);

    //reset input event handle
    //        reset_short_press_action_SM(INPUT_ACCEPT_BOND);

    //change bond status machine
    private_bond_machine =PRIVATE_BOND_SUCCESS;
    //notify new bond success
    bond_success_event_observer();

}

/*********************************************************************
* This function is used to check if the user id has been bonded
**********************************************************************/
uint32_t check_user_id_bonded(const uint8_t* user_id, uint8_t length)
{
    uint32_t err_code = NRF_SUCCESS;

    uint16_t  crc_header;
    uint16_t  m_crc_bond_info;
    uint32_t  *mp_flash_bond_info;
    uint32_t  header;


    if((!user_id) || (length != USER_ID_LENGTH)) {
        err_code = NRF_ERROR_INVALID_PARAM;
        return err_code;
    }

    // Initialize CRC
    m_crc_bond_info = ble_flash_crc16_compute(NULL, 0, NULL);

    // Find pointer to start of bond information flash block
    err_code = ble_flash_page_addr(FLASH_PAGE_USER_ID, &mp_flash_bond_info);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    //get crc
    header = *mp_flash_bond_info;

    if ((header & 0xFFFF0000U) == BLE_FLASH_MAGIC_NUMBER) {
        crc_header = (uint16_t)(header & 0x0000FFFFU);
    } else if (header == 0xFFFFFFFFU) {
        return NRF_ERROR_NOT_FOUND;
    } else {
        return NRF_ERROR_INVALID_DATA;
    }

    /* check crc in the flash */
    // Check CRC
    m_crc_bond_info = ble_flash_crc16_compute((uint8_t *)(mp_flash_bond_info + 1),
                      USER_ID_LENGTH,
                      &m_crc_bond_info);
    if (m_crc_bond_info != crc_header) {
        return NRF_ERROR_INVALID_DATA;
    }

    if (memcmp(user_id,(uint8_t *)(mp_flash_bond_info + 1),length) == 0) {
        err_code = NRF_SUCCESS;
    } else {
        err_code = NRF_ERROR_INVALID_DATA;
    }

    return err_code;
}

/************************************************************
* Tell the user login success
*************************************************************/
void login_success_event_dispatch(void)
{
    L2_Send_Content sendContent;

    global_reponse_buffer[0] = BOND_COMMAND_ID;    /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;   /*L2 header version */
    global_reponse_buffer[2] = 0x04;         /*first key, bond response*/
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = 1;           /* length  = 1 */
    global_reponse_buffer[5] = 0x00;         /* login success */

    sendContent.callback  = NULL;
    sendContent.content  = global_reponse_buffer;
    sendContent.length   = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4]; /*length of whole L2*/

    L1_send(&sendContent);
}

/************************************************************
* Tell the user login fail
*************************************************************/
void login_fail_event_dispatch(void)
{
    L2_Send_Content sendContent;

    global_reponse_buffer[0] = BOND_COMMAND_ID;    /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;   /*L2 header version */
    global_reponse_buffer[2] = 0x04;         /*first key, bond response*/
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = 1;           /* length  = 1 */
    global_reponse_buffer[5] = 0x01;         /* login fail */

    sendContent.callback  = NULL;
    sendContent.content  = global_reponse_buffer;
    sendContent.length   = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + global_reponse_buffer[4]; /*length of whole L2*/

    L1_send(&sendContent);
}
