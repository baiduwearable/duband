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

#include "board_config_pins.h"

#ifdef FEATURE_TWI

#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "twi_master.h"
#include "spi_master_config.h" // This file must be in the application folder

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_assert.h"
#include "nrf_soc.h"
#include "nrf_error.h"
/* Max cycles approximately to wait on RXDREADY and TXDREADY event, this is optimum way instead of using timers, this is not power aware, negetive side is this is not power aware */
#define MAX_TIMEOUT_LOOPS             (10000UL)        /*!< MAX while loops to wait for RXD/TXD event */

static bool twi_master_write(uint8_t *data, uint8_t data_length, bool issue_stop_condition)
{
    uint32_t timeout = MAX_TIMEOUT_LOOPS;   /* max loops to wait for EVENTS_TXDSENT event*/

    if (data_length == 0) {
        /* gently return false for requesting data of size 0 */
        return false;
    }

    NRF_TWI1->TXD = *data++;
    NRF_TWI1->TASKS_STARTTX = 1;

    while (true) {
        while(NRF_TWI1->EVENTS_TXDSENT == 0 && (--timeout)) {}

        if (timeout == 0) {
            /* timeout before receiving event*/
            return false;
        }

        NRF_TWI1->EVENTS_TXDSENT = 0;
        if (--data_length == 0) {
            break;
        }

        NRF_TWI1->TXD = *data++;
    }

    if (issue_stop_condition) {
        NRF_TWI1->EVENTS_STOPPED = 0;
        NRF_TWI1->TASKS_STOP = 1;
        /* wait until stop sequence is sent and clear the EVENTS_STOPPED */
        while(NRF_TWI1->EVENTS_STOPPED == 0) {}
    }

    return true;
}

static bool twi_master_read(uint8_t *data, uint8_t data_length, bool issue_stop_condition)
{
    uint32_t timeout = MAX_TIMEOUT_LOOPS;   /* max loops to wait for RXDREADY event*/

    if(data_length == 0) {
        /* gently return false for requesting data of size 0 */
        return false;
    }


    if (data_length == 1) {
        //NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TWI1->TASKS_STOP;

        sd_ppi_channel_assign(0,
                              &(NRF_TWI1->EVENTS_BB),
                              &(NRF_TWI1->TASKS_STOP));
    } else {
        //NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TWI1->TASKS_SUSPEND;
        sd_ppi_channel_assign(0,
                              &(NRF_TWI1->EVENTS_BB),
                              &(NRF_TWI1->TASKS_SUSPEND));

    }
    // NRF_PPI->CHENSET = PPI_CHENSET_CH0_Msk;
    sd_ppi_channel_enable_set(PPI_CHEN_CH0_Msk);
    NRF_TWI1->TASKS_STARTRX = 1;
    while(true) {
        while((NRF_TWI1->EVENTS_RXDREADY == 0) && (--timeout)) {  //nrf_app_event_wait();
        }

        if(timeout == 0) {
            /* timeout before receiving event*/
            return false;
        }

        NRF_TWI1->EVENTS_RXDREADY = 0;
        *data++ = NRF_TWI1->RXD;

        /* configure PPI to stop TWI master before we get last BB event */
        if (--data_length == 1) {
            //  NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TWI1->TASKS_STOP;
            sd_ppi_channel_assign(0,
                                  &(NRF_TWI1->EVENTS_BB),
                                  &(NRF_TWI1->TASKS_STOP));
        }

        if (data_length == 0)
            break;

        NRF_TWI1->TASKS_RESUME = 1;
    }

    /* wait until stop sequence is sent and clear the EVENTS_STOPPED */
    while(NRF_TWI1->EVENTS_STOPPED == 0) {  //nrf_app_event_wait();
    }
    NRF_TWI1->EVENTS_STOPPED = 0;
    sd_ppi_channel_enable_clr(PPI_CHEN_CH0_Msk);

    //  NRF_PPI->CHENCLR = PPI_CHENCLR_CH0_Msk;

    return true;
}

/**
 * Detects stuck slaves (SDA = 0 and SCL = 1) and tries to clear the bus.
 *
 * @return
 * @retval false Bus is stuck.
 * @retval true Bus is clear.
 */
static bool twi_master_clear_bus(void)
{
    bool bus_clear;

    TWI_SDA_HIGH();
    TWI_SCL_HIGH();
    TWI_DELAY();

    if (TWI_SDA_READ() == 1 && TWI_SCL_READ() == 1) {
        bus_clear = true;
    } else {
        uint_fast8_t i;
        bus_clear = false;

        // Clock max 18 pulses worst case scenario(9 for master to send the rest of command and 9 for slave to respond) to SCL line and wait for SDA come high
        for (i=18; i--;) {
            TWI_SCL_LOW();
            TWI_DELAY();
            TWI_SCL_HIGH();
            TWI_DELAY();

            if (TWI_SDA_READ() == 1) {
                bus_clear = true;
                break;
            }
        }
    }

    return bus_clear;
}

bool twi_master_init(void)
{
    /* To secure correct signal levels on the pins used by the TWI
       master when the system is in OFF mode, and when the TWI master is 
       disabled, these pins must be configured in the GPIO peripheral.
    */
    uint32_t err_code=0;
    NRF_GPIO->PIN_CNF[TWI_MASTER_CONFIG_CLOCK_PIN_NUMBER] =
        (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
        | (GPIO_PIN_CNF_DRIVE_S0D1     << GPIO_PIN_CNF_DRIVE_Pos)
        | (GPIO_PIN_CNF_PULL_Pullup    << GPIO_PIN_CNF_PULL_Pos)
        | (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos)
        | (GPIO_PIN_CNF_DIR_Input      << GPIO_PIN_CNF_DIR_Pos);

    NRF_GPIO->PIN_CNF[TWI_MASTER_CONFIG_DATA_PIN_NUMBER] =
        (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
        | (GPIO_PIN_CNF_DRIVE_S0D1     << GPIO_PIN_CNF_DRIVE_Pos)
        | (GPIO_PIN_CNF_PULL_Pullup    << GPIO_PIN_CNF_PULL_Pos)
        | (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos)
        | (GPIO_PIN_CNF_DIR_Input      << GPIO_PIN_CNF_DIR_Pos);

    NRF_TWI1->EVENTS_RXDREADY = 0;
    NRF_TWI1->EVENTS_TXDSENT = 0;
    NRF_TWI1->PSELSCL = TWI_MASTER_CONFIG_CLOCK_PIN_NUMBER;
    NRF_TWI1->PSELSDA = TWI_MASTER_CONFIG_DATA_PIN_NUMBER;
    NRF_TWI1->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K100 << TWI_FREQUENCY_FREQUENCY_Pos;
    err_code = sd_ppi_channel_assign(0,
                                     &(NRF_TWI1->EVENTS_BB),
                                     &(NRF_TWI1->TASKS_SUSPEND));
    ASSERT(err_code == NRF_SUCCESS);

    err_code = sd_ppi_channel_enable_clr(PPI_CHEN_CH0_Msk);
    ASSERT(err_code == NRF_SUCCESS);

    /*
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TWI1->EVENTS_BB;
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TWI1->TASKS_SUSPEND;
    NRF_PPI->CHENCLR = PPI_CHENCLR_CH0_Msk;*/
    NRF_TWI1->ENABLE = TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos;

    return twi_master_clear_bus();
}

bool twi_master_transfer(uint8_t address, uint8_t *data, uint8_t data_length, bool issue_stop_condition)
{
    bool transfer_succeeded = true;
    if (data_length > 0 && twi_master_clear_bus()) {
        NRF_TWI1->ADDRESS = (address >> 1);

        if ((address & TWI_READ_BIT) != 0) {
            transfer_succeeded = twi_master_read(data, data_length, issue_stop_condition);
        } else {
            transfer_succeeded = twi_master_write(data, data_length, issue_stop_condition);
        }
    }
    return transfer_succeeded;
}
#endif //FEATURE_TWI
/*lint --flb "Leave library region" */
