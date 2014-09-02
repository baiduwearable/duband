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
 
#include <stdint.h>
#include <stddef.h> 
#include "app_util.h"
#include "app_error.h"
#include "bootloader.h"
#include "bootloader_types.h"
#include "ble_flash.h"
#include "crc16.h"
#include "dfu.h"
#include "dfu_types.h"
#include "nrf.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"

/**@brief States of the DFU state machine. */
typedef enum
{
    DFU_STATE_INIT_ERROR,                                                                           /**< State for: dfu_init(...) error. */
    DFU_STATE_IDLE,                                                                                 /**< State for: idle. */
    DFU_STATE_RDY,                                                                                  /**< State for: ready. */
    DFU_STATE_RX_INIT_PKT,                                                                          /**< State for: receiving initialization packet. */
    DFU_STATE_RX_DATA_PKT,                                                                          /**< State for: receiving data packet. */
    DFU_STATE_VALIDATE,                                                                             /**< State for: validate. */
    DFU_STATE_WAIT_4_ACTIVATE                                                                       /**< State for: waiting for dfu_image_activate(). */
} dfu_state_t;

static dfu_state_t m_dfu_state;                                                                     /**< Current DFU state. */
static uint32_t    m_image_size;                                                                    /**< Size of the image that will be transmitted. */
static uint32_t    m_init_packet[16];                                                               /**< Init packet, can hold CRC, Hash, Signed Hash and similar, for image validation, integrety check and authorization checking. */
static uint8_t     m_init_packet_length;                                                            /**< Length of init packet received. */
static uint16_t    m_image_crc;                                                                     /**< Calculated CRC of the image received. */
static uint32_t *  mp_app_write_address;                                                            /**< Pointer to the address in flash to write next word of data received. */
static uint32_t    m_erase_page_index;                                                              /**< Index on next page to erase during image transfer. */

#define IMAGE_WRITE_IN_PROGRESS() (mp_app_write_address > (uint32_t *)DFU_BANK_0_REGION_START)      /**< Macro for determining is image write in progress. */


uint32_t dfu_init(void)
{        
    uint32_t flash_page_size = NRF_FICR->CODEPAGESIZE;

    m_dfu_state          = DFU_STATE_IDLE;    
    m_init_packet_length = 0;
    m_image_crc          = 0;    
    m_erase_page_index   = DFU_BANK_0_REGION_START / flash_page_size;

    return NRF_SUCCESS;
}


/**@brief Function for preparing the next page in Single Bank DFU.
 *
 * @return NRF_SUCCESS on success, an error_code otherwise. 
 */
static uint32_t next_flash_page_prepare(void)
{
    uint32_t flash_page_size = NRF_FICR->CODEPAGESIZE;
    uint32_t last_page_index = BOOTLOADER_REGION_START / flash_page_size;

    if (m_erase_page_index < last_page_index)
    {
        return ble_flash_page_erase(m_erase_page_index++);
    }
    
    return NRF_SUCCESS;
}


uint32_t dfu_image_size_set(uint32_t image_size)
{
    uint32_t * p_bank_start_address = (uint32_t *)DFU_BANK_0_REGION_START; 
    
    uint32_t            err_code;
    dfu_update_status_t update_status;
    
    err_code = NRF_ERROR_INVALID_STATE;
    
    if (image_size > DFU_IMAGE_MAX_SIZE_FULL)
    {
        return NRF_ERROR_DATA_SIZE;
    }   
    
    if ((image_size & (sizeof(uint32_t) - 1)) != 0)
    {
        // Image_size is not a multiple of 4 (word size).
        return NRF_ERROR_NOT_SUPPORTED;
    }    
    
    switch (m_dfu_state)
    {
        case DFU_STATE_IDLE:
            // Start preparing first page, to be able to receive the new image.
            err_code = next_flash_page_prepare();
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }

            mp_app_write_address      = p_bank_start_address;
            m_image_size              = image_size;
            m_dfu_state               = DFU_STATE_RDY;
            update_status.status_code = DFU_BANK_0_ERASED;
            
            bootloader_dfu_update_process(update_status);                    
            break;
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;    
    }
    
    return err_code;
}


uint32_t dfu_data_pkt_handle(dfu_update_packet_t * p_packet)
{
    uint32_t   err_code;
    uint32_t   data_length;
    uint32_t * p_data;
    
    if (p_packet == NULL)
    {
        return NRF_ERROR_NULL;
    }

    // Check pointer alignment.
    if (!is_word_aligned(p_packet->p_data_packet))
    {
        // The p_data_packet is not word aligned address.
        return NRF_ERROR_INVALID_ADDR;
    }

    switch (m_dfu_state)
    {
        case DFU_STATE_RDY:
            // Fall-through.
        
        case DFU_STATE_RX_INIT_PKT:
            m_dfu_state = DFU_STATE_RX_DATA_PKT;
        
            // Fall-through.
        
        case DFU_STATE_RX_DATA_PKT:
            data_length = p_packet->packet_length;
            if (((uint32_t)(mp_app_write_address + data_length) - DFU_BANK_0_REGION_START) > 
                m_image_size)
            {
                // The caller is trying to write more bytes into the flash than the size provided to 
                // the dfu_image_size_set function.
                return NRF_ERROR_DATA_SIZE;
            }

            p_data   = (uint32_t *)p_packet->p_data_packet;            
            err_code = ble_flash_block_write(mp_app_write_address, p_data, p_packet->packet_length);            
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }
            mp_app_write_address += data_length;        
            
            err_code = next_flash_page_prepare();
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }

            break;
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;
    }
    
    return err_code;
}


uint32_t dfu_init_pkt_handle(dfu_update_packet_t * p_packet)
{
    uint32_t err_code;
    uint32_t i;
    
    switch (m_dfu_state)
    {
        case DFU_STATE_RDY:
            m_dfu_state = DFU_STATE_RX_INIT_PKT;
            // Fall-through.     
            
        case DFU_STATE_RX_INIT_PKT:
            // DFU initialization has been done and a start packet has been received.
            if (IMAGE_WRITE_IN_PROGRESS())
            {
                // Image write is already in progress. Cannot handle an init packet now.
                return NRF_ERROR_INVALID_STATE;
            }

            for (i = 0; i < p_packet->packet_length; i++)
            {
                m_init_packet[m_init_packet_length++] = p_packet->p_data_packet[i];
            }
            
            err_code = NRF_SUCCESS;
            break;

        default:
            // Either the start packet was not received or dfu_init function was not called before.
            err_code = NRF_ERROR_INVALID_STATE;        
            break;
    }
    
    return err_code;    
}


uint32_t dfu_image_validate()
{
    uint32_t err_code;
    uint16_t received_crc;    

    switch (m_dfu_state)
    {
        case DFU_STATE_RX_DATA_PKT:
            m_dfu_state = DFU_STATE_VALIDATE;
            
            // Check if the application image write has finished.
            if (((uint32_t)mp_app_write_address - DFU_BANK_0_REGION_START) != m_image_size)
            {
                // Image not yet fully transferred by the peer.
                err_code = NRF_ERROR_INVALID_STATE;
            }
            else
            {
                // Calculate CRC from DFU_BANK_1_REGION_START to mp_app_write_address.
                m_image_crc  = crc16_compute((uint8_t*)DFU_BANK_0_REGION_START, 
                                             m_image_size, 
                                             NULL);
                received_crc = uint16_decode((uint8_t*)&m_init_packet[0]);
                    
                if ((m_init_packet_length != 0) && (m_image_crc != received_crc))
                {
                    return NRF_ERROR_INVALID_DATA;
                }
                m_dfu_state = DFU_STATE_WAIT_4_ACTIVATE;
                err_code    = NRF_SUCCESS;
            }

            break;            
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;  
    }
    
    return err_code;    
}


uint32_t dfu_image_activate()
{
    uint32_t            err_code;
    dfu_update_status_t update_status;
    
    switch (m_dfu_state)
    {    
        case DFU_STATE_WAIT_4_ACTIVATE:            
            update_status.status_code = DFU_UPDATE_COMPLETE;
            update_status.app_crc     = 0xFEED;
            update_status.app_size    = m_image_size;

            bootloader_dfu_update_process(update_status);        
            err_code = NRF_SUCCESS;
            break;
            
        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;
    }
    
    return err_code;
}
