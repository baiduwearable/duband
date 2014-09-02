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

#include "ble_rpc_event_encoder_gatts.h"
#include "ble_rpc_defines.h"
#include "app_util.h"
#include <string.h>


/** @brief Function for encoding the BLE_GATTS_EVT_WRITE event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gatts_write_evt_encode(const ble_evt_t * const p_ble_evt,
                                       uint8_t * const         p_buffer)
{
    uint32_t                      index = 0;
    const ble_gatts_evt_write_t * p_evt_write;

    p_evt_write = &(p_ble_evt->evt.gatts_evt.params.write);

    index            += uint16_encode(p_evt_write->handle, &p_buffer[index]);
    p_buffer[index++] = p_evt_write->op;
    index            += uint16_encode(p_evt_write->context.srvc_uuid.uuid, &p_buffer[index]);
    p_buffer[index++] = p_evt_write->context.srvc_uuid.type;
    index            += uint16_encode(p_evt_write->context.char_uuid.uuid, &p_buffer[index]);
    p_buffer[index++] = p_evt_write->context.char_uuid.type;
    index            += uint16_encode(p_evt_write->context.desc_uuid.uuid, &p_buffer[index]);
    p_buffer[index++] = p_evt_write->context.desc_uuid.type;
    index            += uint16_encode(p_evt_write->context.srvc_handle, &p_buffer[index]);
    index            += uint16_encode(p_evt_write->context.value_handle, &p_buffer[index]);
    p_buffer[index++] = p_evt_write->context.type;
    index            += uint16_encode(p_evt_write->offset, &p_buffer[index]);
    index            += uint16_encode(p_evt_write->len, &p_buffer[index]);

    if (p_evt_write->len != 0)
    {
        memcpy(&(p_buffer[index]), p_evt_write->data, p_evt_write->len);
        index += p_evt_write->len;
    }

    return index;
}


/** @brief Function for encoding the BLE_GATTS_EVT_SYS_ATTR_MISSING event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gatts_sys_attr_missing_encode(const ble_evt_t * const p_ble_evt,
                                              uint8_t * const         p_buffer)
{
    uint32_t index    = 0;
    p_buffer[index++] = p_ble_evt->evt.gatts_evt.params.sys_attr_missing.hint;
    
    return index;
}


uint32_t ble_rpc_evt_gatts_encode(ble_evt_t * p_ble_evt, uint8_t * p_buffer)
{
    uint32_t index    = 0;
    // Encode packet type.
    p_buffer[index++] = BLE_RPC_PKT_EVT;

    // Encode header.
    index += uint16_encode(p_ble_evt->header.evt_id,  &p_buffer[index]);

    // Encode common part of the event.
    index += uint16_encode(p_ble_evt->evt.gatts_evt.conn_handle, &p_buffer[index]);

    // Encode events.
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            index += gatts_write_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            index += gatts_sys_attr_missing_encode(p_ble_evt, &p_buffer[index]);
            break;

        default:
            break;
    }

    return index;
}

