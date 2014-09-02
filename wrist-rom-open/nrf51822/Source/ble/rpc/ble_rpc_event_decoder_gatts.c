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

#include "ble_rpc_event_decoder_gatts.h"
#include <stdint.h>
#include <stddef.h>
#include "app_util.h"

#define BLE_GATTS_EVT_WRITE_LEN_POSITION 23 /**< Position in encoded packet for the variable length field in the GATTS write event. */


/** @brief Function for decoding the event header id from an encoded event.
 *
 * @param[in]   p_evt       The pointer to the encoded event.
 * @param[out]  p_evt_hdr   The pointer to where the decoded event header will be stored.
 *
 * @return Number of bytes decoded.
 */
static uint8_t evt_header_id_decode(const uint8_t * const p_evt, ble_evt_hdr_t * const p_evt_hdr)
{
    uint32_t index = 0;

    p_evt_hdr->evt_id  = uint16_decode(&(p_evt[index]));
    index             += sizeof(uint16_t);

    return index;
}


/** @brief Function for decoding the BLE_GATTS_EVT_WRITE event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be stored.
 */
static void gatts_write_evt_decode(const uint8_t * const         p_evt_data,
                                   ble_gatts_evt_write_t * const p_decoded_evt)
{
    uint32_t index = 0;

    p_decoded_evt->handle                 = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);

    p_decoded_evt->op                     = p_evt_data[index++];

    p_decoded_evt->context.srvc_uuid.uuid = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);
    p_decoded_evt->context.srvc_uuid.type = p_evt_data[index++];

    p_decoded_evt->context.char_uuid.uuid = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);
    p_decoded_evt->context.char_uuid.type = p_evt_data[index++];

    p_decoded_evt->context.desc_uuid.uuid = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);
    p_decoded_evt->context.desc_uuid.type = p_evt_data[index++];

    p_decoded_evt->context.srvc_handle    = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);

    p_decoded_evt->context.value_handle   = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);

    p_decoded_evt->context.type           = p_evt_data[index++];

    p_decoded_evt->offset                 = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);

    p_decoded_evt->len                    = uint16_decode(&p_evt_data[index]);
    index += sizeof(uint16_t);

    uint16_t data_index;
    for (data_index = 0; data_index < p_decoded_evt->len; data_index++)
    {
        p_decoded_evt->data[data_index]   = p_evt_data[index + data_index];
    }
}


void ble_rpc_gatts_evt_length_decode(uint8_t               event_id,
                                     uint16_t *            p_event_length,
                                     uint8_t const * const p_packet)
{
    //lint -e526 -e628 -e516 -save // Symbol '__INTADDR__()' not defined
                                   // no argument information provided for function '__INTADDR__()'
                                   // Symbol '__INTADDR__()' has arg. type conflict
    switch (event_id)
    {
        // GATTS events.
        case BLE_GATTS_EVT_WRITE:
            *p_event_length = (uint16_t)(offsetof(ble_evt_t,
                                         evt.gatts_evt.params.write.data));

            // Additional data, One byte is already counted in the structure itself.
            *p_event_length += uint16_decode(&p_packet[BLE_GATTS_EVT_WRITE_LEN_POSITION]);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            *p_event_length = (uint16_t)(offsetof(ble_evt_t,
                                         evt.gatts_evt.params.sys_attr_missing));

            *p_event_length += sizeof(ble_gatts_evt_sys_attr_missing_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        default:
            *p_event_length = 0;
            break;
    }
    //lint -restore
}


void ble_rpc_gatts_evt_packet_decode(ble_evt_t *           p_ble_evt,
                                     uint8_t const * const p_packet)
{
    uint32_t index = 0;

    index += evt_header_id_decode(&p_packet[index], &(p_ble_evt->header));

    switch (p_ble_evt->header.evt_id)
    {
        // GATTS events
        case BLE_GATTS_EVT_WRITE:
            p_ble_evt->evt.gatts_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index += sizeof(p_ble_evt->evt.gatts_evt.conn_handle);

            gatts_write_evt_decode(&(p_packet[index]), &(p_ble_evt->evt.gatts_evt.params.write));
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            p_ble_evt->evt.gatts_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index += sizeof(p_ble_evt->evt.gatts_evt.conn_handle);

            p_ble_evt->evt.gatts_evt.params.sys_attr_missing.hint = p_packet[index++];
            break;

        default:
            break;
    }
}



