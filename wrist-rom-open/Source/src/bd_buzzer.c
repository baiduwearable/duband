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

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf51_bitfields.h"
#include "nrf_soc.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "bd_buzzer.h"
#include "app_util.h"

#include "board_config_pins.h"

#define PPI_CHAN0_TO_TOGGLE_BUZZER              0                                         /*!< The PPI Channel that connects CC0 compare event to the GPIOTE Task that toggles the Advertising LED. */
#define GPIOTE_CHAN_FOR_BUZZER_TASK             0                                         /*!< The GPIOTE Channel used to perform write operation on the Advertising LED pin. */
#define TIMER_PRESCALER                      4                                         /*!< Prescaler setting for timer. */
#define CAPTURE_COMPARE_0_VALUE              1666 //0x1E84                                    /*!< Capture compare value that corresponds to 250 ms. */

#define ADVERTISING_BUZZER_PIN_NO         BAIDU_LED_4//NRF6310_LED_6     /**< Pin that can be used by applications to indicate advertising state.*/


/////////////////////////////////////////////////////////////////////
#define LED_INTENSITY_HIGH    (224U) /*! change intensity value for light intensity*/
#define LED_INTENSITY_LOW     (32U) /*! change intensity value for light intensity*/
#define LED_OFF               (1U) /*!< Led off */
#define LED_INTENSITY_HALF    (127U) /*!< Half intensity. Used to calculate timer parameters. */

/* @brief Timer 1 initialisation function.
 *
 * @details This function will initialise Timer 1 peripheral. This timer is used only to
 *          generate capture compare events that toggle the advertising LED state.
 */
static void timer1_init(void)
{
    // Configure timer
    NRF_TIMER1->MODE      = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->BITMODE   = TIMER_BITMODE_BITMODE_08Bit;
    NRF_TIMER1->PRESCALER = TIMER_PRESCALER;

    /* Load initial values to TIMER2 CC registers */
    /* Set initial  CC0 value to anything >1 */
    NRF_TIMER1->CC[0] = LED_INTENSITY_LOW;
    NRF_TIMER1->CC[1] = (LED_INTENSITY_HALF*2);

    /* Create an Event-Task shortcut to clear TIMER2 on COMPARE[1] event. */
    NRF_TIMER1->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos);

}


/* @brief PPI initialisation function.
 *
 * @details This function will initialise Programmable Peripheral Interconnect peripheral. It will
 *          configure the PPI channels as follows -
 *              PPI Channel 0 - Connecting CC0 Compare event to GPIOTE Task to toggle the LED state
 *          This configuration will feed a PWM input to the LED thereby making it flash in an
 *          interval that is dependent on the TIMER configuration.
 */
static void ppi_init(void)
{
    /* Configure PPI channel 0 to toggle PWM_OUTPUT_PIN on every TIMER2 COMPARE[0] match */
    sd_ppi_channel_assign(0, &(NRF_TIMER1->EVENTS_COMPARE[0]), &(NRF_GPIOTE->TASKS_OUT[0]));

    /* Configure PPI channel 1 to toggle PWM_OUTPUT_PIN on every TIMER2 COMPARE[1] match */
    sd_ppi_channel_assign(1,&(NRF_TIMER1->EVENTS_COMPARE[1]),&(NRF_GPIOTE->TASKS_OUT[0]));

    /* Enable only PPI channels 0 and 1 */
    sd_ppi_channel_enable_set(PPI_CHEN_CH0_Msk | PPI_CHEN_CH1_Msk);
}

static void gpiote_init(void)
{
    // Configure the GPIOTE Task to toggle the LED state.
    nrf_gpiote_task_config(GPIOTE_CHAN_FOR_BUZZER_TASK,
                           ADVERTISING_BUZZER_PIN_NO,
                           NRF_GPIOTE_POLARITY_TOGGLE,
                           NRF_GPIOTE_INITIAL_VALUE_HIGH);
}

void buzzer_start(void)
{
    ppi_init();
    timer1_init();
    gpiote_init();

    NRF_TIMER1->TASKS_START = 1;
}


void buzzer_stop(void)
{
    // Disable the GPIOTE_CHAN_FOR_buzzer_TASK. This is because when an task has been configured
    // to operate on a pin, the pin can only be written from GPIOTE module. Attempting to write a
    // pin (using nrf_gpio_pin_clear() below for example) as a normal GPIO pin will have no effect.
    NRF_GPIOTE->CONFIG[GPIOTE_CHAN_FOR_BUZZER_TASK] =
        (GPIOTE_CONFIG_MODE_Disabled << GPIOTE_CONFIG_MODE_Pos);

    NRF_TIMER1->TASKS_STOP = 1;

    nrf_gpio_pin_set(ADVERTISING_BUZZER_PIN_NO);
}



/**
 * @}
 */
