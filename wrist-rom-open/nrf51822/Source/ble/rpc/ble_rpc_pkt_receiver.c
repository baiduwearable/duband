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

#include "ble_rpc_event_decoder.h"
#include "ble_rpc_cmd_encoder.h"
#include "ble_rpc_defines.h"
#include "hci_transport.h"
#include "app_error.h"
#include <stdint.h>
#include <stddef.h>
#include "nrf51.h"
#include "ble_stack_handler.h"
#include "app_scheduler.h"




/** @brief Function for handling events from transport layer.
 *
 * @param pkt_type   Type of the event received.
 */
void hci_pkt_handler(hci_transport_evt_t hci_event)
{
    if (hci_event.evt_type == HCI_TRANSPORT_RX_RDY)
    {
        uint32_t err_code;

        uint8_t *   p_encoded_packet;
        uint32_t    encoded_packet_length;

        err_code = hci_transport_rx_pkt_extract(&p_encoded_packet, &encoded_packet_length);

        if ((err_code == NRF_SUCCESS) && (p_encoded_packet != NULL))
        {
            if (p_encoded_packet[0] == BLE_RPC_PKT_RESP)
            {
                ble_rpc_cmd_rsp_pkt_received(p_encoded_packet, (uint16_t) encoded_packet_length);
            }
            else if (p_encoded_packet[0] == BLE_RPC_PKT_EVT)
            {
                err_code = ble_rpc_event_pkt_received(p_encoded_packet,
                                                      (uint16_t) encoded_packet_length);
                APP_ERROR_CHECK(err_code);

                err_code = app_sched_event_put(NULL, 0, ble_stack_evt_get);
            }
            else
            {
                (void) hci_transport_rx_pkt_consume(p_encoded_packet);
            }
        }

        APP_ERROR_CHECK(err_code);
    }
}


uint32_t ble_rpc_pkt_receiver_init(void)
{
    uint32_t err_code;

    // The caller is interested in BLE events. Register with transport layer for events.
    err_code = hci_transport_open();

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = hci_transport_evt_handler_reg(hci_pkt_handler);
    if (err_code != NRF_SUCCESS)
    {
        (void)hci_transport_close();
    }

    return err_code;
}

