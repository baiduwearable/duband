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

/**@file
 *
 * @defgroup ble_sdk_app_bootloader_main main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file.
 *
 * -# Receive start data package. 
 * -# Based on start packet, prepare NVM area to store received data. 
 * -# Receive data packet. 
 * -# Validate data packet.
 * -# Write Data packet to NVM.
 * -# If not finished - Wait for next packet.
 * -# Receive stop data packet.
 * -# Activate Image, boot application.
 *
 */
#include "dfu.h"
#include "bootloader.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "nrf51.h"
#include "ble_hci.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "nrf_error.h"
#include "boards.h"
#include "ble_debug_assert_handler.h"

//uart debug information
#ifdef DEBUG_LOG
    #include "simple_uart.h"
#endif

#define APP_GPIOTE_MAX_USERS            2                                                       /**< Number of GPIOTE users in total. Used by button module and dfu_transport_serial module (flow control). */

#define APP_TIMER_PRESCALER             0                                                       /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS            3                                                       /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                                       /**< Size of timer operation queues. */


/**@brief Function for error handling, which is called when an error has occurred. 
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name. 
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    nrf_gpio_pin_set(LED4);
    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover on reset.
    NVIC_SystemReset();
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] file_name   File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for initialization of LEDs.
 *
 * @details Initializes all LEDs used by the application.
 */
static void leds_init(void)
{
    GPIO_LED_CONFIG(LED0);
    GPIO_LED_CONFIG(LED1);
    GPIO_LED_CONFIG(LED4);
}


/**@brief Function for clearing the LEDs.
 *
 * @details Clears all LEDs used by the application.
 */
static void leds_off(void)
{
    nrf_gpio_pin_set(LED0);
    nrf_gpio_pin_set(LED1);
    nrf_gpio_pin_set(LED4);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, true);
}



void wdt_stop(void)
{
    NRF_WDT->TASKS_START = 0;
}

/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    
#ifdef DEBUG_LOG
    simple_uart_config(RTS_PIN_NUMBER, TX_PIN_NUMBER, CTS_PIN_NUMBER, RX_PIN_NUMBER, HWFC);
#endif
    
    wdt_stop(); //stop wdt whether it started or not   
    leds_init();

    // This check ensures that the defined fields in the bootloader corresponds with actual
    // setting in the nRF51 chip.
    APP_ERROR_CHECK_BOOL(NRF_UICR->CLENR0 == CODE_REGION_1_START);

    APP_ERROR_CHECK_BOOL(*((uint32_t *)NRF_UICR_BOOT_START_ADDRESS) == BOOTLOADER_REGION_START);
    APP_ERROR_CHECK_BOOL(NRF_FICR->CODEPAGESIZE == CODE_PAGE_SIZE);

    // Initialize.
    timers_init();
    
#ifdef DEBUG_LOG
    simple_uart_putstring((const uint8_t *)"bootlader start\r\n");
    uint8_t buffer[10];
#endif

    uint8_t should_enter_bootloader =  NRF_POWER->GPREGRET & 0xff;
    uint16_t wdt_reset = (NRF_POWER->RESETREAS) & (0xFFFF);
    
#ifdef DEBUG_LOG
    simple_uart_putstring((const uint8_t *)"should_enter_bootloader is : ");
    sprintf((char *)buffer,"%d",should_enter_bootloader);
    simple_uart_putstring((const uint8_t *)buffer);
    simple_uart_putstring((const uint8_t *)"\r\n");
    
    if(wdt_reset & 0x0002) {
        simple_uart_putstring((const uint8_t *)"wdt reset will goto dfu\r\n ");
    }
#endif
    if (should_enter_bootloader || (wdt_reset & 0x02))
    {
        nrf_gpio_pin_set(LED1);
        
#ifdef DEBUG_LOG
    simple_uart_putstring((const uint8_t *)"will enter dfu bootlaoder\r\n");
#endif
        
        // Initiate an update of the firmware.
        err_code = bootloader_dfu_start();
        APP_ERROR_CHECK(err_code);

        nrf_gpio_pin_clear(LED1);
    }

    //exit from DFU unit so just Start application
    {
#ifdef DEBUG_LOG
    simple_uart_putstring((const uint8_t *)"will lunch applicaiton \r\n");
#endif
        leds_off();
        
        //set retain register
        NRF_POWER->GPREGRET = 0x00;
        //clear AIRCR register
        NRF_POWER->RESETREAS = 0x04;

        if(wdt_reset & 0x02) {
           NRF_POWER->RESETREAS = 0x02; 
        }

        // Select a bank region to use as application region.
        // @note: Only applications running from DFU_BANK_0_REGION_START is supported.
        bootloader_app_start(DFU_BANK_0_REGION_START);
        
    }
    
#ifdef DEBUG_LOG
    simple_uart_putstring((const uint8_t *)"can not start applicaiton so reenter OTA \r\n");
#endif
    leds_off();
    
    //comes here should reenter DFU
    NRF_POWER->GPREGRET = 0x01;
    
    NVIC_SystemReset();
}
