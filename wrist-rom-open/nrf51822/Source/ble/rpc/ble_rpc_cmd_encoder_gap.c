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

#include "ble_gap.h"
#include "nordic_common.h"
#include "app_util.h"
#include "ble_rpc_cmd_encoder.h"
#include "ble_rpc_defines.h"
#include "hci_transport.h"
#include <string.h>

                                     /**< These externals come from from the ble_rpc_dms_encoder.c file. */
extern uint8_t * g_cmd_response_buf; /**< Pointer to the buffer used for storing command response. */
extern uint8_t * g_cmd_buffer;       /**< Pointer to the buffer used for storing serialized commands. */


/**@brief       Function for encoding the peer address into the packet array provided and returning
 *              the number of bytes encoded.
 *
 * @param[in]   p_packet        Pointer to the memory location where the encoded peer address should
 *                              be stored. The memory location should hold minimum
 *                              @ref BLE_GAP_ADDR_LEN + addr_type of data (7 bytes total).
 * @param[in]   p_peer_address  Pointer to the peer address that should be encoded.
 *
 * @return      Number of bytes encoded.
 */
static uint32_t peer_address_encode(uint8_t *                    p_packet,
                                    const ble_gap_addr_t * const p_peer_address)
{
    uint32_t i;
    uint32_t index = 0;

    p_packet[index++] = p_peer_address->addr_type;

    for (i = 0; i < BLE_GAP_ADDR_LEN; i++)
    {
        p_packet[index++] = p_peer_address->addr[i];
    }

    return index;
}


/**@brief       Function for encoding the whitelist into the packet array provided and returning
 *              the number of bytes encoded.
 *
 * @param[in]   p_packet     Pointer to the memory location where the encoded peer address should
 *                           be stored. The memory location should hold at minimum the address and 
 *                           IRK counters (2 bytes), (@ref BLE_GAP_ADDR_LEN + addr_type) *
 *                           @ref BLE_GAP_WHITELIST_ADDR_MAX_COUNT of data (56 bytes), and
 *                           @ref BLE_GAP_SEC_KEY_LEN * @ref BLE_GAP_WHITELIST_IRK_MAX_COUNT of data
 *                           (128 bytes). In total 186 bytes.
 * @param[in]   p_whitelist  Pointer to the peer address that should be encoded.
 *
 * @return      Number of bytes encoded.
 */
static uint32_t whitelist_encode(uint8_t *                         p_packet,
                                 const ble_gap_whitelist_t * const p_whitelist)
{
    uint32_t i;
    uint32_t j;
    uint32_t index = 0;

    p_packet[index++] = p_whitelist->addr_count;

    for (i = 0; i < p_whitelist->addr_count; i++)
    {
        p_packet[index++] = p_whitelist->pp_addrs[i]->addr_type;
        for (j = 0; j < BLE_GAP_ADDR_LEN; j++)
        {
            p_packet[index++] = p_whitelist->pp_addrs[i]->addr[j];
        }
    }

    p_packet[index++] = p_whitelist->irk_count;
    for (i = 0; i < p_whitelist->irk_count; i++)
    {
        for (j = 0; j < BLE_GAP_SEC_KEY_LEN; j++)
        {
            p_packet[index++] = p_whitelist->pp_irks[i]->irk[j];
        }
    }

    return index;
}


uint32_t sd_ble_gap_device_name_set(ble_gap_conn_sec_mode_t const * const p_write_perm,
                                    uint8_t const                 * const p_dev_name,
                                    uint16_t                              len)
{
    uint32_t i;
    uint32_t index  = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_DEVICE_NAME_SET;

    if (p_write_perm != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        g_cmd_buffer[index++] = (uint8_t) ((p_write_perm->sm) | (p_write_perm->lv << 4));
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    index += uint16_encode(len, &g_cmd_buffer[index]);

    if (p_dev_name != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        for (i = 0; i < len; i++)
        {
            g_cmd_buffer[index++] = p_dev_name[i];
        }
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_DEVICE_NAME_SET);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_appearance_set(uint16_t appearance)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_APPEARANCE_SET;
    index                += uint16_encode(appearance, &g_cmd_buffer[index]);

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_APPEARANCE_SET);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_disconnect(uint16_t conn_handle, uint8_t hci_status_code)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_DISCONNECT;
    index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);
    g_cmd_buffer[index++] = hci_status_code;

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_DISCONNECT);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_sec_params_reply(uint16_t                           conn_handle,
                                     uint8_t                            sec_status,
                                     ble_gap_sec_params_t const * const p_sec_params)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_SEC_PARAMS_REPLY;
    index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);
    g_cmd_buffer[index++] = sec_status;

    if (p_sec_params != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(p_sec_params->timeout, &g_cmd_buffer[index]);
        g_cmd_buffer[index++] = (
                                 (p_sec_params->oob     << 5) |
                                 (p_sec_params->io_caps << 2) |
                                 (p_sec_params->mitm    << 1) |
                                 (p_sec_params->bond    << 0)
                                );
        g_cmd_buffer[index++] = p_sec_params->min_key_size;
        g_cmd_buffer[index++] = p_sec_params->max_key_size;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_SEC_PARAMS_REPLY);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_ppcp_set(ble_gap_conn_params_t const * const p_conn_params)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_PPCP_SET;

    if (p_conn_params != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;

        index += uint16_encode(p_conn_params->min_conn_interval, &g_cmd_buffer[index]);
        index += uint16_encode(p_conn_params->max_conn_interval, &g_cmd_buffer[index]);
        index += uint16_encode(p_conn_params->slave_latency, &g_cmd_buffer[index]);
        index += uint16_encode(p_conn_params->conn_sup_timeout, &g_cmd_buffer[index]);
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_PPCP_SET);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_ppcp_get(ble_gap_conn_params_t * const p_conn_params)
{
    uint32_t err_code;
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_PPCP_GET;

    if (p_conn_params != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_PPCP_GET);
    if (p_conn_params != NULL)
    {
        memcpy(p_conn_params, 
               &(g_cmd_response_buf[BLE_OP_CODE_SIZE + BLE_PKT_TYPE_SIZE + RPC_ERR_CODE_SIZE]),
               sizeof(*p_conn_params));
    }
               
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_adv_start(ble_gap_adv_params_t const * const p_adv_params)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_ADV_START;
    g_cmd_buffer[index++] = p_adv_params->type;

    if (p_adv_params->p_peer_addr == NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += peer_address_encode(&g_cmd_buffer[index],
                                                    p_adv_params->p_peer_addr);
    }

    g_cmd_buffer[index++] = p_adv_params->fp;

    if (p_adv_params->p_whitelist == NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += whitelist_encode(&g_cmd_buffer[index], p_adv_params->p_whitelist);
    }

    index += uint16_encode(p_adv_params->interval, &g_cmd_buffer[index]);
    index += uint16_encode(p_adv_params->timeout, &g_cmd_buffer[index]);
    
    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_ADV_START);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_adv_data_set(uint8_t const * const p_data,
                                 uint8_t               dlen,
                                 uint8_t const * const p_sr_data,
                                 uint8_t               srdlen)
{
    uint32_t i;
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_ADV_DATA_SET;
    g_cmd_buffer[index++] = dlen;

    for (i = 0; i < dlen; i++)
    {
        g_cmd_buffer[index++] = p_data[i];
    }

    g_cmd_buffer[index++] = srdlen;

    for (i = 0; i < srdlen; i++)
    {
        g_cmd_buffer[index++] = p_sr_data[i];
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_ADV_DATA_SET);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_device_name_get(uint8_t * const p_dev_name, uint16_t * const p_len)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_DEVICE_NAME_GET;

    if (p_len != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;  
        index                += uint16_encode(*p_len, &g_cmd_buffer[index]);
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;  
    }

    g_cmd_buffer[index++] = (p_dev_name != NULL) ? RPC_BLE_FIELD_PRESENT :
                                                   RPC_BLE_FIELD_NOT_PRESENT;
        
    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }   

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_DEVICE_NAME_GET);
    if (p_len != NULL)
    {
        *p_len = uint16_decode(&(g_cmd_response_buf[BLE_OP_CODE_SIZE  + 
                                                    BLE_PKT_TYPE_SIZE +
                                                    RPC_ERR_CODE_SIZE]));
        if (p_dev_name != NULL)
        {
            memcpy(p_dev_name, 
                   &(g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                        BLE_PKT_TYPE_SIZE +
                                        RPC_ERR_CODE_SIZE +
                                        sizeof(*p_len)]),
                   *p_len);
        }
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_appearance_get(uint16_t * const p_appearance)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_APPEARANCE_GET;

    if (p_appearance != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;   
    }
    else
    {
       g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT; 
    }
    
    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }    
        
    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_APPEARANCE_GET);
    if (p_appearance != NULL)
    {
        *p_appearance = uint16_decode(&(g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                                           BLE_PKT_TYPE_SIZE +
                                                           RPC_ERR_CODE_SIZE]));
    }
        
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_conn_param_update(uint16_t                            conn_handle,
                                      ble_gap_conn_params_t const * const p_conn_params)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_CONN_PARAM_UPDATE;
    index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);

    if (p_conn_params != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(p_conn_params->min_conn_interval, 
                                              &g_cmd_buffer[index]);
        index                += uint16_encode(p_conn_params->max_conn_interval, 
                                              &g_cmd_buffer[index]);
        index                += uint16_encode(p_conn_params->slave_latency, 
                                              &g_cmd_buffer[index]);
        index                += uint16_encode(p_conn_params->conn_sup_timeout, 
                                              &g_cmd_buffer[index]);
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_CONN_PARAM_UPDATE);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gap_sec_info_reply(uint16_t                          conn_handle,
                                   ble_gap_enc_info_t  const * const p_enc_info,
                                   ble_gap_sign_info_t const * const p_sign_info)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GAP_SEC_INFO_REPLY;
    index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);

    if (p_enc_info != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(p_enc_info->div, &g_cmd_buffer[index]);
        memcpy(&g_cmd_buffer[index], p_enc_info->ltk, BLE_GAP_SEC_KEY_LEN);
        index                += BLE_GAP_SEC_KEY_LEN;
        g_cmd_buffer[index++] = (p_enc_info->auth | (p_enc_info->ltk_len << 1));
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    if (p_sign_info != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        memcpy(&g_cmd_buffer[index], p_sign_info->csrk, BLE_GAP_SEC_KEY_LEN);
        index                += BLE_GAP_SEC_KEY_LEN;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GAP_SEC_INFO_REPLY);
    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}
