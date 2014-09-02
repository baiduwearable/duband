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

#include "bootloader.h"
#include "bootloader_types.h"
#include "bootloader_util.h"
#include "dfu.h"
#include "dfu_transport.h"
#include "nrf51.h"
#include "app_error.h"
#include "nrf_sdm.h"
#include "ble_flash.h"
#include "nordic_common.h"

#ifdef DEBUG_LOG
    #include "simple_uart.h"
#endif 

#define IRQ_ENABLED             0x01           /**< Field identifying if an interrupt is enabled. */
#define MAX_NUMBER_INTERRUPTS   32             /**< Maximum number of interrupts available. */


bool bootloader_app_is_valid(uint32_t app_addr)
{
    const bootloader_settings_t * p_bootloader_settings;

    // There exists an application in CODE region 1.
    if (DFU_BANK_0_REGION_START == EMPTY_FLASH_MASK)
    {
        return false;
    }
    
    bool success = false;
    
    switch (app_addr)
    {
        case DFU_BANK_0_REGION_START:
            bootloader_util_settings_get(&p_bootloader_settings);

            // The application in CODE region 1 is flagged as valid during update.
            if (p_bootloader_settings->bank_0 == BANK_VALID_APP)
            {
                success = true;
            }
            break;
            
        case DFU_BANK_1_REGION_START:
        default:
            break;
    }

    return success;
}


static void bootloader_settings_save(bootloader_settings_t settings)
{
    const bootloader_settings_t * p_bootloader_settings;

    bootloader_util_settings_get(&p_bootloader_settings);

    // Save needed data for later use. 
    uint32_t settings_page = ((uint32_t) p_bootloader_settings) / NRF_FICR->CODEPAGESIZE;

    uint32_t num_of_words = sizeof(bootloader_settings_t) / sizeof(uint32_t);

    // Save bootloader settings.
    uint32_t err_code = ble_flash_page_erase(settings_page);
    APP_ERROR_CHECK(err_code);    
    err_code          = ble_flash_block_write((uint32_t *)p_bootloader_settings, 
                                              (uint32_t *)&settings, 
                                              num_of_words);
    APP_ERROR_CHECK(err_code);
}


void bootloader_dfu_update_process(dfu_update_status_t update_status)
{
    const bootloader_settings_t * p_bootloader_settings;
    bootloader_util_settings_get(&p_bootloader_settings);

    if (update_status.status_code == DFU_UPDATE_COMPLETE)
    {
        bootloader_settings_t settings;
        
        settings.bank_0_crc  = update_status.app_crc;
        settings.bank_0_size = update_status.app_size;
        settings.bank_0      = BANK_VALID_APP;
        settings.bank_1      = BANK_INVALID_APP;
        
        bootloader_settings_save(settings);
    }
    else if (update_status.status_code == DFU_TIMEOUT)
    {
        // Timeout has occurred. Close the connection with the DFU Controller.
        uint32_t err_code = dfu_transport_close();
        APP_ERROR_CHECK(err_code);
    }
    else if (update_status.status_code == DFU_BANK_0_ERASED)
    {
        bootloader_settings_t settings;
        
        settings.bank_0_crc  = 0;
        settings.bank_0_size = 0;
        settings.bank_0      = BANK_ERASED;
        settings.bank_1      = p_bootloader_settings->bank_1;
        
        bootloader_settings_save(settings);
    }
    else if (update_status.status_code == DFU_BANK_1_ERASED)
    {
        bootloader_settings_t settings;
        
        settings.bank_0      = p_bootloader_settings->bank_0;
        settings.bank_0_crc  = p_bootloader_settings->bank_0_crc;
        settings.bank_0_size = p_bootloader_settings->bank_0_size;
        settings.bank_1      = BANK_ERASED;
        
        bootloader_settings_save(settings);
    }
    else
    {
        // No implementation needed.
    }
}


uint32_t bootloader_dfu_start(void)
{
    uint32_t err_code;
    
    // Clear swap if banked update is used.    
    err_code = dfu_init(); 
    if (err_code == NRF_SUCCESS)    
    {
        err_code = dfu_transport_update_start();
    }
        
    return err_code;
}


/**@brief Function for disabling all interrupts before jumping from bootloader to application.
 */
static void interrupts_disable(void)
{
    uint32_t interrupt_setting_mask;
    uint8_t  irq;

    // We start the loop from first interrupt, i.e. interrupt 0.
    irq                    = 0;
    // Fetch the current interrupt settings.
    interrupt_setting_mask = NVIC->ISER[0];
    
    for (; irq < MAX_NUMBER_INTERRUPTS; irq++)
    {
        if (interrupt_setting_mask & (IRQ_ENABLED << irq))
        {
            // The interrupt was enabled, and hence disable it.
            NVIC_DisableIRQ((IRQn_Type) irq);
        }
    }        
}


void bootloader_app_start(uint32_t app_addr)
{
    // If the applications CRC has been checked and passed, the magic number will be written and we
    // can start the application safely.
    interrupts_disable();

    uint32_t err_code = sd_softdevice_forward_to_application();
    #ifdef DEBUG_LOG
    if(err_code == NRF_SUCCESS) {
        simple_uart_putstring((const uint8_t *)"sd_softdevice_forward_to_application success \r\n");
    } else {
        simple_uart_putstring((const uint8_t *)"sd_softdevice_forward_to_application fail \r\n");
    }
    #endif 
    APP_ERROR_CHECK(err_code);

    bootloader_util_app_start(CODE_REGION_1_START);
}


void bootloader_settings_get(bootloader_settings_t * const p_settings)
{
    const bootloader_settings_t * p_bootloader_settings;
    bootloader_util_settings_get(&p_bootloader_settings);

    p_settings->bank_0      = p_bootloader_settings->bank_0;
    p_settings->bank_0_crc  = p_bootloader_settings->bank_0_crc;
    p_settings->bank_0_size = p_bootloader_settings->bank_0_size;
    p_settings->bank_1      = p_bootloader_settings->bank_1;
}
