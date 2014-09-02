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

#include "ble_rpc_cmd_encoder.h"
#include <stdint.h>
#include <string.h>
#include "nrf51.h"
#include "ble.h"
#include "nrf_soc.h"
#include "app_util.h"
#include "hci_transport.h"
#include "nordic_common.h"

#define INVALID_OP_CODE 0xFF                 /**< Invalid operation code used for invalidating command response. */

uint8_t *               g_cmd_buffer;        /**< Pointer to the buffer used for storing serialized commands. */
uint8_t *               g_cmd_response_buf;  /**< Pointer to the buffer used for storing command response. */
volatile cmd_response_t g_cmd_response;      /**< Response of last received command/response. */
volatile bool           m_tx_done = false;   /**< Variable used for signaling when transmission of the last command is complete. */


void ble_rpc_cmd_rsp_pkt_received(uint8_t * packet, uint16_t packet_length)
{
    uint32_t index = 0;

    g_cmd_response_buf = packet;

    if (g_cmd_response_buf[index++] != BLE_RPC_PKT_RESP)
    {
        // Received a packet that is not of interest to the command encoder.
        return;
    }

    g_cmd_response.op_code = g_cmd_response_buf[index++];

    if (packet_length >= RPC_BLE_CMD_RESP_PKT_MIN_SIZE)
    {
        g_cmd_response.err_code = uint32_decode(&g_cmd_response_buf[index++]);
    }
    else
    {
        g_cmd_response.err_code = NRF_ERROR_INTERNAL;
    }
}

/**@brief Function for the transport layer to indicate when it has finished transmitting the bytes from 
 *        the TX buffer.
 *
 * @param[in] result    TX done event result code.
 */
void transport_tx_complete_handler(hci_transport_tx_done_result_t result)
{
    m_tx_done = true;
}


/**@brief Function for blocking in a loop, using WFE to allow low power mode, while waiting for a
 *        response from the connectivity chip.
 *
 * @param[in] op_code   The Operation Code for which a response message is expected.
 *
 * @return    The decoded error code received from the connectivity chip.
 */
uint32_t ble_rpc_cmd_resp_wait(uint8_t op_code)
{
    for (;;)
    {
        __WFE();

        if (m_tx_done && (g_cmd_response.op_code == op_code))
        {
            m_tx_done              = false;
            g_cmd_response.op_code = INVALID_OP_CODE;

            return g_cmd_response.err_code;
        }
    }
}


uint32_t wait_for_tx_done(void)
{
    for (;;)
    {
        __WFE();

        if (m_tx_done)
        {
            m_tx_done = false;
            return NRF_SUCCESS;
        }
    }
}


uint32_t sd_ble_uuid_encode(ble_uuid_t const * const p_uuid,
                            uint8_t          * const p_uuid_le_len,
                            uint8_t          * const p_uuid_le)
{
    uint32_t err_code;
    uint32_t index = 0;
    
    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_BLE_UUID_ENCODE;

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
    if (p_uuid_le_len != NULL)
    {        
        g_cmd_buffer[index++] = RPC_BLE_FIELD_PRESENT;
    }
    else
    {
        g_cmd_buffer[index++] = RPC_BLE_FIELD_NOT_PRESENT;
    }        
    if (p_uuid_le != NULL)
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
        
    err_code = ble_rpc_cmd_resp_wait(SD_BLE_UUID_ENCODE);
    if (p_uuid_le_len != NULL)
    {
        *p_uuid_le_len = g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                            BLE_PKT_TYPE_SIZE +
                                            RPC_ERR_CODE_SIZE];
    }
    if ((p_uuid_le != NULL) && (p_uuid_le_len != NULL))
    {
        memcpy(p_uuid_le, 
               &(g_cmd_response_buf[BLE_OP_CODE_SIZE  +
                                    BLE_PKT_TYPE_SIZE +
                                    RPC_ERR_CODE_SIZE +
                                    sizeof(*p_uuid_le_len)]),
               *p_uuid_le_len);
    }

    UNUSED_VARIABLE(hci_transport_rx_pkt_consume(g_cmd_response_buf));
    
    return err_code;
}


uint32_t sd_power_system_off(void)
{
    // @note: This function will request the Connectivity Chip to enter System Off mode.
    //        No response is expected from the Connectivity Chip.
    uint32_t index = 0; 

    g_cmd_buffer[index++] = BLE_RPC_PKT_CMD;
    g_cmd_buffer[index++] = SD_POWER_SYSTEM_OFF;
    
    uint32_t err_code = hci_transport_pkt_write(g_cmd_buffer, index);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return wait_for_tx_done();
}


uint32_t ble_rpc_cmd_encoder_init(void)
{
    // Allocate memory for serialized commands. This module will use the same memory for all
    // commands.
    uint32_t err_code;
    
    err_code = hci_transport_tx_done_register(transport_tx_complete_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return (hci_transport_tx_alloc(&g_cmd_buffer));
}
