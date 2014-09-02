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

#include "ble_rpc_event_encoder_gap.h"
#include "ble_rpc_defines.h"
#include "app_util.h"
#include "ble_gap.h"


/**@brief Function for encoding a peer address.
 *
 * @param[out]  p_buffer        Pointer to the buffer used for the encoded ble_gap_addr_t.
 * @param[in]   p_peer_address  Pointer to the structure to be encoded.
 *
 * @return      Number of bytes encoded.
 */
static uint32_t peer_address_encode(uint8_t * const              p_buffer,
                                    const ble_gap_addr_t * const p_peer_address)
{
    uint32_t i;    
    uint32_t index = 0;

    p_buffer[index++] = p_peer_address->addr_type;
    for (i = 0; i < BLE_GAP_ADDR_LEN; i++)
    {
        p_buffer[index++] = p_peer_address->addr[i];
    }

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_CONNECTED event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_connected_evt_encode(const ble_evt_t * const p_ble_evt,
                                         uint8_t * const         p_buffer)
{
    uint32_t index = 0;

    index += peer_address_encode(&p_buffer[index],
                                 &(p_ble_evt->evt.gap_evt.params.connected.peer_addr));

    p_buffer[index]  = (p_ble_evt->evt.gap_evt.params.connected.irk_match_idx << 1) & 0xFE;
    p_buffer[index] |= p_ble_evt->evt.gap_evt.params.connected.irk_match;
    index++;

    const ble_gap_conn_params_t * p_conn_params;
    p_conn_params = &(p_ble_evt->evt.gap_evt.params.connected.conn_params);

    index += uint16_encode(p_conn_params->min_conn_interval, &p_buffer[index]);
    index += uint16_encode(p_conn_params->max_conn_interval, &p_buffer[index]);
    index += uint16_encode(p_conn_params->slave_latency,     &p_buffer[index]);
    index += uint16_encode(p_conn_params->conn_sup_timeout,  &p_buffer[index]);

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_DISCONNECTED event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_disconnected_evt_encode(const ble_evt_t * const p_ble_evt,
                                            uint8_t * const         p_buffer)
{
    uint32_t index = 0;

    index += peer_address_encode(&p_buffer[index],
                                 &(p_ble_evt->evt.gap_evt.params.disconnected.peer_addr));
    p_buffer[index++] = p_ble_evt->evt.gap_evt.params.disconnected.reason;

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_TIMEOUT event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_timeout_evt_encode(const ble_evt_t * const p_ble_evt,
                                       uint8_t * const         p_buffer)
{
    uint32_t index = 0;

    p_buffer[index++] = p_ble_evt->evt.gap_evt.params.timeout.src;

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_CONN_PARAM_UPDATE event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_conn_param_upd_evt_encode(const ble_evt_t * const p_ble_evt,
                                              uint8_t * const         p_buffer)
{
    uint32_t                      index = 0;
    const ble_gap_conn_params_t * p_conn_params;

    p_conn_params = &(p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params);

    index += uint16_encode(p_conn_params->min_conn_interval, &p_buffer[index]);
    index += uint16_encode(p_conn_params->max_conn_interval, &p_buffer[index]);
    index += uint16_encode(p_conn_params->slave_latency,     &p_buffer[index]);
    index += uint16_encode(p_conn_params->conn_sup_timeout,  &p_buffer[index]);

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_SEC_PARAMS_REQUEST event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_sec_param_req_evt_encode(const ble_evt_t * const p_ble_evt,
                                             uint8_t * const         p_buffer)
{
    uint32_t                                 index = 0;
    const ble_gap_evt_sec_params_request_t * p_sec_params_request;

    p_sec_params_request = &(p_ble_evt->evt.gap_evt.params.sec_params_request);

    index += uint16_encode(p_sec_params_request->peer_params.timeout, &p_buffer[index]);

    p_buffer[index++] = (p_sec_params_request->peer_params.oob     << 5) |
                        (p_sec_params_request->peer_params.io_caps << 2) |
                        (p_sec_params_request->peer_params.mitm    << 1) |
                        (p_sec_params_request->peer_params.bond);

    p_buffer[index++] = p_sec_params_request->peer_params.min_key_size;
    p_buffer[index++] = p_sec_params_request->peer_params.max_key_size;

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_SEC_INFO_REQUEST event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_sec_info_req_evt_encode(const ble_evt_t * const p_ble_evt,
                                            uint8_t * const         p_buffer)
{
    uint32_t                               index = 0;
    const ble_gap_evt_sec_info_request_t * p_sec_info_request;

    p_sec_info_request = &(p_ble_evt->evt.gap_evt.params.sec_info_request);

    index += peer_address_encode(&p_buffer[index], &(p_sec_info_request->peer_addr));
    index += uint16_encode(p_sec_info_request->div, &p_buffer[index]);

    p_buffer[index++] = (p_sec_info_request->sign_info << 2 ) |
                        (p_sec_info_request->id_info   << 1 ) |
                        (p_sec_info_request->enc_info);

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_AUTH_STATUS event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_auth_status_evt_encode(const ble_evt_t * const p_ble_evt,
                                           uint8_t * const         p_buffer)
{
    uint32_t                          index = 0;
    const ble_gap_evt_auth_status_t * p_auth_status;

    p_auth_status = &(p_ble_evt->evt.gap_evt.params.auth_status);

    p_buffer[index++] = p_auth_status->auth_status;
    p_buffer[index++] = p_auth_status->error_src;

    p_buffer[index++] = (p_auth_status->sm1_levels.lv3 << 5) |
                        (p_auth_status->sm1_levels.lv2 << 4) |
                        (p_auth_status->sm1_levels.lv1 << 3) |
                        (p_auth_status->sm2_levels.lv3 << 2) |
                        (p_auth_status->sm2_levels.lv2 << 1) |
                        (p_auth_status->sm2_levels.lv1);

    p_buffer[index++] = (p_auth_status->periph_kex.csrk      << 4) |
                        (p_auth_status->periph_kex.address   << 3) |
                        (p_auth_status->periph_kex.irk       << 2) |
                        (p_auth_status->periph_kex.ediv_rand << 1) |
                        (p_auth_status->periph_kex.ltk);

    p_buffer[index++] = (p_auth_status->central_kex.csrk      << 4) |
                        (p_auth_status->central_kex.address   << 3) |
                        (p_auth_status->central_kex.irk       << 2) |
                        (p_auth_status->central_kex.ediv_rand << 1) |
                        (p_auth_status->central_kex.ltk);

    index += uint16_encode(p_auth_status->periph_keys.enc_info.div, &p_buffer[index]);

    uint32_t i;
    for (i = 0; i < BLE_GAP_SEC_KEY_LEN; i++)
    {
        p_buffer[index++] = p_auth_status->periph_keys.enc_info.ltk[i];
    }

    p_buffer[index++] = (p_auth_status->periph_keys.enc_info.ltk_len << 1) |
                        (p_auth_status->periph_keys.enc_info.auth);

    for (i = 0; i < BLE_GAP_SEC_KEY_LEN; i++)
    {
        p_buffer[index++] = (p_auth_status->central_keys.irk.irk[i]);
    }

    index += peer_address_encode(&p_buffer[index], &(p_auth_status->central_keys.id_info));

    return index;
}


/** @brief Function for encoding the BLE_GAP_EVT_CONN_SEC_UPDATE event.
 *
 * @param[in]   p_ble_evt       Input BLE event.
 * @param[out]  p_buffer        Pointer to a buffer for the encoded event.
 *
 * @return Number of bytes encoded.
 */
static uint32_t gap_con_sec_upd_evt_encode(const ble_evt_t * const p_ble_evt,
                                           uint8_t * const         p_buffer)
{
    uint32_t                              index = 0;
    const ble_gap_evt_conn_sec_update_t * p_conn_sec_update;

    p_conn_sec_update = &(p_ble_evt->evt.gap_evt.params.conn_sec_update);

    p_buffer[index++] = p_conn_sec_update->conn_sec.sec_mode.sm |
                        (p_conn_sec_update->conn_sec.sec_mode.lv << 4);
    p_buffer[index++] = p_conn_sec_update->conn_sec.encr_key_size;

    return index;
}


uint32_t ble_rpc_evt_gap_encode(ble_evt_t * p_ble_evt, uint8_t * p_buffer)
{
    uint32_t  index  = 0;

    // Encode packet type.
    p_buffer[index++] = BLE_RPC_PKT_EVT;

    // Encode header.
    index += uint16_encode(p_ble_evt->header.evt_id,  &p_buffer[index]);

    // Encode common part of the event.
    index += uint16_encode(p_ble_evt->evt.gap_evt.conn_handle, &p_buffer[index]);

    // Encode events.
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            index += gap_connected_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            index += gap_disconnected_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            index += gap_timeout_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
            index += gap_conn_param_upd_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            index += gap_sec_param_req_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            index += gap_sec_info_req_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            index += gap_auth_status_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        case BLE_GAP_EVT_CONN_SEC_UPDATE:
            index += gap_con_sec_upd_evt_encode(p_ble_evt, &p_buffer[index]);
            break;

        default:
            break;
    }

    return index;
}
