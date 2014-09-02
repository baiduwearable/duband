/* Copyright (c) [2014 Baidu]. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File Name          : 
 * Author             : 
 * Version            : $Revision:$
 * Date               : $Date:$
 * Description        : 
 *                      
 * HISTORY:
 * Date               | Modification                    | Author
 * 28/03/2014         | Initial Revision                | 
 
 */
#include <string.h>
#include "nordic_common.h"
#include "app_util.h"
#include "app_gpiote.h"
#include "app_error.h"
#include "bd_interrupt_notify.h"


static io_int_event_cfg_t *           m_io_evt_container = NULL;           /**< pin configuration. */
static uint8_t                        m_io_evt_count;              /**< Number of configured buttons. */
static uint32_t                       m_active_low_states_mask;    /**< Mask representing buttons which are active low. */
static uint32_t                       m_active_high_states_mask;   /**< Mask representing buttons which are active high. */
static app_gpiote_user_id_t           m_gpiote_user_id;            /**< GPIOTE user ID for  */



/**@brief GPIOTE event handler.
 *
 * @details Saves the current status of the button pins, and starts a timer. If the timer is already
 *          running, it will be restarted.
 *
 * @param[in]  event_pins_low_to_high   Mask telling which pin(s) had a low to high transition.
 * @param[in]  event_pins_high_to_low   Mask telling which pin(s) had a high to low transition.
 */
static void gpiote_event_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{
    uint32_t event_pins_mask;

    event_pins_mask = event_pins_low_to_high | event_pins_high_to_low;

    if(event_pins_mask != 0) {
        uint8_t i;

        for(i =0;i< m_io_evt_count; ++i) {

            io_int_event_cfg_t * p_evt = &m_io_evt_container[i];

            if(((event_pins_mask & (1 << p_evt->pin_no)) != 0) &&(p_evt->callback)) {

                p_evt->callback(p_evt->pin_no);

            }

        }
    }

}


/* register a callback for a special interrupt */
uint32_t interrupt_notify_register( io_int_event_cfg_t *      p_io_pins,
                                    uint8_t                 pin_count)
{
    uint32_t err_code;

    // Save configuration.
    m_io_evt_container  = p_io_pins;
    m_io_evt_count      = pin_count;


    // Configure pins.
    m_active_high_states_mask = 0;
    m_active_low_states_mask  = 0;

    while (pin_count--) {
        io_int_event_cfg_t * p_btn = &p_io_pins[pin_count];

        // Configure pin.
        nrf_gpio_cfg_input(p_btn->pin_no, p_btn->pull_cfg);

        // Build GPIOTE user registration masks.
        m_active_high_states_mask |= ((p_btn->active_high ? 1 : 0) << p_btn->pin_no);
        m_active_low_states_mask  |= ((p_btn->active_high ? 0 : 1) << p_btn->pin_no);
    }

    // Register button module as a GPIOTE user.
    err_code = app_gpiote_user_register(&m_gpiote_user_id,
                                        m_active_high_states_mask,
                                        m_active_low_states_mask,
                                        gpiote_event_handler);

    return err_code;
}


uint32_t app_io_evt_register_enable(void)
{
    if (m_io_evt_container == NULL) {
        return NRF_ERROR_INVALID_STATE;
    }

    return app_gpiote_user_enable(m_gpiote_user_id);
}


uint32_t app_io_evt_register_disable(void)
{

    if (m_io_evt_container == NULL) {
        return NRF_ERROR_INVALID_STATE;
    }

    return app_gpiote_user_disable(m_gpiote_user_id);
}
