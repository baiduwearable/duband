/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <nrf51.h>
#include "ble_error_log.h"
#include "app_util.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "ble_flash.h"


// Made static to avoid the error_log to go on the stack.
static ble_error_log_data_t   m_ble_error_log;            /**< . */
static volatile uint8_t       m_ble_log_clear_flag = 0;   /**< Volatile flag to be used for clearing the flash in debug mode. */
//lint -esym(526,__Vectors)
extern uint32_t             * __Vectors;                  /**< The initialization vector holds the address to __initial_sp that will be used when fetching the stack. */ 


static void fetch_stack(ble_error_log_data_t * error_log)
{
    uint32_t * p_stack;
    uint32_t * initial_sp;
    uint32_t   length;
  
    initial_sp = (uint32_t *) __Vectors;
    p_stack    = (uint32_t *) __current_sp();
  
    length = ((uint32_t) initial_sp) - ((uint32_t) p_stack);
    memcpy(error_log->stack_info, 
           p_stack, 
           (length > STACK_DUMP_LENGTH) ? STACK_DUMP_LENGTH : length);
}


uint32_t ble_error_log_write(uint32_t err_code, const uint8_t * p_message, uint16_t line_number)
{
    uint8_t  error_log_size;

    error_log_size              = CEIL_DIV(sizeof(ble_error_log_data_t), sizeof(uint32_t));
    m_ble_error_log.failure     = true;
    m_ble_error_log.err_code    = err_code;
    m_ble_error_log.line_number = line_number;

    strncpy((char *)m_ble_error_log.message, (const char *)p_message, ERROR_MESSAGE_LENGTH - 1);
    m_ble_error_log.message[ERROR_MESSAGE_LENGTH - 1] = '\0';

    fetch_stack(&m_ble_error_log);

    return ble_flash_page_write(FLASH_PAGE_ERROR_LOG,
                                (uint32_t *) &m_ble_error_log,
                                error_log_size);
}


uint32_t ble_error_log_read(ble_error_log_data_t * error_log)
{
    uint8_t  error_log_size = CEIL_DIV(sizeof(ble_error_log_data_t), sizeof(uint32_t));
    uint32_t err_code       = NRF_SUCCESS;
    
    err_code = ble_flash_page_read(FLASH_PAGE_ERROR_LOG, (uint32_t *) error_log, &error_log_size);
    
    // If nothing is in flash; then return NRF_SUCCESS.
    if (err_code == NRF_ERROR_NOT_FOUND)
    {
        return NRF_SUCCESS;
    }

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (!error_log->failure)
    {
        return NRF_SUCCESS;
    }

    nrf_gpio_pin_set(LOG_LED_PIN_NO); // Notify that a message exists in the log.

    while (error_log->failure && !m_ble_log_clear_flag)
    {
        // Put breakpoint, and read data, then log->failure=false; to continue in debug mode.
        // In application, define how to clear the error log, 
        // e.g. read button 6, if pressed, then clear log and continue.
    }
    
    nrf_gpio_pin_clear(LOG_LED_PIN_NO);
    err_code = ble_flash_page_erase(FLASH_PAGE_ERROR_LOG);

    return err_code;
}


void ble_error_log_clear(void)
{
    m_ble_log_clear_flag = 1;
}


void ble_error_log_init(void)
{
    // Variable made volatile in order to avoid optimization.
    // If an assert has been seen, and the old stack must be fetched, 
    // then find the assembly entry of this function in the map file 
    // and set 'read_error_log = true' in the debugger to break execution.
    volatile bool read_error_log = false;
    uint32_t      err_code;

    if (read_error_log)
    {
        err_code = ble_error_log_read(&m_ble_error_log);
        APP_ERROR_CHECK_BOOL(err_code == NRF_SUCCESS || err_code == NRF_ERROR_NOT_FOUND);
    }
}
