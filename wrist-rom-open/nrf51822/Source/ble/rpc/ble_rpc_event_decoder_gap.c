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

#include "ble_rpc_event_decoder_gap.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "app_util.h"
#include "ble_rpc_event_decoder.h"


/** @brief Function for decoding the event header id from an encoded event.
 *
 * @param[in]       p_evt       The pointer to the encoded event.
 * @param[in, out]  p_evt_hdr   The pointer to the decoded event header.
 *
 * @return          Number of bytes decoded.
 */
static uint8_t evt_header_id_decode(const uint8_t * const p_evt, ble_evt_hdr_t * const p_evt_hdr)
{
    uint32_t index = 0;

    p_evt_hdr->evt_id = uint16_decode(&(p_evt[index]));
    index            += sizeof(uint16_t);

    return index;
}


/** @brief Function for decoding a Bluetooth device address.
 *
 * @param[in]       p_encoded_ble_addr The pointer to the encoded Bluetooth device address.
 * @param[in, out]  p_addr             The pointer to the decoded address.
 *
 * @return          Number of bytes decoded.
 */
static uint8_t ble_addr_decode(const uint8_t * const  p_encoded_ble_addr,
                               ble_gap_addr_t * const p_addr)
{
    uint32_t index = 0;

    p_addr->addr_type = p_encoded_ble_addr[index++];

    memcpy(p_addr->addr, &(p_encoded_ble_addr[index]), BLE_GAP_ADDR_LEN);
    index += BLE_GAP_ADDR_LEN;

    return index;
}


/** @brief Function for decoding the BLE_GAP_EVT_CONNECTED event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_connected_evt_decode(const uint8_t * const           p_evt_data,
                                     ble_gap_evt_connected_t * const p_decoded_evt)
{
    uint32_t index = 0;

    index += ble_addr_decode(p_evt_data, &(p_decoded_evt->peer_addr));

    p_decoded_evt->irk_match      = (p_evt_data[index] & 0x01);
    p_decoded_evt->irk_match_idx  = (p_evt_data[index] & 0xFE) >> 1;
    index++;

    p_decoded_evt->conn_params.min_conn_interval = uint16_decode(&p_evt_data[index]);
    index                                       += sizeof(uint16_t);

    p_decoded_evt->conn_params.max_conn_interval = uint16_decode(&p_evt_data[index]);
    index                                       += sizeof(uint16_t);

    p_decoded_evt->conn_params.slave_latency     = uint16_decode(&p_evt_data[index]);
    index                                       += sizeof(uint16_t);

    p_decoded_evt->conn_params.conn_sup_timeout  = uint16_decode(&p_evt_data[index]);
}


/** @brief Function for decoding the BLE_GAP_EVT_DISCONNECTED event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_disconnected_evt_decode(const uint8_t * const              p_evt_data,
                                        ble_gap_evt_disconnected_t * const p_decoded_evt)
{
    uint32_t index = 0;

    index += ble_addr_decode(p_evt_data, &(p_decoded_evt->peer_addr));
    p_decoded_evt->reason = p_evt_data[index];
}


/** @brief Function for decoding the BLE_GAP_EVT_TIMEOUT event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_timeout_evt_decode(const uint8_t * const          p_evt_data,
                                   ble_gap_evt_timeout_t * const  p_decoded_evt)
{
    p_decoded_evt->src = p_evt_data[0];
}


/** @brief Function for decoding the BLE_GAP_EVT_CONN_PARAM_UPDATE event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_conn_param_update_evt_decode(
    const uint8_t * const                    p_evt_data,
    ble_gap_evt_conn_param_update_t * const  p_decoded_evt)
{
    uint32_t index = 0;

    p_decoded_evt->conn_params.min_conn_interval = uint16_decode(&p_evt_data[index]);
    index                                       += sizeof(uint16_t);

    p_decoded_evt->conn_params.max_conn_interval = uint16_decode(&p_evt_data[index]);
    index                                       += sizeof(uint16_t);

    p_decoded_evt->conn_params.slave_latency     = uint16_decode(&p_evt_data[index]);
    index                                       += sizeof(uint16_t);

    p_decoded_evt->conn_params.conn_sup_timeout  = uint16_decode(&p_evt_data[index]);
}


/** @brief Function for decoding the BLE_GAP_EVT_SEC_PARAMS_REQUEST event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_sec_params_request_evt_decode(
    const uint8_t * const                    p_evt_data,
    ble_gap_evt_sec_params_request_t * const p_decoded_evt)
{
    uint32_t index = 0;

    p_decoded_evt->peer_params.timeout = uint16_decode(&p_evt_data[index]);
    index                             += sizeof(uint16_t);

    p_decoded_evt->peer_params.bond    = (p_evt_data[index] >> 0) & 0x1;
    p_decoded_evt->peer_params.mitm    = (p_evt_data[index] >> 1) & 0x1;
    p_decoded_evt->peer_params.io_caps = (p_evt_data[index] >> 2) & 0x7;
    p_decoded_evt->peer_params.oob     = (p_evt_data[index] >> 5) & 0x1;
    index                             += sizeof(uint8_t);

    p_decoded_evt->peer_params.min_key_size = p_evt_data[index++];
    p_decoded_evt->peer_params.max_key_size = p_evt_data[index++];
}


/** @brief Function for decoding the BLE_GAP_EVT_AUTH_STATUS event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_auth_status_evt_decode(const uint8_t * const              p_evt_data,
                                       ble_gap_evt_auth_status_t * const  p_decoded_evt)
{
    uint32_t index = 0;

    p_decoded_evt->auth_status = p_evt_data[index++];
    p_decoded_evt->error_src   = p_evt_data[index++];

    p_decoded_evt->sm1_levels.lv3         = (p_evt_data[index] >> 5) & 0x01;
    p_decoded_evt->sm1_levels.lv2         = (p_evt_data[index] >> 4) & 0x01;
    p_decoded_evt->sm1_levels.lv1         = (p_evt_data[index] >> 3) & 0x01;
    p_decoded_evt->sm2_levels.lv3         = (p_evt_data[index] >> 2) & 0x01;
    p_decoded_evt->sm2_levels.lv2         = (p_evt_data[index] >> 1) & 0x01;
    p_decoded_evt->sm2_levels.lv1         = (p_evt_data[index] >> 0) & 0x01;
    index                                += sizeof(uint8_t);    
  
    p_decoded_evt->periph_kex.csrk        = (p_evt_data[index] >> 4) & 0x01;
    p_decoded_evt->periph_kex.address     = (p_evt_data[index] >> 3) & 0x01;
    p_decoded_evt->periph_kex.irk         = (p_evt_data[index] >> 2) & 0x01;
    p_decoded_evt->periph_kex.ediv_rand   = (p_evt_data[index] >> 1) & 0x01;
    p_decoded_evt->periph_kex.ltk         = (p_evt_data[index] >> 0) & 0x01;
    index                                += sizeof(uint8_t);  
    
    p_decoded_evt->central_kex.ltk        = (p_evt_data[index] >> 4) & 0x01;
    p_decoded_evt->central_kex.ediv_rand  = (p_evt_data[index] >> 3) & 0x01;
    p_decoded_evt->central_kex.irk        = (p_evt_data[index] >> 2) & 0x01;
    p_decoded_evt->central_kex.address    = (p_evt_data[index] >> 1) & 0x01;
    p_decoded_evt->central_kex.csrk       = (p_evt_data[index] >> 0) & 0x01;
    index                                += sizeof(uint8_t);  
    
    p_decoded_evt->periph_keys.enc_info.div = uint16_decode(&p_evt_data[index]);
    index                                  += sizeof(uint16_t);
    
    uint32_t i;
    for (i = 0; i < BLE_GAP_SEC_KEY_LEN; i++)
    {
        p_decoded_evt->periph_keys.enc_info.ltk[i] = p_evt_data[index++];
    }
    
    p_decoded_evt->periph_keys.enc_info.ltk_len = (p_evt_data[index] >> 1);
    p_decoded_evt->periph_keys.enc_info.auth    = (p_evt_data[index] >> 0) & 0x01;
    index                                      += sizeof(uint8_t); 
    
    for (i = 0; i < BLE_GAP_SEC_KEY_LEN; i++)
    {
        p_decoded_evt->central_keys.irk.irk[i] = p_evt_data[index++];
    }
    
    p_decoded_evt->central_keys.id_info.addr_type = p_evt_data[index++];

    for (i = 0; i < BLE_GAP_ADDR_LEN; i++)
    {
        p_decoded_evt->central_keys.id_info.addr[i] = p_evt_data[index++];
    }
}


/** @brief Function for decoding the BLE_GAP_EVT_SEC_INFO_REQUEST event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_sec_info_request_evt_decode(const uint8_t * const                  p_evt_data,
                                            ble_gap_evt_sec_info_request_t * const p_decoded_evt)
{
    uint32_t index = 0;

    p_decoded_evt->peer_addr.addr_type = p_evt_data[index++];
    p_decoded_evt->peer_addr.addr[0]   = p_evt_data[index++];
    p_decoded_evt->peer_addr.addr[1]   = p_evt_data[index++];
    p_decoded_evt->peer_addr.addr[2]   = p_evt_data[index++];
    p_decoded_evt->peer_addr.addr[3]   = p_evt_data[index++];
    p_decoded_evt->peer_addr.addr[4]   = p_evt_data[index++];
    p_decoded_evt->peer_addr.addr[5]   = p_evt_data[index++];

    p_decoded_evt->div = uint16_decode(&p_evt_data[index]);
    index             += sizeof(uint16_t);

    p_decoded_evt->enc_info            = (p_evt_data[index] >> 0) & 0x1;
    p_decoded_evt->id_info             = (p_evt_data[index] >> 1) & 0x1;
    p_decoded_evt->sign_info           = (p_evt_data[index] >> 2) & 0x1;
}


/** @brief Function for decoding the BLE_GAP_EVT_CONN_SEC_UPDATE event.
 *
 * @param[in]   p_evt_data      The pointer to the encoded event.
 * @param[out]  p_decoded_evt   The pointer to where the decoded event will be returned.
 */
static void gap_conn_sec_update_evt_decode(const uint8_t * const                 p_evt_data,
                                           ble_gap_evt_conn_sec_update_t * const p_decoded_evt)
{
    uint32_t index = 0;

    p_decoded_evt->conn_sec.sec_mode.sm = p_evt_data[index] & 0x0F;
    p_decoded_evt->conn_sec.sec_mode.lv = (p_evt_data[index] >> 4) & 0x0F;
    index++;

    p_decoded_evt->conn_sec.encr_key_size = p_evt_data[index++];
}


void ble_rpc_gap_evt_length_decode(uint8_t event_id, uint16_t * p_event_length)
{
    //lint -e526 -e628 -e516 -save // Symbol '__INTADDR__()' not defined
                                   // no argument information provided for function '__INTADDR__()'
                                   // Symbol '__INTADDR__()' has arg. type conflict
    switch (event_id)
    {
        // GAP events
        case BLE_GAP_EVT_CONNECTED:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t, evt.gap_evt.params.connected));
            *p_event_length += sizeof(ble_gap_evt_connected_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t, evt.gap_evt.params.disconnected));
            *p_event_length += sizeof(ble_gap_evt_disconnected_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t, evt.gap_evt.params.timeout));
            *p_event_length += sizeof(ble_gap_evt_timeout_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t,
                                          evt.gap_evt.params.conn_param_update));
            *p_event_length += sizeof(ble_gap_evt_conn_param_update_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t,
                                          evt.gap_evt.params.sec_params_request));
            *p_event_length += sizeof(ble_gap_evt_sec_params_request_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t, evt.gap_evt.params.auth_status));
            *p_event_length += sizeof(ble_gap_evt_auth_status_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t, evt.gap_evt.params.sec_info_request));
            *p_event_length += sizeof(ble_gap_evt_sec_info_request_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        case BLE_GAP_EVT_CONN_SEC_UPDATE:
            *p_event_length  = (uint16_t)(offsetof(ble_evt_t, evt.gap_evt.params.conn_sec_update));
            *p_event_length += sizeof(ble_gap_evt_conn_sec_update_t);
            *p_event_length -= sizeof(ble_evt_hdr_t);
            break;

        default:
            *p_event_length = 0;
            break;
    }
    //lint -restore
}


void ble_rpc_gap_evt_packet_decode(ble_evt_t *           p_ble_evt,
                                   uint8_t const * const p_packet)
{
    uint32_t index = 0;

    index += evt_header_id_decode(&p_packet[index], &(p_ble_evt->header));

    switch (p_ble_evt->header.evt_id)
    {
        // GAP events
        case BLE_GAP_EVT_CONNECTED:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_connected_evt_decode(&(p_packet[index]),
                                     &(p_ble_evt->evt.gap_evt.params.connected));
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_disconnected_evt_decode(&(p_packet[index]),
                                        &(p_ble_evt->evt.gap_evt.params.disconnected));
            break;

        case BLE_GAP_EVT_TIMEOUT:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_timeout_evt_decode(&(p_packet[index]), &(p_ble_evt->evt.gap_evt.params.timeout));
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_conn_param_update_evt_decode(&(p_packet[index]),
                                             &(p_ble_evt->evt.gap_evt.params.conn_param_update));
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_sec_params_request_evt_decode(&(p_packet[index]),
                                              &(p_ble_evt->evt.gap_evt.params.sec_params_request));
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_auth_status_evt_decode(&(p_packet[index]),
                                       &(p_ble_evt->evt.gap_evt.params.auth_status));
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_sec_info_request_evt_decode(&(p_packet[index]),
                                            &(p_ble_evt->evt.gap_evt.params.sec_info_request));
            break;

        case BLE_GAP_EVT_CONN_SEC_UPDATE:
            p_ble_evt->evt.gap_evt.conn_handle = uint16_decode(&(p_packet[index]));
            index                             += sizeof(p_ble_evt->evt.gap_evt.conn_handle);

            gap_conn_sec_update_evt_decode(&(p_packet[index]),
                                           &(p_ble_evt->evt.gap_evt.params.conn_sec_update));
            break;

        default:
            break;
    }
}
