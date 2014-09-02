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


#include "ble_rpc_event_encoder.h"
#include "ble_rpc_event_encoder_gap.h"
#include "ble_rpc_event_encoder_gatts.h"
#include <string.h>
#include <stdint.h>
#include "app_util.h"
#include "ble_gap.h"
#include "nrf_error.h"
#include "hci_transport.h"


void ble_rpc_event_handle(ble_evt_t * p_ble_evt)
{
    uint32_t  err_code;
    uint8_t * p_buffer        = NULL;
    uint8_t   evt_packet_size = 0;

    // Allocate a memory buffer from HCI transport layer for transmitting the Command Response.
    // Loop until a buffer is available.
    do
    {
        err_code = hci_transport_tx_alloc(&p_buffer);
    }
    while (err_code == NRF_ERROR_NO_MEM);

    APP_ERROR_CHECK(err_code);

    uint16_t event_id = p_ble_evt->header.evt_id;

    if ((BLE_GAP_EVT_BASE <= event_id) && (event_id < BLE_GAP_EVT_LAST))
    {
        evt_packet_size = ble_rpc_evt_gap_encode(p_ble_evt, p_buffer);
    }
    else if ((BLE_GATTS_EVT_BASE <= event_id) && (event_id < BLE_GATTS_EVT_LAST))
    {
        evt_packet_size = ble_rpc_evt_gatts_encode(p_ble_evt, p_buffer);
    }
    else
    {
        // Do nothing.
    }

    if (evt_packet_size != 0)
    {
        err_code = hci_transport_pkt_write(p_buffer, evt_packet_size);
        APP_ERROR_CHECK(err_code);
        // @note: TX buffer must be freed upon TX done event from HCI Transport layer. The BLE 
        // connectivity example project handles this in main.c.
    }
    else
    {
        // No event was encoded, therefore the buffer is freed immediately.
        err_code = hci_transport_tx_free();
        APP_ERROR_CHECK(err_code);
    }
}

