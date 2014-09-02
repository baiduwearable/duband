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

#include <bd_spi_master.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "common.h"
#include "spi_master_config.h" // This file must be in the application folder
#define SPI_BUFSIZE 8
volatile uint8_t   SPIMasterBuffer[SPI_BUFSIZE];
volatile uint8_t   SPISlaveBuffer[SPI_BUFSIZE];
volatile uint8_t  SPIReadLength, SPIWriteLength;
uint32_t SEL_SS_PINOUT;

uint32_t* spi_master_init(SPIModuleNumber module_number, SPIMode mode, bool lsb_first)
{
    uint32_t config_mode;

    NRF_SPI_Type *spi_base_address = (SPI0 == module_number)? NRF_SPI0 : (NRF_SPI_Type *)NRF_SPI1;

    if(SPI0 == module_number) {
        /* Configure pins, frequency and mode */
        spi_base_address->PSELSCK  = SPI_PSELSCK0;
        spi_base_address->PSELMOSI = SPI_PSELMOSI0;
        spi_base_address->PSELMISO = SPI_PSELMISO0;
        nrf_gpio_pin_set(SPI_PSELSS0); /* disable Set slave select (inactive high) */
        SEL_SS_PINOUT = SPI_PSELSS0;
    }
#ifdef SPI1
    else {
        /* Configure pins, frequency and mode */
        spi_base_address->PSELSCK  = SPI_PSELSCK1;
        spi_base_address->PSELMOSI = SPI_PSELMOSI1;
        spi_base_address->PSELMISO = SPI_PSELMISO1;
        nrf_gpio_pin_set(SPI_PSELSS1);         /* disable Set slave select (inactive high) */
        SEL_SS_PINOUT = SPI_PSELSS1;
    }
#endif
    spi_base_address->FREQUENCY = (uint32_t) SPI_OPERATING_FREQUENCY;

    switch (mode ) {
            /*lint -e845 -save // A zero has been given as right argument to operator '!'" */
        case SPI_MODE0:
            config_mode = (SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) | (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos);
            break;
        case SPI_MODE1:
            config_mode = (SPI_CONFIG_CPHA_Trailing << SPI_CONFIG_CPHA_Pos) | (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos);
            break;
        case SPI_MODE2:
            config_mode = (SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) | (SPI_CONFIG_CPOL_ActiveLow << SPI_CONFIG_CPOL_Pos);
            break;
        case SPI_MODE3:
            config_mode = (SPI_CONFIG_CPHA_Trailing << SPI_CONFIG_CPHA_Pos) | (SPI_CONFIG_CPOL_ActiveLow << SPI_CONFIG_CPOL_Pos);
            break;
        default:
            config_mode = 0;
            break;
            /*lint -restore */
    }
    if (lsb_first) {
        /*lint -e{845} // A zero has been given as right argument to operator '|'" */
        spi_base_address->CONFIG = (config_mode | (SPI_CONFIG_ORDER_LsbFirst << SPI_CONFIG_ORDER_Pos));
    } else {
        /*lint -e{845} // A zero has been given as right argument to operator '|'" */
        spi_base_address->CONFIG = (config_mode | (SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos));
    }

    spi_base_address->EVENTS_READY = 0U;

    return (uint32_t *)spi_base_address;
}

bool spi_master_tx_rx(uint32_t *spi_base_address, uint8_t transfer_size, const uint8_t *tx_data, uint8_t *rx_data)
{
    uint32_t counter = 0;
    uint16_t number_of_txd_bytes = 0;
    uint32_t SEL_SS_PINOUT = SPI_PSELSS0;
    /*lint -e{826} //Are too small pointer conversion */
    NRF_SPI_Type *spi_base = (NRF_SPI_Type *)spi_base_address;

    if( (uint32_t *)NRF_SPI0 == spi_base_address) {
        SEL_SS_PINOUT = SPI_PSELSS0;
    }
#ifdef SPI1
    else {
        SEL_SS_PINOUT = SPI_PSELSS1;
    }
#endif
    /* enable slave (slave select active low) */
    nrf_gpio_pin_clear(SEL_SS_PINOUT);

    while(number_of_txd_bytes < transfer_size) {
        spi_base->TXD = (uint32_t)(tx_data[number_of_txd_bytes]);

        /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        while ((spi_base->EVENTS_READY == 0U) && (counter < TIMEOUT_COUNTER)) {
            counter++;
        }

        if (counter == TIMEOUT_COUNTER) {
            /* timed out, disable slave (slave select active low) and return with error */
            nrf_gpio_pin_set(SEL_SS_PINOUT);
            return false;
        } else {   /* clear the event to be ready to receive next messages */
            spi_base->EVENTS_READY = 0U;
        }

        rx_data[number_of_txd_bytes] = (uint8_t)spi_base->RXD;
        number_of_txd_bytes++;
    };

    /* disable slave (slave select active low) */
    nrf_gpio_pin_set(SEL_SS_PINOUT);

    return true;
}

void spi_master_enable(SPIModuleNumber module_number)
{

    NRF_SPI_Type *spi_base_address = (SPI0 == module_number)? NRF_SPI0 : (NRF_SPI_Type *)NRF_SPI1;

    if(SPI0 == module_number) {
        /* Configure GPIO pins used for pselsck, pselmosi, pselmiso and pselss for SPI0 */
        nrf_gpio_cfg_output(SPI_PSELSCK0);
        nrf_gpio_cfg_output(SPI_PSELMOSI0);
        nrf_gpio_cfg_input(SPI_PSELMISO0, NRF_GPIO_PIN_NOPULL);
        nrf_gpio_cfg_output(SPI_PSELSS0);
    }
#ifdef SPI1
    else {
        /* Configure GPIO pins used for pselsck, pselmosi, pselmiso and pselss for SPI1*/
        nrf_gpio_cfg_output(SPI_PSELSCK1);
        nrf_gpio_cfg_output(SPI_PSELMOSI1);
        nrf_gpio_cfg_input(SPI_PSELMISO1, NRF_GPIO_PIN_NOPULL);
        nrf_gpio_cfg_output(SPI_PSELSS1);
    }
#endif
    spi_base_address->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
}

void spi_master_disable(SPIModuleNumber module_number)
{
    NRF_SPI_Type *spi_base_address = (SPI0 == module_number)? NRF_SPI0 : (NRF_SPI_Type *)NRF_SPI1;

    if(SPI0 == module_number) {
        NRF_GPIO->PIN_CNF[SPI_PSELSCK0] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        NRF_GPIO->PIN_CNF[SPI_PSELMOSI0] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        NRF_GPIO->PIN_CNF[SPI_PSELMISO0] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        NRF_GPIO->PIN_CNF[SPI_PSELSS0] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
    }
#ifdef SPI1
    else {
        NRF_GPIO->PIN_CNF[SPI_PSELSCK1] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        NRF_GPIO->PIN_CNF[SPI_PSELMOSI1] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        NRF_GPIO->PIN_CNF[SPI_PSELMISO1] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
        NRF_GPIO->PIN_CNF[SPI_PSELSS1] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
    }
#endif
    spi_base_address->ENABLE = (SPI_ENABLE_ENABLE_Disabled << SPI_ENABLE_ENABLE_Pos);

}
