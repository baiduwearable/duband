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

#include "ble_rpc_cmd_decoder_gatts.h"
#include <stdio.h>
#include <string.h>
#include "app_util.h"
#include "ble.h"
#include "ble_gatts.h"
#include "ble_rpc_cmd_decoder.h"
#include "ble_rpc_defines.h"
#include "nrf_error.h"
#include "nrf_soc.h"
#include "nordic_common.h"

#define GATTS_CHAR_USER_DESC_LEN_MAX 512 /**< Max length of characteristic user description. */

/**@brief Function for decoding attribute meta data.
 *
 * @note The attribute meta data vloc needs to be @ref BLE_GATTS_VLOC_STACK. As the Connectivity
 *       Chip SoftDevice is not able to refer to a memory address on the Application Chip.
 *
 * @param[in]  p_buffer         The buffer containing the encoded ble_gatts_attr_md_t.
 * @param[out] p_attr_md        Decoded attribute meta data.
 * @param[out] p_index          Number of bytes decoded.
 *
 * @retval NRF_SUCCESS               If the decoding of attribute meta data was successful.
 * @retval NRF_ERROR_INVALID_PARAM   If vloc is set to a value other than BLE_GATTS_VLOC_STACK.
 */
static uint32_t attr_md_decode(uint8_t *             p_buffer,
                               ble_gatts_attr_md_t * p_attr_md,
                               uint32_t *            p_index)
{
    uint32_t index = 0;

    p_attr_md->read_perm.sm     = ((p_buffer[index]   >> 0) & 0xF);
    p_attr_md->read_perm.lv     = ((p_buffer[index++] >> 4) & 0xF);

    p_attr_md->write_perm.sm    = ((p_buffer[index]   >> 0) & 0xF);
    p_attr_md->write_perm.lv    = ((p_buffer[index++] >> 4) & 0xF);

    p_attr_md->vlen             = ((p_buffer[index]   >> 0) & 0x1);
    p_attr_md->vloc             = ((p_buffer[index]   >> 1) & 0x3);
    p_attr_md->rd_auth          = ((p_buffer[index]   >> 3) & 0x1);
    p_attr_md->wr_auth          = ((p_buffer[index++] >> 4) & 0x1);


    *p_index += index;

    return (p_attr_md->vloc == BLE_GATTS_VLOC_STACK) ? NRF_SUCCESS : NRF_ERROR_INVALID_PARAM;
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GATTS_SERVICE_ADD opcode.
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
static uint32_t gatts_service_add_handle(const uint8_t * const p_command, uint32_t command_len)
{
    uint8_t      type;
    uint16_t *   p_handle;
    uint32_t     err_code;
    ble_uuid_t   uuid;
    ble_uuid_t * p_uuid;
    uint8_t      resp_data[sizeof(uint16_t)];

    uint32_t index  = 0;
    uint16_t handle = 0;

    type = p_command[index++];
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SERVICE_ADD);

    // Service UUID field is present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        uuid.uuid = uint16_decode(&p_command[index]);
        index    += sizeof(uint16_t);
        uuid.type = p_command[index++];
        p_uuid    = &uuid;
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SERVICE_ADD);
    }
    else
    {
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SERVICE_ADD);
        p_uuid = NULL;
    }

    // Handle present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        p_handle = &handle;
    }
    else
    {
        p_handle = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SERVICE_ADD);

    err_code = sd_ble_gatts_service_add(type, p_uuid, p_handle);

    if (err_code == NRF_SUCCESS)
    {
        UNUSED_VARIABLE(uint16_encode(handle, resp_data));
        return ble_rpc_cmd_resp_data_send(SD_BLE_GATTS_SERVICE_ADD,
                                          err_code,
                                          resp_data,
                                          sizeof(resp_data));
    }
    else
    {
        return ble_rpc_cmd_resp_send(SD_BLE_GATTS_SERVICE_ADD, err_code);
    }
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GATTS_SYS_ATTR_GET opcode.
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
static uint32_t gatts_sys_attr_get_handle(uint8_t const * const p_command, uint32_t command_len)
{
    uint32_t err_code;
    uint8_t  resp_data[BLE_GATTS_VAR_ATTR_LEN_MAX];
    uint16_t conn_handle;

    uint32_t   index              = 0;
    uint16_t   attr_data_length   = 0;
    uint8_t *  p_attr_data        = &resp_data[3];
    uint16_t * p_attr_data_length = &attr_data_length;

    conn_handle  = uint16_decode(&p_command[index]);
    index       += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SYS_ATTR_GET);

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        attr_data_length  = uint16_decode(&p_command[index]);
        index            += sizeof(uint16_t);
    }
    else
    {
        p_attr_data_length = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SYS_ATTR_GET);

    if (p_command[index++] == RPC_BLE_FIELD_NOT_PRESENT)
    {
        p_attr_data = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SYS_ATTR_GET);

    err_code = sd_ble_gatts_sys_attr_get(conn_handle, p_attr_data, p_attr_data_length);

    if (err_code == NRF_SUCCESS)
    {
        index  = 0;
        index += uint16_encode(*p_attr_data_length, &resp_data[0]);

        if (p_attr_data == NULL)
        {
            resp_data[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }
        else
        {
            resp_data[index++]  = RPC_BLE_FIELD_PRESENT;
            index              += *p_attr_data_length;
        }

        return ble_rpc_cmd_resp_data_send(SD_BLE_GATTS_SYS_ATTR_GET,
                                          err_code,
                                          resp_data,
                                          index);
    }
    else
    {
        return ble_rpc_cmd_resp_send(SD_BLE_GATTS_SYS_ATTR_GET, err_code);
    }
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GATTS_SYS_ATTR_SET opcode.
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
static uint32_t gatts_sys_attr_set_handle(uint8_t const * const p_command, uint32_t command_len)
{
    uint16_t conn_handle;
    uint32_t err_code;

    uint32_t        index            = 0;
    uint16_t        attr_data_length = 0;
    const uint8_t * p_attr_data      = NULL;

    conn_handle  = uint16_decode(&p_command[index]);
    index       += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SYS_ATTR_SET);

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        attr_data_length  = uint16_decode(&p_command[index]);
        index            += sizeof(uint16_t);
        p_attr_data       = &(p_command[index]);
        index            += attr_data_length;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_SYS_ATTR_SET);

    err_code = sd_ble_gatts_sys_attr_set(conn_handle, p_attr_data, attr_data_length);

    return ble_rpc_cmd_resp_send(SD_BLE_GATTS_SYS_ATTR_SET, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GATTS_VALUE_SET opcode.
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
static uint32_t gatts_value_set_handle(uint8_t const * const p_command, uint32_t command_len)
{
    uint16_t handle;
    uint16_t offset;
    uint32_t err_code;

    uint32_t        index    = 0;
    uint16_t        length   = 0;
    uint16_t *      p_length = NULL;
    const uint8_t * p_value  = NULL;

    handle  = uint16_decode(&p_command[index]);
    index  += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_VALUE_SET);

    offset  = uint16_decode(&p_command[index]);
    index  += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_VALUE_SET);

    // Value length present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        length    = uint16_decode(&p_command[index]);
        p_length  = &length;
        index    += sizeof(uint16_t);
    }

    RPC_DECODER_LENGTH_CHECK(command_len, (index + length), SD_BLE_GATTS_VALUE_SET);

    // Value present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        p_value = &p_command[index];
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_VALUE_SET);

    err_code = sd_ble_gatts_value_set(handle, offset, p_length, p_value);

    if (err_code == NRF_SUCCESS)
    {
        uint8_t resp_data[sizeof(uint16_t)];
        UNUSED_VARIABLE(uint16_encode(*p_length, resp_data));

        return ble_rpc_cmd_resp_data_send(SD_BLE_GATTS_VALUE_SET,
                                          err_code,
                                          resp_data,
                                          sizeof(resp_data));
    }
    else
    {
        return ble_rpc_cmd_resp_send(SD_BLE_GATTS_VALUE_SET, err_code);
    }
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GATTS_HVX opcode.
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
static uint32_t gatts_hvx_handle(uint8_t * const p_command, uint32_t command_len)
{
    uint32_t               err_code;
    uint16_t               conn_handle;
    ble_gatts_hvx_params_t hvx_params;
    uint8_t                resp_data[sizeof(uint16_t)];

    uint16_t                 hvx_params_data_length = 0;
    uint32_t                 index                  = 0;
    ble_gatts_hvx_params_t * p_hvx_params           = NULL;

    conn_handle  = uint16_decode(&p_command[index]);
    index       += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_HVX);

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        hvx_params.handle  = uint16_decode(&p_command[index]);
        index             += sizeof(uint16_t);
        hvx_params.type    = p_command[index++];
        hvx_params.offset  = uint16_decode(&p_command[index]);
        index             += sizeof(uint16_t);
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_HVX);
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            hvx_params_data_length  = uint16_decode(&p_command[index]);
            index                  += sizeof(uint16_t);
            hvx_params.p_len        = &hvx_params_data_length;
        }
        else
        {
            hvx_params.p_len = NULL;
        }
        RPC_DECODER_LENGTH_CHECK(command_len,
                                 (index + hvx_params_data_length),
                                 SD_BLE_GATTS_HVX);

        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            hvx_params.p_data = &(p_command[index]);
        }
        else
        {
            hvx_params.p_data = NULL;
        }
        p_hvx_params = &hvx_params;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_HVX);

    err_code = sd_ble_gatts_hvx(conn_handle, p_hvx_params);

    if (err_code == NRF_SUCCESS)
    {
        if (p_hvx_params != NULL && p_hvx_params->p_len != NULL)
        {
            UNUSED_VARIABLE(uint16_encode(*(p_hvx_params->p_len), resp_data));
            return ble_rpc_cmd_resp_data_send(SD_BLE_GATTS_HVX,
                                              err_code,
                                              resp_data,
                                              sizeof(resp_data));
        }
    }

    return ble_rpc_cmd_resp_send(SD_BLE_GATTS_HVX, err_code);
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_GATTS_CHARACTERISTIC_ADD opcode.
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
static uint32_t gatts_characteristic_add_handle(uint8_t * const p_command, uint32_t command_len)
{
    uint32_t                   err_code;
    uint16_t                   service_handle;
    uint8_t                    attr_data[BLE_GATTS_VAR_ATTR_LEN_MAX];
    uint8_t                    char_user_desc[GATTS_CHAR_USER_DESC_LEN_MAX];
    ble_gatts_char_md_t        char_md;
    ble_gatts_attr_md_t        char_md_user_desc_md;
    ble_gatts_attr_md_t        char_md_cccd_md;
    ble_gatts_attr_md_t        char_md_sccd_md;
    ble_gatts_char_pf_t        char_md_char_pf;
    ble_gatts_char_md_t *      p_char_md;
    ble_gatts_attr_t           attr_char_value;
    ble_uuid_t                 attr_char_value_uuid;
    ble_gatts_attr_md_t        attr_char_value_attr_md;
    ble_gatts_attr_t *         p_attr_char_value;
    ble_gatts_char_handles_t   handles;
    ble_gatts_char_handles_t * p_handles;

    uint32_t index = 0;

    service_handle  = uint16_decode(&p_command[index]);
    index          += sizeof(uint16_t);
    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);

    // Char Metadata present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        // Properties.
        char_md.char_props.broadcast       = ((p_command[index]   >> 0) & 1);
        char_md.char_props.read            = ((p_command[index]   >> 1) & 1);
        char_md.char_props.write_wo_resp   = ((p_command[index]   >> 2) & 1);
        char_md.char_props.write           = ((p_command[index]   >> 3) & 1);
        char_md.char_props.notify          = ((p_command[index]   >> 4) & 1);
        char_md.char_props.indicate        = ((p_command[index]   >> 5) & 1);
        char_md.char_props.auth_signed_wr  = ((p_command[index++] >> 6) & 1);

        // Extended properties.
        char_md.char_ext_props.reliable_wr = ((p_command[index]   >> 0) & 1);
        char_md.char_ext_props.wr_aux      = ((p_command[index++] >> 1) & 1);

        // User description.
        char_md.char_user_desc_max_size  = uint16_decode(&p_command[index]);
        index                           += sizeof(uint16_t);

        char_md.char_user_desc_size  = uint16_decode(&p_command[index]);
        index                       += sizeof(uint16_t);

        RPC_DECODER_LENGTH_CHECK(command_len,
                                 (index + char_md.char_user_desc_size),
                                 SD_BLE_GATTS_CHARACTERISTIC_ADD);

        // User description present.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            uint32_t desc_index;
            for (desc_index = 0; desc_index < char_md.char_user_desc_size; desc_index++)
            {
                char_user_desc[desc_index] = p_command[index++];
            }
            char_md.p_char_user_desc = char_user_desc;
        }
        else
        {
            char_md.p_char_user_desc = NULL;
        }

        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);

        // Presentation format present.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            char_md_char_pf.format     = p_command[index++];
            char_md_char_pf.exponent   = p_command[index++];
            char_md_char_pf.unit       = uint16_decode(&p_command[index]);
            index += sizeof(uint16_t);
            char_md_char_pf.name_space = p_command[index++];
            char_md_char_pf.desc       = uint16_decode(&p_command[index]);
            index += sizeof(uint16_t);
            char_md.p_char_pf          = &char_md_char_pf;
        }
        else
        {
            char_md.p_char_pf = NULL;
        }
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);

        // User descriptor metadata.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            RPC_DECODER_LENGTH_CHECK(command_len,
                                     (index + sizeof(ble_gatts_attr_md_t)),
                                     SD_BLE_GATTS_CHARACTERISTIC_ADD);
            err_code = attr_md_decode(&p_command[index], &char_md_user_desc_md, &index);
            if (err_code != NRF_SUCCESS)
            {
                return ble_rpc_cmd_resp_send(SD_BLE_GATTS_CHARACTERISTIC_ADD, err_code);
            }
            char_md.p_user_desc_md = &char_md_user_desc_md;
        }
        else
        {
            char_md.p_user_desc_md = NULL;
        }

        // CCCD metadata.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            RPC_DECODER_LENGTH_CHECK(command_len,
                                     (index + sizeof(ble_gatts_attr_md_t)),
                                     SD_BLE_GATTS_CHARACTERISTIC_ADD);

            err_code = attr_md_decode(&p_command[index], &char_md_cccd_md, &index);
            if (err_code != NRF_SUCCESS)
            {
                return ble_rpc_cmd_resp_send(SD_BLE_GATTS_CHARACTERISTIC_ADD, err_code);
            }
            char_md.p_cccd_md = &char_md_cccd_md;
        }
        else
        {
            char_md.p_cccd_md = NULL;
        }

        // SCCD metadata.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            RPC_DECODER_LENGTH_CHECK(command_len,
                                     (index + sizeof(ble_gatts_attr_md_t)),
                                     SD_BLE_GATTS_CHARACTERISTIC_ADD);
            err_code = attr_md_decode(&p_command[index], &char_md_sccd_md, &index);
            if (err_code != NRF_SUCCESS)
            {
                return ble_rpc_cmd_resp_send(SD_BLE_GATTS_CHARACTERISTIC_ADD, err_code);
            }
            char_md.p_sccd_md = &char_md_sccd_md;
        }
        else
        {
            char_md.p_sccd_md = NULL;
        }

        p_char_md = &char_md;
    }
    else
    {
        p_char_md = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);

    // Char attribute present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        // UUID present.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            attr_char_value_uuid.uuid  = uint16_decode(&p_command[index]);
            index                     += sizeof(uint16_t);

            attr_char_value_uuid.type = p_command[index++];
            attr_char_value.p_uuid    = &attr_char_value_uuid;
        }
        else
        {
            attr_char_value.p_uuid = NULL;
        }
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);
        // Attribute metadata present.
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            RPC_DECODER_LENGTH_CHECK(command_len,
                                     (index + sizeof(ble_gatts_attr_md_t)),
                                     SD_BLE_GATTS_CHARACTERISTIC_ADD);
            err_code = attr_md_decode(&p_command[index], &attr_char_value_attr_md, &index);
            if (err_code != NRF_SUCCESS)
            {
                return ble_rpc_cmd_resp_send(SD_BLE_GATTS_CHARACTERISTIC_ADD, err_code);
            }
            attr_char_value.p_attr_md = &attr_char_value_attr_md;
        }
        else
        {
            attr_char_value.p_attr_md = NULL;
        }

        attr_char_value.init_len   = uint16_decode(&p_command[index]);
        index                     += sizeof(uint16_t);
        attr_char_value.init_offs  = uint16_decode(&p_command[index]);
        index                     += sizeof(uint16_t);
        attr_char_value.max_len    = uint16_decode(&p_command[index]);
        index                     += sizeof(uint16_t);

        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);

        // Attribute data present
        if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
        {
            uint32_t data_index;
            RPC_DECODER_LENGTH_CHECK(command_len,
                                     (index + attr_char_value.init_len),
                                     SD_BLE_GATTS_CHARACTERISTIC_ADD);
            for (data_index = 0; data_index < attr_char_value.init_len; data_index++)
            {
                attr_data[data_index] = p_command[index++];
            }
            attr_char_value.p_value = attr_data;
        }
        else
        {
            attr_char_value.p_value = NULL;
        }
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);
        p_attr_char_value = &attr_char_value;
    }
    else
    {
        p_attr_char_value = NULL;
    }

    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        p_handles = &handles;
    }
    else
    {
        p_handles = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_GATTS_CHARACTERISTIC_ADD);

    err_code = sd_ble_gatts_characteristic_add(service_handle,
                                               p_char_md,
                                               p_attr_char_value,
                                               p_handles);

    if (p_handles != NULL)
    {
        index = 0;
        uint8_t resp_data[sizeof(ble_gatts_char_handles_t)];

        index += uint16_encode(p_handles->value_handle, &resp_data[index]);
        index += uint16_encode(p_handles->user_desc_handle, &resp_data[index]);
        index += uint16_encode(p_handles->cccd_handle, &resp_data[index]);
        index += uint16_encode(p_handles->sccd_handle, &resp_data[index]);

        return ble_rpc_cmd_resp_data_send(SD_BLE_GATTS_CHARACTERISTIC_ADD,
                                          err_code,
                                          resp_data,
                                          sizeof(resp_data));
    }

    return ble_rpc_cmd_resp_send(SD_BLE_GATTS_CHARACTERISTIC_ADD, err_code);
}


uint32_t ble_rpc_cmd_gatts_decode(uint8_t * p_command, uint8_t op_code, uint32_t command_len)
{
    uint32_t err_code;

    switch (op_code)
    {
        case SD_BLE_GATTS_SERVICE_ADD:
            err_code = gatts_service_add_handle(p_command, command_len);
            break;

        case SD_BLE_GATTS_SYS_ATTR_GET:
            err_code = gatts_sys_attr_get_handle(p_command, command_len);
            break;

        case SD_BLE_GATTS_SYS_ATTR_SET:
            err_code = gatts_sys_attr_set_handle(p_command, command_len);
            break;

        case SD_BLE_GATTS_VALUE_SET:
            err_code = gatts_value_set_handle(p_command, command_len);
            break;

        case SD_BLE_GATTS_HVX:
            err_code = gatts_hvx_handle(p_command, command_len);
            break;

        case SD_BLE_GATTS_CHARACTERISTIC_ADD:
            err_code = gatts_characteristic_add_handle(p_command, command_len);
            break;

        default:
            err_code = ble_rpc_cmd_resp_send(op_code, NRF_ERROR_NOT_SUPPORTED);
            break;
    }

    return err_code;
}
