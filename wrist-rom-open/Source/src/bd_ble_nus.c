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

#include "bd_ble_nus.h"
#include <string.h>
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"
#include "nrf_gpio.h"
#include "bd_communicate_protocol.h"

#ifdef DEBUG_LOG
    #include "simple_uart.h"
    #include <string.h>
    #include <stdio.h>
#endif

static SendCompletePara m_send_complete_para =  {NULL, NULL,TASK_NONE};
void set_complete_callback(SendCompletePara para)
{
    m_send_complete_para.callback = para.callback;
    m_send_complete_para.context = para.context;
    m_send_complete_para.task_type = para.task_type;
}

/**@brief Connect event handler.
 *
 * @param[in]   p_nus       Nordic UART Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_nus_t * p_nus, ble_evt_t * p_ble_evt)
{
    p_nus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;


    if(p_nus->connect_handler != NULL) {
        p_nus->connect_handler();
    }
}


/**@brief Disconnect event handler.
 *
 * @param[in]   p_nus       Nordic UART Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_nus_t * p_nus, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_nus->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_nus->is_notification_enabled = false;
    bluetooth_l0_reset();
}


/**@brief Write event handler.
 *
 * @param[in]   p_nus       Nordic UART Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_nus_t * p_nus, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (
        (p_evt_write->handle == p_nus->rx_handles.cccd_handle)
        &&
        (p_evt_write->len == 2)
    ) {
        uint16_t value = *(uint16_t *) p_evt_write->data;
        if (value == 0x001) {
            p_nus->is_notification_enabled = true;
        } else {
            p_nus->is_notification_enabled = false;
        }

    } else if ((p_evt_write->handle == p_nus->tx_handles.value_handle) &&
               (p_nus->data_handler != NULL)) {
        p_nus->data_handler(p_nus, p_evt_write->data, p_evt_write->len);
    }
}

static void on_tx_complete(ble_nus_t * p_nus, ble_evt_t * p_ble_evt)
{

    //if there's event need to schedule
    if(m_send_complete_para.callback) {
        m_send_complete_para.callback(m_send_complete_para.context,m_send_complete_para.task_type);
    }

}

void ble_nus_on_ble_evt(ble_nus_t * p_nus, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_nus, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_nus, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_nus, p_ble_evt);
            break;
        case BLE_EVT_TX_COMPLETE:
            on_tx_complete(p_nus,p_ble_evt);
            break;

        default:
            break;
    }
}


/**@brief Add RX characteristic.
 *
 * @param[in]   p_nus        Nordic UART Service structure.
 * @param[in]   p_nus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t rx_char_add(ble_nus_t * p_nus, const ble_nus_init_t * p_nus_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    // Add Battery Level characteristic
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = p_nus->uuid_type;
    ble_uuid.uuid = BLE_UUID_NUS_RX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = 20;

    return sd_ble_gatts_characteristic_add(p_nus->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_nus->rx_handles);

}

/**@brief Add TX characteristic.
 *
 * @param[in]   p_nus        Nordic UART Service structure.
 * @param[in]   p_nus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t tx_char_add(ble_nus_t * p_nus, const ble_nus_init_t * p_nus_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write  = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = p_nus->uuid_type;
    ble_uuid.uuid = BLE_UUID_NUS_TX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = 1;
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = 20;

    return sd_ble_gatts_characteristic_add(p_nus->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_nus->tx_handles);
}

uint32_t ble_nus_init(ble_nus_t * p_nus, const ble_nus_init_t * p_nus_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;
    ble_uuid128_t nus_base_uuid = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x00, 0x00, 0x40, 0x6E};

    // Initialize service structure
    p_nus->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_nus->data_handler = p_nus_init->data_handler;
    p_nus->connect_handler = p_nus_init->connect_handler;
    p_nus->is_notification_enabled = false;


    // Add custom base UUID
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_nus->uuid_type);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    // Add service
    ble_uuid.type = p_nus->uuid_type;
    ble_uuid.uuid = BLE_UUID_NUS_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_nus->service_handle);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    err_code = rx_char_add(p_nus, p_nus_init);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    err_code = tx_char_add(p_nus, p_nus_init);
    if (err_code != NRF_SUCCESS) {
        return err_code;
    }

    return NRF_SUCCESS;
}


uint32_t ble_nus_send_string(ble_nus_t * p_nus, uint8_t * string, uint16_t *length)
{
    ble_gatts_hvx_params_t hvx_params;

    if (!p_nus->is_notification_enabled) {
        return NRF_ERROR_INVALID_STATE;
    }

    if (*length > NUS_MAX_DATA_LENGTH) {
        return NRF_ERROR_INVALID_PARAM;
    }

#ifdef DEBUG_LOG
    uint8_t dumpContent[41];
    uint32_t i = 0;
    for(;i<(*length)*2;i+=2) {
        sprintf(((char *)dumpContent) + i,"%.2x",string[i/2]);
    }
    dumpContent[(*length)*2] = 0; //string end

    if(string[0] == 0xAB && string[1] == 0x10) {
        LOG(LEVEL_INFO,"res pack & con is:%s\r\n",dumpContent);
    } else {
        LOG(LEVEL_INFO,"data pack & con is:%s\r\n",dumpContent);
    }
#endif

    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = p_nus->rx_handles.value_handle;
    hvx_params.p_data = string;
    hvx_params.p_len  = length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_nus->conn_handle, &hvx_params);
}
