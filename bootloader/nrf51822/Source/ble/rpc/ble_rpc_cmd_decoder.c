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

#include "ble_rpc_cmd_decoder.h"
#include "ble_rpc_cmd_decoder_gap.h"
#include "ble_rpc_cmd_decoder_gatts.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "hci_transport.h"
#include "ble_rpc_defines.h"
#include "nrf_error.h"
#include "app_util.h"
#include "ble.h"
#include "nrf_soc.h"
#include "ble_gatts.h"
#include "ble_ranges.h"
#include "nordic_common.h"

uint32_t ble_rpc_cmd_resp_send(uint8_t op_code, uint32_t status)
{
    uint8_t * p_buffer;
    uint32_t  err_code;
    uint32_t  index = 0;

    // Allocate a memory buffer from HCI transport layer for transmitting the Command Response.
    // Loop until a buffer is available.
    do
    {    
        err_code = hci_transport_tx_alloc(&p_buffer);
    }
    while (err_code == NRF_ERROR_NO_MEM);

    if (err_code == NRF_SUCCESS)
    {
        // Encode packet type.
        p_buffer[index++] = BLE_RPC_PKT_RESP;

        // Encode Op Code.
        p_buffer[index++] = op_code;

        // Encode Status.
        index += uint32_encode(status, &(p_buffer[RPC_CMD_RESP_STATUS_POS]));

        return hci_transport_pkt_write(p_buffer, index);
    }

    return err_code;
}


uint32_t ble_rpc_cmd_resp_data_send(uint8_t               op_code,
                                    uint8_t               status,
                                    const uint8_t * const p_data,
                                    uint16_t              data_len)
{
    uint8_t * p_buffer;
    uint32_t  err_code;
    uint32_t  index = 0;

    // Allocate a memory buffer from HCI transport layer for transmitting the Command Response.
    // Loop until a buffer is available.
    do
    {    
        err_code = hci_transport_tx_alloc(&p_buffer);
    }
    while (err_code == NRF_ERROR_NO_MEM);

    if (err_code == NRF_SUCCESS)
    {
        // Encode packet type.
        p_buffer[index++] = BLE_RPC_PKT_RESP;

        // Encode Op Code.
        p_buffer[index++] = op_code;

        // Encode Status.
        index += uint32_encode(status, &(p_buffer[index]));

        // Additional data in response packet.
        memcpy(&p_buffer[index], p_data, data_len);

        index += data_len;

        return hci_transport_pkt_write(p_buffer, index);
    }

    return err_code;
}


/**@brief Function for decoding a command packet with RPC_SD_BLE_UUID_ENCODE opcode.
 *
 * This function will decode the command, call the BLE Stack API, and also send command response
 * to the peer through the transport layer.
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
static uint32_t uuid_encode_handle(uint8_t * p_command, uint32_t command_len)
{
    ble_uuid_t uuid_data;
    uint32_t   err_code;

    // uuid can be up to 16 bytes, and 1 byte length field
    uint8_t      response_buffer[sizeof(ble_uuid128_t) + sizeof(uint8_t)];

    uint32_t     index           = 0;
    ble_uuid_t * p_uuid_data     = &uuid_data;
    uint8_t *    p_length        = &(response_buffer[0]);
    uint8_t *    p_result_buffer = &(response_buffer[1]);

    // UUID field present.
    if (p_command[index++] == RPC_BLE_FIELD_PRESENT)
    {
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_UUID_ENCODE);
        uuid_data.uuid  = uint16_decode(&(p_command[index]));
        index          += sizeof(uint16_t);
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_UUID_ENCODE);
        uuid_data.type  = p_command[index++];
    }
    else
    {
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_UUID_ENCODE);
        p_uuid_data = NULL;
    }

    // Length field not present.
    if (p_command[index++] == RPC_BLE_FIELD_NOT_PRESENT)
    {
        RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_UUID_ENCODE);
        p_length = NULL;
    }

    // Result buffer not present.
    if (p_command[index++] == RPC_BLE_FIELD_NOT_PRESENT)
    {
        p_result_buffer = NULL;
    }

    RPC_DECODER_LENGTH_CHECK(command_len, index, SD_BLE_UUID_ENCODE);

    err_code = sd_ble_uuid_encode(p_uuid_data, p_length, p_result_buffer);

    if (err_code == NRF_SUCCESS)
    {
        return ble_rpc_cmd_resp_data_send(SD_BLE_UUID_ENCODE,
                                          NRF_SUCCESS,
                                          response_buffer,
                                          response_buffer[0] + sizeof(uint8_t));
    }

    return ble_rpc_cmd_resp_send(SD_BLE_UUID_ENCODE, err_code);
}


/**@brief Function for handling the RPC_SD_POWER_SYSTEM_OFF command.
 *
 * This function will decode the command, call the SOC API and put the chip into system off
 * mode. It will always return error code NRF_SUCCESS.
 *
 * @return NRF_SUCCESS
 */
static uint32_t sd_power_system_off_handle(void)
{
    UNUSED_VARIABLE(sd_power_system_off());
    return NRF_SUCCESS;
}


/**@brief Function for processing an encoded BLE base command from Application Chip.
 *
 * @details     This function will decode the encoded command and call the appropriate BLE Stack
 *              API. It will then create a Command Response packet with the return value from the
 *              stack API encoded in it and will send it to the transport layer for transmission to
 *              the application controller chip.

 * @param[in]   p_command       The encoded command.
 * @param[in]   op_code         Operation code of the command.
 * @param[in]   command_len     Length of the encoded command.
 *
 * @retval      NRF_SUCCESS          If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 *                                   If the transport layer returns an error code while sending
 *                                   the Command Response, the same error code will be returned by
 *                                   this function (see @ref hci_transport_pkt_write for the list
 *                                   of error codes).
 */
static uint32_t ble_base_cmd_decode(uint8_t * p_command, uint8_t op_code, uint32_t command_len)
{
    uint32_t err_code;

    switch (op_code)
    {
        case SD_BLE_UUID_ENCODE:
            err_code = uuid_encode_handle(p_command, command_len);
            break;

        default:
            err_code = ble_rpc_cmd_resp_send(op_code, NRF_ERROR_NOT_SUPPORTED);
            break;
    }

    return err_code;
}

/**@brief Function for processing an encoded SOC command from Application Chip.
 *
 * @details     This function will decode the encoded command and call the appropriate SOC
 *              API. It will then create a Command Response packet with the return value from the
 *              stack API encoded in it and will send it to the transport layer for transmission to
 *              the application controller chip.

 * @param[in]   p_command       The encoded command.
 * @param[in]   op_code         Operation code of the command.
 * @param[in]   command_len     Length of the encoded command.
 *
 * @retval      NRF_SUCCESS          If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 *                                   If the transport layer returns an error code while sending
 *                                   the Command Response, the same error code will be returned by
 *                                   this function (see @ref hci_transport_pkt_write for the list
 *                                   of error codes).
 */
static uint32_t soc_cmd_decode(uint8_t * p_command, uint8_t op_code, uint32_t command_len)
{
    uint32_t err_code;

    switch (op_code)
    {
        case SD_POWER_SYSTEM_OFF:
            err_code = sd_power_system_off_handle();
            break;

        default:
            err_code = ble_rpc_cmd_resp_send(op_code, NRF_ERROR_NOT_SUPPORTED);
            break;
    }

    return err_code;
}



/**@brief Function for parsing the operation code of an encoded command and redirect decoding of
 *        the command to the appropriate decoder function.
 *
 * @param[in]   p_command       The encoded command.
 * @param[in]   command_len     Length of the encoded command including op code.
 *
 * @retval      NRF_SUCCESS          If the decoding of the command was successful, the SoftDevice
 *                                   API was called, and the command response was sent to peer,
 *                                   otherwise an error code.
 *                                   If the transport layer returns an error code while sending
 *                                   the Command Response, the same error code will be returned by
 *                                   this function (see @ref hci_transport_pkt_write for the list
 *                                   of error codes).
 */
static uint32_t command_process(uint8_t * p_command, uint32_t command_len)
{
    uint8_t  op_code;
    uint32_t err_code;

    command_len = command_len - BLE_OP_CODE_SIZE;
    op_code     = p_command[RPC_CMD_OP_CODE_POS];

    // If BLE base command opcode.
    if ((op_code >= BLE_SVC_BASE) && (op_code <= BLE_SVC_LAST))
    {
        err_code = ble_base_cmd_decode(&p_command[RPC_CMD_DATA_POS], op_code, command_len);
    }
    // If GAP command opcode.
    else if ((op_code >= BLE_GAP_SVC_BASE) && (op_code <= BLE_GAP_SVC_LAST))
    {
        err_code = ble_rpc_cmd_gap_decode(&p_command[RPC_CMD_DATA_POS], op_code, command_len);
    }
    // If GATTS command opcode.
    else if ((op_code >= BLE_GATTS_SVC_BASE) && (op_code <= BLE_GATTS_SVC_LAST))
    {
        err_code = ble_rpc_cmd_gatts_decode(&p_command[RPC_CMD_DATA_POS], op_code, command_len);
    }
    // If SOC of command opcode.
    else if ((op_code >= SOC_SVC_BASE) && (op_code < SVC_SOC_LAST))
    {
        err_code = soc_cmd_decode(&p_command[RPC_CMD_DATA_POS], op_code, command_len);
    }
    // The operation code is not part of any decoder.
    else
    {
        err_code = ble_rpc_cmd_resp_send(op_code, NRF_ERROR_NOT_SUPPORTED);
    }

    return err_code;
}


void ble_rpc_cmd_handle(void * p_event_data, uint16_t event_size)
{
    uint32_t  err_code;
    uint32_t  rpc_cmd_length_read;
    uint8_t * p_rpc_cmd_buffer;

    err_code = hci_transport_rx_pkt_extract(&p_rpc_cmd_buffer, &rpc_cmd_length_read);
    APP_ERROR_CHECK(err_code);

    err_code = command_process(&p_rpc_cmd_buffer[RPC_CMD_RESP_OP_CODE_POS], rpc_cmd_length_read);
    APP_ERROR_CHECK(err_code);

    err_code = hci_transport_rx_pkt_consume(p_rpc_cmd_buffer);
    APP_ERROR_CHECK(err_code);
}


