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

#include "ble_gatts.h"
#include <string.h>
#include "app_util.h"
#include "ble_rpc_cmd_encoder.h"
#include "ble_rpc_defines.h"
#include "hci_transport.h"
#include "nordic_common.h"

                                     /**< These externals come from from the ble_rpc_dms_encoder.c file. */
extern uint8_t * g_cmd_response_buf; /**< Pointer to the buffer used for storing command response. */
extern uint8_t * g_cmd_buffer;       /**< Pointer to the buffer used for storing serialized commands. */

/**@brief Function for decoding the sd_ble_gatts_sys_attr_get API response data.
 *
 * @param[out] p_sys_attr_data Decode data buffer output.
 * @param[out] p_len           Decode data length output.
 */
static __INLINE void gatts_sys_attr_get_decode(uint8_t  * const p_sys_attr_data,
                                               uint16_t * const p_len)
{
    if (p_len != NULL)
    {
        *p_len = uint16_decode(&(g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                                    BLE_PKT_TYPE_SIZE +
                                                    RPC_ERR_CODE_SIZE]));

        const uint32_t attributes_data_present_offset = BLE_OP_CODE_SIZE  +
                                                        BLE_PKT_TYPE_SIZE +
                                                        RPC_ERR_CODE_SIZE +
                                                        sizeof(uint16_t);
        const uint32_t attributes_offset              = attributes_data_present_offset + 1u;

        if ((p_sys_attr_data != NULL) &&
            (g_cmd_response_buf[attributes_data_present_offset] == RPC_BLE_FIELD_PRESENT))
        {
            if (*p_len <= (RPC_BLE_PKT_MAX_SIZE - attributes_offset))
            {
                memcpy(p_sys_attr_data, &(g_cmd_response_buf[attributes_offset]), *p_len);
            }
            else
            {
                // @note: If this branch is executed, the length field has erroneous data since the 
                // g_cmd_buffer reception API is length checked.
                APP_ERROR_HANDLER(*p_len);
            }
        }
    }
}


/**@brief Function for encoding characteristics in the SD_BLE_GATTS_CHARACTERISTIC_ADD command.
 *
 * @param[in]     p_gatts_attr_md  Pointer to the gatts_attr_md to be encoded.
 * @param[in,out] p_packet         Pointer to the packet buffer where the encoded data is stored.
 *
 * @return The number of bytes encoded.
 */
static uint32_t gatts_char_md_encode(ble_gatts_attr_md_t * p_gatts_attr_md, uint8_t * p_packet)
{
    uint32_t index = 0;

    p_packet[index++] = (p_gatts_attr_md->read_perm.sm  | p_gatts_attr_md->read_perm.lv << 4 );
    p_packet[index++] = (p_gatts_attr_md->write_perm.sm | p_gatts_attr_md->write_perm.lv << 4 );
    p_packet[index++] = (p_gatts_attr_md->vlen    << 0 |
                         p_gatts_attr_md->vloc    << 1 |
                         p_gatts_attr_md->rd_auth << 3 |
                         p_gatts_attr_md->wr_auth << 4);
    return index;
}


uint32_t sd_ble_gatts_service_add(uint8_t                  type,
                                  ble_uuid_t const * const p_uuid,
                                  uint16_t         * const p_handle)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GATTS_SERVICE_ADD;
    g_cmd_buffer[index++] = type;

    if (p_uuid != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(p_uuid->uuid, &g_cmd_buffer[index]);
        g_cmd_buffer[index++] = p_uuid->type;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    if (p_handle != NULL)
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

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GATTS_SERVICE_ADD);
    if (p_handle != NULL)
    {
        *p_handle = uint16_decode(&(g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                                       BLE_PKT_TYPE_SIZE +
                                                       RPC_ERR_CODE_SIZE]));
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gatts_sys_attr_set(uint16_t              conn_handle,
                                   uint8_t const * const p_sys_attr_data,
                                   uint16_t              len)
{
    if (len <= (RPC_BLE_PKT_MAX_SIZE - BLE_OP_CODE_SIZE - BLE_PKT_TYPE_SIZE - sizeof(conn_handle)))
    {
        uint32_t index = 0;

        g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
        g_cmd_buffer[index++] = SD_BLE_GATTS_SYS_ATTR_SET;
        index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);

        if (p_sys_attr_data != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += uint16_encode(len, &g_cmd_buffer[index]);

            // The lint warning suppression below is done under the assumption that the application
            // will ensure the correct value of len (the actual length of p_sys_attr_data) is 
            // passed.
            memcpy(&(g_cmd_buffer[index]), p_sys_attr_data, len); //lint -e670 Possible access beyond array.
            index += len;
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

        err_code = ble_rpc_cmd_resp_wait(SD_BLE_GATTS_SYS_ATTR_SET);
        UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
        return err_code;
    }
    else
    {
        return NRF_ERROR_NO_MEM;
    }
}


uint32_t sd_ble_gatts_hvx(uint16_t conn_handle, ble_gatts_hvx_params_t const * const p_hvx_params)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GATTS_HVX;
    index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);

    if (p_hvx_params != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(p_hvx_params->handle, &g_cmd_buffer[index]);
        g_cmd_buffer[index++] = p_hvx_params->type;
        index                += uint16_encode(p_hvx_params->offset, &g_cmd_buffer[index]);

        if (p_hvx_params->p_len != NULL)
        {
            if (*(p_hvx_params->p_len) > (
                                             RPC_BLE_PKT_MAX_SIZE                -
                                             (
                                                 BLE_OP_CODE_SIZE                +
                                                 BLE_PKT_TYPE_SIZE               +
                                                 sizeof(conn_handle)             +
                                                 RPC_BLE_FIELD_LEN               +
                                                 sizeof(p_hvx_params->handle)    +
                                                 sizeof(p_hvx_params->type)      +
                                                 sizeof(p_hvx_params->offset)    +
                                                 RPC_BLE_FIELD_LEN               +
                                                 sizeof(*(p_hvx_params->p_len))  +
                                                 RPC_BLE_FIELD_LEN
                                             )
                                         )
                )
            {
                return NRF_ERROR_DATA_SIZE;
            }

            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += uint16_encode(*(p_hvx_params->p_len), &g_cmd_buffer[index]);

            if (p_hvx_params->p_data != NULL)
            {
                g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
                memcpy(&(g_cmd_buffer[index]), p_hvx_params->p_data, *(p_hvx_params->p_len));
                index                += *(p_hvx_params->p_len);
            }
            else
            {
                g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
            }
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
            // @note: The data field is omitted if the length field is also omitted.
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
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

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GATTS_HVX);
    if ((p_hvx_params != NULL) && (p_hvx_params->p_len != NULL))
    {
        *(p_hvx_params->p_len) = uint16_decode(&
                                 (g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                                     BLE_PKT_TYPE_SIZE +
                                                     RPC_ERR_CODE_SIZE]));
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gatts_characteristic_add(uint16_t                    service_handle,
                                         ble_gatts_char_md_t const * const p_char_md,
                                         ble_gatts_attr_t const    * const p_attr_char_value,
                                         ble_gatts_char_handles_t  * const p_handles)
{
    uint32_t index = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GATTS_CHARACTERISTIC_ADD;
    index                += uint16_encode(service_handle, &g_cmd_buffer[index]);

    if (p_char_md != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        g_cmd_buffer[index++] = ((p_char_md->char_props.broadcast      << 0)|
                                 (p_char_md->char_props.read           << 1)|
                                 (p_char_md->char_props.write_wo_resp  << 2)|
                                 (p_char_md->char_props.write          << 3)|
                                 (p_char_md->char_props.notify         << 4)|
                                 (p_char_md->char_props.indicate       << 5)|
                                 (p_char_md->char_props.auth_signed_wr << 6));
        g_cmd_buffer[index++] = ((p_char_md->char_ext_props.reliable_wr << 0)|
                                 (p_char_md->char_ext_props.wr_aux      << 1));
        index                += uint16_encode(p_char_md->char_user_desc_max_size, 
                                              &g_cmd_buffer[index]);
        index                += uint16_encode(p_char_md->char_user_desc_size, &g_cmd_buffer[index]);

        if (p_char_md->p_char_user_desc != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            memcpy(&(g_cmd_buffer[index]), 
                   p_char_md->p_char_user_desc, 
                   p_char_md->char_user_desc_size);
            index                += p_char_md->char_user_desc_size;
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }

        if (p_char_md->p_char_pf != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            g_cmd_buffer[index++] = p_char_md->p_char_pf->format;
            g_cmd_buffer[index++] = p_char_md->p_char_pf->exponent;
            index                += uint16_encode(p_char_md->p_char_pf->unit, &g_cmd_buffer[index]);
            g_cmd_buffer[index++] = p_char_md->p_char_pf->name_space;
            index                += uint16_encode(p_char_md->p_char_pf->desc, &g_cmd_buffer[index]);
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }

        if (p_char_md->p_user_desc_md != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += gatts_char_md_encode(p_char_md->p_user_desc_md, 
                                                         &g_cmd_buffer[index]);
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }

        if (p_char_md->p_cccd_md != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += gatts_char_md_encode(p_char_md->p_cccd_md, 
                                                         &g_cmd_buffer[index]);
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }

        if (p_char_md->p_sccd_md != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += gatts_char_md_encode(p_char_md->p_sccd_md, 
                                                         &g_cmd_buffer[index]);
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    if (p_attr_char_value != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        if (p_attr_char_value->p_uuid != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += uint16_encode(p_attr_char_value->p_uuid->uuid, 
                                                  &g_cmd_buffer[index]);
            g_cmd_buffer[index++] = p_attr_char_value->p_uuid->type;
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }

        if (p_attr_char_value->p_attr_md != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            index                += gatts_char_md_encode(p_attr_char_value->p_attr_md, 
                                                         &g_cmd_buffer[index]);
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }

        index += uint16_encode(p_attr_char_value->init_len, &g_cmd_buffer[index]);
        index += uint16_encode(p_attr_char_value->init_offs, &g_cmd_buffer[index]);
        index += uint16_encode(p_attr_char_value->max_len, &g_cmd_buffer[index]);

        if (p_attr_char_value->p_value != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            memcpy(&(g_cmd_buffer[index]), p_attr_char_value->p_value, p_attr_char_value->init_len);
            index                += p_attr_char_value->init_len;
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }
    if (p_handles !=NULL)
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

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GATTS_CHARACTERISTIC_ADD);

    if (p_handles != NULL)
    {
        index                       = BLE_OP_CODE_SIZE + BLE_PKT_TYPE_SIZE + RPC_ERR_CODE_SIZE;
        p_handles->value_handle     = uint16_decode(&g_cmd_response_buf[index]);
        index                      += sizeof(uint16_t);
        p_handles->user_desc_handle = uint16_decode(&g_cmd_response_buf[index]);
        index                      += sizeof(uint16_t);
        p_handles->cccd_handle      = uint16_decode(&g_cmd_response_buf[index]);
        index                      += sizeof(uint16_t);
        p_handles->sccd_handle      = uint16_decode(&g_cmd_response_buf[index]);
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gatts_value_set(uint16_t              handle,
                                uint16_t              offset,
                                uint16_t * const      p_len,
                                uint8_t const * const p_value)
{
    uint32_t index  = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GATTS_VALUE_SET;
    index                += uint16_encode(handle, &g_cmd_buffer[index]);
    index                += uint16_encode(offset, &g_cmd_buffer[index]);

    if (p_len != NULL)
    {
        if ((*p_len) > (
            RPC_BLE_PKT_MAX_SIZE       -
            (BLE_OP_CODE_SIZE          +
            BLE_PKT_TYPE_SIZE          +
            sizeof(handle)             +
            sizeof(offset)             +
            RPC_BLE_FIELD_LEN          +
            sizeof(*p_len)             +
            RPC_BLE_FIELD_LEN)))
        {
            return NRF_ERROR_DATA_SIZE;
        }

        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(*p_len, &g_cmd_buffer[index]);

        if (p_value != NULL)
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
            memcpy(&(g_cmd_buffer[index]), p_value, *p_len);
            index                += *p_len;
        }
        else
        {
            g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
        }
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;

        // @note: If length field is omitted value field must also be omitted.
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GATTS_VALUE_SET);

    if (p_len != NULL)
    {
        *p_len = g_cmd_response_buf[BLE_OP_CODE_SIZE + BLE_PKT_TYPE_SIZE + RPC_ERR_CODE_SIZE];
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gatts_sys_attr_get(uint16_t         conn_handle,
                                   uint8_t  * const p_sys_attr_data,
                                   uint16_t * const p_len)
{
    uint32_t index  = 0;

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_GATTS_SYS_ATTR_GET;
    index                += uint16_encode(conn_handle, &g_cmd_buffer[index]);

    if (p_len != NULL)
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
        index                += uint16_encode(*p_len, &g_cmd_buffer[index]);
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }

    g_cmd_buffer[index++] = (p_sys_attr_data != NULL) ? RPC_BLE_FIELD_PRESENT :
                                                        RPC_BLE_FIELD_NOT_PRESENT;

    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = ble_rpc_cmd_resp_wait(SD_BLE_GATTS_SYS_ATTR_GET);
    if (err_code == NRF_SUCCESS)
    {
        gatts_sys_attr_get_decode(p_sys_attr_data, p_len);
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    return err_code;
}


uint32_t sd_ble_gatts_descriptor_add(uint16_t                       char_handle,
                                     ble_gatts_attr_t const * const p_attr,
                                     uint16_t* const                p_handle)
{
    // Not implemented yet.
    return NRF_ERROR_NOT_SUPPORTED;
}
