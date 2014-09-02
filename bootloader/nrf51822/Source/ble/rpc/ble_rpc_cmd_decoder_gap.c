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

#include "ble_rpc_cmd_decoder_gap.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app_util.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_rpc_cmd_decoder.h"
#include "ble_rpc_defines.h"
#include "nrf_error.h"
#include "nordic_common.h"


/**@brief Function for decoding a ble_gap_conn_sec_mode_t from an input command.
 *
 * @param[out]  p_conn_sec_mode The pointer to the decode result structure.
 * @param[in]   p_buffer        The input command.
 *
 * @return      NRF_SUCCESS     Number of bytes decoded.
 */
static uint8_t conn_sec_mode_decode(ble_gap_conn_sec_mode_t * const p_conn_sec_mode,
                                    const uint8_t * const           p_buffer)
{
    p_conn_sec_mode->sm = (p_buffer[0] & 0x0f);
    p_conn_sec_mode->lv = (p_buffer[0] & 0xf0) >> 4;

    return sizeof(uint8_t);
}


/**@brief Function for decoding a ble_gap_addr_t from an input buffer.
 *
 * @param[out]  p_addr          The pointer to the decode result structure.
 * @param[in]   p_buffer        The buffer containing the encoded ble_gap_addr_t.
 *
 * @return      Number of bytes decoded.
 */
static uint8_t addr_decode(ble_gap_addr_t * const p_addr, const uint8_t * const p_buffer)
{
    uint32_t index = 0;

    p_addr->addr_type = p_buffer[index++];
    memcpy(p_addr->addr, &(p_buffer[index]), BLE_GAP_ADDR_LEN);

    index += BLE_GAP_ADDR_LEN;

    return index;
}


/**@brief Function for decoding a ble_gap_whitelist_t from an input buffer.
 *
 * @param[out]   p_wl           The pointer to the decode result structure.
 * @param[in]    p_buffer       The buffer containing the encoded ble_gap_whitelist_t.
 * @param[out]   p_len          Number of bytes decoded.
 *
 * @retval NRF_SUCCESS               If the decoding of whitelists was successful.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t whitelist_decode(ble_gap_whitelist_t * const p_wl,
                                 const uint8_t * const       p_buffer,
                                 uint8_t *                   p_len)
{
    uint32_t index      = 0;
    uint32_t error_code = NRF_SUCCESS;

    static ble_gap_addr_t * p_addresses[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    static ble_gap_addr_t   addresses[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    static ble_gap_irk_t  * p_irks[BLE_GAP_WHITELIST_IRK_MAX_COUNT];
    static ble_gap_irk_t    irks[BLE_GAP_WHITELIST_IRK_MAX_COUNT];

    p_wl->addr_count = p_buffer[index++];

    if (p_wl->addr_count > BLE_GAP_WHITELIST_ADDR_MAX_COUNT)
    {
        error_code = NRF_ERROR_INVALID_LENGTH;
    }
    else
    {
        uint32_t i;

        for (i = 0; i < p_wl->addr_count; i++)
        {
            index          += addr_decode(&(addresses[i]), &(p_buffer[index]));
            p_addresses[i]  = &(addresses[i]);
        }

        p_wl->irk_count = p_buffer[index++];
        if (p_wl->irk_count > BLE_GAP_WHITELIST_IRK_MAX_COUNT)
        {
            error_code = NRF_ERROR_INVALID_LENGTH;
        }
        else
        {
            for (i = 0; i < p_wl->irk_count; i++)
            {
                // Reuse the memory used by the buffer because irk is also a byte array like
                // the buffer.
                memcpy(irks[i].irk, &(p_buffer[index]), BLE_GAP_SEC_KEY_LEN);
                index      += BLE_GAP_SEC_KEY_LEN;
                p_irks[i]   = &irks[i];
            }
        }
        p_wl->pp_addrs = (p_wl->addr_count != 0) ? p_addresses : NULL;
        p_wl->pp_irks  = (p_wl->irk_count != 0) ? p_irks : NULL;
    }

    *p_len = index;

    return error_code;
}

/**@brief Function for decoding security parameters fields in RPC_SD_BLE_GAP_SEC_PARAMS_REPLY
 *        command.
 *
 * @param[in]  p_buffer         The buffer containing the encoded ble_gap_sec_params_t.
 * @param[out] p_params         The pointer to the decode result structure.
 *
 * @return     Number of bytes decoded.
 */
static uint32_t sec_params_decode(const uint8_t * const p_buffer, ble_gap_sec_params_t * p_params)
{
    uint8_t  bit_fields = 0;
    uint32_t index      = 0;

    p_params->timeout       = uint16_decode(&p_buffer[index]);
    index                  += sizeof(uint16_t);

    bit_fields              = p_buffer[index++];
    p_params->bond          = (bit_fields >> 0) & 0x01;
    p_params->mitm          = (bit_fields >> 1) & 0x01;
    p_params->io_caps       = (bit_fields >> 2) & 0x07;
    p_params->oob           = (bit_fields >> 5) & 0x01;

    p_params->min_key_size  = p_buffer[index++];
    p_params->max_key_size  = p_buffer[index++];

    return index;
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_DEVICE_NAME_SET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_device_name_set_handle(uint8_t * p_command, uint32_t command_len)
{
    uint16_t                  dev_name_len;
    uint8_t *                 p_device_name;
    ble_gap_conn_sec_mode_t   dev_name_write_perm;
    ble_gap_conn_sec_mode_t * p_dev_name_write_perm;

    uint32_t index = 0;
    
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        index                += conn_sec_mode_decode(&dev_name_write_perm, &(p_command[index]));
        p_dev_name_write_perm = &dev_name_write_perm;
    }
    else
    {
        p_dev_name_write_perm = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DEVICE_NAME_SET);

    dev_name_len = uint16_decode(&(p_command[index]));
    index       += sizeof(uint16_t);

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        p_device_name = &(p_command[index]);
        index        += dev_name_len;
    }
    else
    {
        p_device_name = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DEVICE_NAME_SET);

    uint32_t err_code = sd_ble_gap_device_name_set(p_dev_name_write_perm,
                                                   p_device_name,
                                                   dev_name_len);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_DEVICE_NAME_SET, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_DEVICE_NAME_GET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_device_name_get_handle(uint8_t * p_command, uint32_t command_len)
{
    uint32_t err_code;

    uint16_t dev_name_len = 0;
    uint32_t index        = 0;

    // Two bytes will be reserved for the length of device name.
    uint8_t    resp_data[BLE_GAP_DEVNAME_MAX_LEN + sizeof(dev_name_len)];
    // Pointer to the device name length variable.
    uint8_t *  p_dev_name     = &(resp_data[2]);
    // Pointer to the device name target.
    uint16_t * p_dev_name_len = &(dev_name_len);

    // Length present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DEVICE_NAME_GET);

        // Use the remote buffer size when calling the SoftDevice to make sure the caller has
        // enough memory allocated.
        *p_dev_name_len  = uint16_decode(&p_command[index]);
        index           += sizeof(uint16_t);

        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DEVICE_NAME_GET);

        // Verify that the remote buffer size is smaller or equal to the local buffer size.
        if (*p_dev_name_len > BLE_GAP_DEVNAME_MAX_LEN)
        {
            return ble_rpc_cmd_resp_send(SD_BLE_GAP_DEVICE_NAME_GET, NRF_ERROR_INVALID_LENGTH);
        }
    }
    else
    {
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DEVICE_NAME_GET);
        p_dev_name_len = NULL;
    }

    // Remote result buffer is present.
    if (p_command[index++] == RPC_BLE_FIELD_NOT_PRESENT)
    {
        p_dev_name = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DEVICE_NAME_GET);

    err_code = sd_ble_gap_device_name_get(p_dev_name, p_dev_name_len);

    if (err_code == NRF_SUCCESS)
    {

        // Encode the length.
        UNUSED_VARIABLE(uint16_encode(*p_dev_name_len, &resp_data[0]));

        // Calculate the total size of the device name and the device name length bytes.
        uint16_t total_len  = uint16_decode(&resp_data[0]);
        total_len          += sizeof(total_len);

        return ble_rpc_cmd_resp_data_send(SD_BLE_GAP_DEVICE_NAME_GET,
                                          NRF_SUCCESS,
                                          resp_data,
                                          total_len);
    }

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_DEVICE_NAME_GET, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_APPEARANCE_SET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_appearance_set_handle(uint8_t * p_command, uint32_t command_len)
{
    uint16_t appearance = uint16_decode(&(p_command[0]));

    RPC_DECODER_LENGTH_CHECK(command_len, sizeof(appearance), SD_BLE_GAP_APPEARANCE_SET);

    uint32_t err_code = sd_ble_gap_appearance_set(appearance);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_APPEARANCE_SET, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_APPEARANCE_GET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_appearance_get_handle(uint8_t const * const p_command, uint32_t command_len)
{

    uint8_t  resp_data[sizeof(uint16_t)];
    uint16_t appearance;
    uint32_t err_code;

    uint32_t   index        = 0;
    uint8_t    out_index    = 0;
    uint16_t * p_appearance = &appearance;
    
    // If appearance field is present.
    if (p_command[index++] == RPC_BLE_FIELD_NOT_PRESENT)
    {
        p_appearance = NULL;
    }
    
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_APPEARANCE_GET);

    err_code  = sd_ble_gap_appearance_get(p_appearance);
    
    if (err_code == NRF_SUCCESS)
    {
        out_index += uint16_encode(*p_appearance, resp_data);
        return ble_rpc_cmd_resp_data_send(SD_BLE_GAP_APPEARANCE_GET,
                                          err_code,
                                          resp_data,
                                          out_index);
    }
    else
    {
        return ble_rpc_cmd_resp_send(SD_BLE_GAP_APPEARANCE_GET, err_code);
    }       
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_PPCP_SET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_ppcp_set_handle(uint8_t * p_command, uint32_t command_len)
{
    uint32_t              err_code;
    ble_gap_conn_params_t conn_params;

    uint32_t                index        = 0;
    ble_gap_conn_params_t * p_conn_param = NULL;

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        conn_params.min_conn_interval = uint16_decode(&p_command[index]);
        index += sizeof(uint16_t);

        conn_params.max_conn_interval = uint16_decode(&p_command[index]);
        index += sizeof(uint16_t);

        conn_params.slave_latency     = uint16_decode(&p_command[index]);
        index += sizeof(uint16_t);

        conn_params.conn_sup_timeout  = uint16_decode(&p_command[index]);
        p_conn_param = &conn_params;
    }
    
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_PPCP_SET);

    err_code = sd_ble_gap_ppcp_set(p_conn_param);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_PPCP_SET, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_PPCP_GET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_ppcp_get_handle(uint8_t * p_command, uint32_t command_len)
{
    uint32_t              err_code;
    ble_gap_conn_params_t conn_params;
    uint8_t               resp_data[sizeof(ble_gap_conn_params_t)];

    uint8_t  encode_index = 0;
    uint32_t index        = 0;

    // Pointer to the connection parameters result buffer.
    ble_gap_conn_params_t * p_conn_params = &conn_params;

    // If length field is not present.
    if (p_command[index++] == RPC_BLE_FIELD_NOT_PRESENT)
    {
        p_conn_params = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_PPCP_GET);

    err_code = sd_ble_gap_ppcp_get(p_conn_params);

    if (err_code == NRF_SUCCESS)
    {
        encode_index += uint16_encode(p_conn_params->min_conn_interval, &resp_data[encode_index]);
        encode_index += uint16_encode(p_conn_params->max_conn_interval, &resp_data[encode_index]);
        encode_index += uint16_encode(p_conn_params->slave_latency, &resp_data[encode_index]);
        encode_index += uint16_encode(p_conn_params->conn_sup_timeout, &resp_data[encode_index]);

        return ble_rpc_cmd_resp_data_send(SD_BLE_GAP_PPCP_GET, err_code, resp_data, encode_index);
    }
    else
    {
        return ble_rpc_cmd_resp_send(SD_BLE_GAP_PPCP_GET, err_code);
    }
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_ADV_START opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_adv_start_handle(uint8_t * p_command, uint32_t command_len)
{
    ble_gap_adv_params_t adv_params;
    ble_gap_addr_t       directed_peer_address;
    ble_gap_whitelist_t  white_list;

    uint32_t index = 0;

    adv_params.type = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        // Peer Address Present. Decode the peer address.
        index                  += addr_decode(&directed_peer_address, &(p_command[index]));
        adv_params.p_peer_addr  = &(directed_peer_address);

        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);
    }
    else
    {
        adv_params.p_peer_addr = NULL;
    }

    adv_params.fp = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);

    // Whitelist present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        uint8_t  decoded_length = 0;
        uint32_t err_code       = whitelist_decode(&white_list,
                                                   &(p_command[index]),
                                                   &decoded_length);
        if (err_code != NRF_SUCCESS)
        {
            return ble_rpc_cmd_resp_send(SD_BLE_GAP_ADV_START, err_code);
        }
        index = index + decoded_length;
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);
        adv_params.p_whitelist = &white_list;
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);
    }
    else
    {
        adv_params.p_whitelist = NULL;
    }

    adv_params.interval = uint16_decode(&p_command[index]);
    index              += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);

    adv_params.timeout  = uint16_decode(&p_command[index]);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_ADV_START);

    uint32_t err_code   = sd_ble_gap_adv_start(&adv_params);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_ADV_START, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_ADV_DATA_SET opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 *
 * @note The NRF_ERROR_INVALID_LENGTH error code for this function represents both a SoftDevice
 *       error and content length check error.
 */
static uint32_t gap_adv_data_set_handle(const uint8_t * const p_command, uint32_t command_len)
{
    uint8_t         dlen;
    uint8_t         srdlen;
    const uint8_t * p_data;
    const uint8_t * p_sr_data;

    uint32_t index = 0;

    dlen    = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, (index + dlen), SD_BLE_GAP_ADV_DATA_SET);
    p_data  = &(p_command[index]);
    index  += dlen;

    srdlen    = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, (index + srdlen), SD_BLE_GAP_ADV_DATA_SET);
    p_sr_data = &(p_command[index]);

    uint32_t err_code = sd_ble_gap_adv_data_set(p_data, dlen, p_sr_data, srdlen);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_ADV_DATA_SET, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_DISCONNECT opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_disconnect_handle(const uint8_t * const p_command, uint32_t command_len)
{
    uint16_t conn_handle;
    uint8_t  hci_status_code;
    uint32_t err_code;

    uint32_t index = 0;

    conn_handle      = uint16_decode(&p_command[index]);
    index           += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DISCONNECT);
    hci_status_code  = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_DISCONNECT);

    err_code = sd_ble_gap_disconnect(conn_handle, hci_status_code);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_DISCONNECT, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_SEC_PARAMS_REPLY opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_sec_params_reply_handle(const uint8_t * const p_command, uint32_t command_len)
{
    uint8_t              sec_status;
    uint16_t             conn_handle;
    uint32_t             err_code;
    ble_gap_sec_params_t sec_params;

    uint32_t               index       = 0;
    ble_gap_sec_params_t * p_sec_param = NULL;

    conn_handle  = uint16_decode(&p_command[index]);
    index       += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_SEC_PARAMS_REPLY);

    sec_status = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_SEC_PARAMS_REPLY);
    
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        index       += sec_params_decode(&p_command[index], &sec_params);
        p_sec_param  = &sec_params;
    }
    else
    {
        p_sec_param = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_SEC_PARAMS_REPLY);

    err_code = sd_ble_gap_sec_params_reply(conn_handle, sec_status, p_sec_param);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_SEC_PARAMS_REPLY, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_CONN_PARAM_UPDATE opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_conn_param_update_handle(const uint8_t * const p_command, uint32_t command_len)
{
    uint16_t                conn_handle;
    uint32_t                err_code;
    ble_gap_conn_params_t   conn_params;

    ble_gap_conn_params_t * p_conn_params = NULL;
    uint32_t                index = 0;

    conn_handle  = uint16_decode(&p_command[index]);
    index       += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_CONN_PARAM_UPDATE);

    // Check if the Connection Parameters field is present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        conn_params.min_conn_interval = uint16_decode(&p_command[index]);
        index                        += sizeof(uint16_t);
        conn_params.max_conn_interval = uint16_decode(&p_command[index]);
        index                        += sizeof(uint16_t);
        conn_params.slave_latency     = uint16_decode(&p_command[index]);
        index                        += sizeof(uint16_t);
        conn_params.conn_sup_timeout  = uint16_decode(&p_command[index]);
        p_conn_params                 = &conn_params;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_CONN_PARAM_UPDATE);

    err_code = sd_ble_gap_conn_param_update(conn_handle, p_conn_params);

    return ble_rpc_cmd_resp_send(SD_BLE_GAP_CONN_PARAM_UPDATE, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GAP_SEC_INFO_REPLY opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the the transport layer.
 *
 * @param[in] p_command         The encoded structure that needs to be decoded and passed on
 *                              to the BLE Stack API.
 * @param[in] command_len       The length of the encoded command read from transport layer.
 *
 * @retval NRF_SUCCESS               If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 * @retval NRF_ERROR_INVALID_LENGTH  If the content length of the packet is not conforming to the
 *                                   codec specification.
 */
static uint32_t gap_sec_info_reply_handle(uint8_t * const p_command, uint32_t command_len)
{

    bool     use_sign;
    uint16_t conn_handle;
    uint32_t err_code;
    
    uint32_t            index     = 0;
    ble_gap_enc_info_t  enc_info  = {0};
    ble_gap_sign_info_t sign_info = {{0}};
    
    conn_handle  = uint16_decode(&p_command[index]);
    index       += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_SEC_INFO_REPLY);

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {        
        enc_info.div     = uint16_decode(&p_command[index]);
        index           += sizeof(uint16_t);
        memcpy(enc_info.ltk, &p_command[index], sizeof(enc_info.ltk));
        index           += sizeof(enc_info.ltk);
        enc_info.auth    = (p_command[index] & 0x01);
        enc_info.ltk_len = ((p_command[index] & 0xfe) >> 1);
        index++;
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_SEC_INFO_REPLY);
    }

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        use_sign = true;
        
        memcpy(sign_info.csrk, &p_command[index], sizeof(sign_info.csrk));
        index += sizeof(sign_info.csrk);
    }
    else
    {
        use_sign = false;
    }    

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GAP_SEC_INFO_REPLY);
    
    err_code = sd_ble_gap_sec_info_reply(conn_handle, 
                                        (p_command[2] == RPC_BLE_FIELD_PRESENT) ? &enc_info : NULL, 
                                        (use_sign) ? &sign_info : NULL);
                                        
    return ble_rpc_cmd_resp_send(SD_BLE_GAP_SEC_INFO_REPLY, err_code);
}


uint32_t ble_rpc_cmd_gap_decode(uint8_t * p_command, uint8_t op_code, uint32_t command_len)
{
    uint32_t err_code;

    switch (op_code)
    {
        case SD_BLE_GAP_DEVICE_NAME_SET:
            err_code = gap_device_name_set_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_DEVICE_NAME_GET:
            err_code = gap_device_name_get_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_APPEARANCE_SET:
            err_code = gap_appearance_set_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_APPEARANCE_GET:
            err_code = gap_appearance_get_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_PPCP_SET:
            err_code = gap_ppcp_set_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_PPCP_GET:
            err_code = gap_ppcp_get_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_ADV_START:
            err_code = gap_adv_start_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_ADV_DATA_SET:
            err_code = gap_adv_data_set_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_DISCONNECT:
            err_code = gap_disconnect_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_SEC_PARAMS_REPLY:
            err_code = gap_sec_params_reply_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_CONN_PARAM_UPDATE:
            err_code = gap_conn_param_update_handle(p_command, command_len);
            break;

        case SD_BLE_GAP_SEC_INFO_REPLY:
            err_code = gap_sec_info_reply_handle(p_command, command_len);
            break;

        default:
            err_code = ble_rpc_cmd_resp_send(op_code, NRF_ERROR_NOT_SUPPORTED);
            break;
    }

    return err_code;
}
