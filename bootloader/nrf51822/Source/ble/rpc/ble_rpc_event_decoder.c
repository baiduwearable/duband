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
#include "ble_rpc_event_decoder_gap.h"
#include "ble_rpc_event_decoder_gatts.h"
#include "ble_rpc_defines.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_error.h"
#include "ble.h"
#include "app_util.h"
#include "hci_transport.h"

#define EVENT_QUEUE_SIZE                    3                     /**< Event queue size for the event decoder. If the queue is full an error is returned. @note This value should be 1 less than the number of RX buffers defined as \ref RX_BUF_QUEUE_SIZE in \ref hci_mem_pool.c to always allow a command response to be received. */
#define EVENT_ID_POSITION                   1                     /**< Position in encoded packet for the Event Id. */

/** @brief Macro checking if a pointer is aligned to the required boundary. */
#define IS_ALIGNED(ptr, byte_boundary) (                                         \
                                        (                                        \
                                         (ptr + (byte_boundary - 1))             \
                                         &                                       \
                                         (~(byte_boundary - 1))                  \
                                        )                                        \
                                        ==                                       \
                                        ptr                                      \
                                       )

typedef struct 
{
    uint8_t * p_packet;                                           /**< Pointer to a received BLE event packet. */
    uint16_t  packet_length;                                      /**< Length of the received BLE event packet. */
} ble_rpc_event_packet_t;

static ble_rpc_event_packet_t m_packet_queue[EVENT_QUEUE_SIZE];   /**< Queue for holding received packets until they are fetched by application through \ref sd_ble_evt_get . */
static uint8_t                m_packet_queue_write_index;         /**< Write index where next packet will be stored. */
static uint8_t                m_packet_queue_read_index;          /**< Read index for oldest packet in the queue. */


/** @brief Function for decoding a BLE event. The decoded BLE event will be returned in the memory
 *         pointed to by p_ble_evt.
 *
 *         This function parses the Event Id in the encoded packet and dispatches the rest of the
 *         packet to GAP or GATTS decoder for further processing.
 *
 * @param[out]  p_ble_evt       The pointer for storing the decoded event.
 * @param[in]   p_packet        The pointer to the encoded event.
 * @param[in]   packet_length   The length of the encoded event.
 */
static void evt_packet_decode(ble_evt_t *           p_ble_evt,
                              uint8_t const * const p_packet,
                              uint16_t              packet_length)
{
    uint32_t event_id = p_packet[EVENT_ID_POSITION];

    if ((BLE_GAP_EVT_BASE <= event_id) && (event_id < BLE_GAP_EVT_LAST))
    {
        ble_rpc_gap_evt_packet_decode(p_ble_evt,
                                      &p_packet[EVENT_ID_POSITION]);
    }
    else if ((BLE_GATTS_EVT_BASE <= event_id) && (event_id < BLE_GATTS_EVT_LAST))
    {
        ble_rpc_gatts_evt_packet_decode(p_ble_evt,
                                        &p_packet[EVENT_ID_POSITION]);
    }
    else
    {
        // Do nothing.
    }
}


/** @brief Function for decoding the length of a BLE event. The decoded BLE event length
 *         will be returned in p_event_length.
 *
 *         This function parses the Event Id in the encoded packet and dispatches the length 
 *         decoding to GAP or GATTS length decoder for further processing.
 *
 * @param[out]  p_event_length  The pointer for storing the decoded event length.
 * @param[in]   p_packet        The pointer to the encoded event.
 * @param[in]   packet_length   The length of the encoded event.
 */
static void evt_length_decode(uint16_t *            p_event_length,
                              uint8_t const * const p_packet,
                              uint16_t              packet_length)
{
    uint32_t event_id = p_packet[EVENT_ID_POSITION];

    if ((BLE_GAP_EVT_BASE <= event_id) && (event_id < BLE_GAP_EVT_LAST))
    {
        ble_rpc_gap_evt_length_decode(event_id, p_event_length);
    }
    else if ((BLE_GATTS_EVT_BASE <= event_id) && (event_id < BLE_GATTS_EVT_LAST))
    {
        ble_rpc_gatts_evt_length_decode(event_id,
                                        p_event_length,
                                        &p_packet[EVENT_ID_POSITION]);
    }
    else
    {
        // Unsupported (or invalid) event, set length to zero to indicate no event available.
        *p_event_length = 0;
    }
}


uint32_t sd_ble_evt_get(uint8_t * p_dest, uint16_t * p_len)
{
    ble_evt_t * p_ble_evt = (ble_evt_t *)p_dest;
    uint16_t    evt_length;
    uint8_t     pop_index;
    uint8_t *   p_packet;
    uint16_t    packet_length;

    if (p_len == NULL)
    {
        return NRF_ERROR_INVALID_ADDR;
    }

    // Check pointer alignment.
    if (!IS_ALIGNED((uint32_t) p_dest, BLE_EVTS_PTR_ALIGNMENT))
    {
        return NRF_ERROR_INVALID_ADDR;
    }

    pop_index      = m_packet_queue_read_index;
    p_packet       = m_packet_queue[pop_index].p_packet;
    packet_length  = m_packet_queue[pop_index].packet_length;

    // Check if there is any event received.
    if (p_packet == NULL)
    {
        // No event received.
        *p_len = 0;
        return NRF_ERROR_NOT_FOUND;
    }

    if (p_packet[0] != BLE_RPC_PKT_EVT)
    {
        // No event received.
        *p_len = 0;
        return NRF_ERROR_NOT_FOUND;
    }

    evt_length_decode(&evt_length, p_packet, packet_length);

    if (evt_length == 0)
    {
        // Unsupported/Invalid event received - put packet pointer to NULL.
        UNUSED_VARIABLE(hci_transport_rx_pkt_consume(p_packet));

        m_packet_queue[pop_index].p_packet = NULL;

        if (++m_packet_queue_read_index >= EVENT_QUEUE_SIZE)
        {
            m_packet_queue_read_index = 0;
        }

        *p_len = 0;

        return NRF_ERROR_NOT_FOUND;
    }

    uint16_t ble_evt_len = evt_length + sizeof(ble_evt_hdr_t);

    // Check that the provided buffer is large enough.
    if ((p_dest != NULL) && (*p_len < ble_evt_len))
    {
        // Not enough memory provided to fit the event.
        *p_len = 0;
        return NRF_ERROR_DATA_SIZE;
    }

    // Set the calculated length of the event as output.
    *p_len = ble_evt_len;

    if (p_dest != NULL)
    {
        // Decode the encoded event data.
        p_ble_evt->header.evt_len = evt_length;
        evt_packet_decode(p_ble_evt, p_packet, packet_length);

        // Clear the encoded packet to invalidate it.
        UNUSED_VARIABLE(hci_transport_rx_pkt_consume(p_packet));
        m_packet_queue[pop_index].p_packet = NULL;

        if (++m_packet_queue_read_index >= EVENT_QUEUE_SIZE)
        {
            m_packet_queue_read_index = 0;
        }
    }

    return NRF_SUCCESS;
}


uint32_t ble_rpc_event_pkt_received(uint8_t * p_event_packet, uint16_t event_packet_length)
{
    if (m_packet_queue[m_packet_queue_write_index].p_packet != NULL)
    {
        UNUSED_VARIABLE(hci_transport_rx_pkt_consume(p_event_packet));
        return NRF_ERROR_NO_MEM;
    }
    
    m_packet_queue[m_packet_queue_write_index].p_packet      = p_event_packet;
    m_packet_queue[m_packet_queue_write_index].packet_length = event_packet_length;

    if (++m_packet_queue_write_index >= EVENT_QUEUE_SIZE)
    {
        m_packet_queue_write_index = 0;
    }

    return NRF_SUCCESS;
}
