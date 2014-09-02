/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
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

#include <spi_master.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "common.h"
#include "spi_master_config.h" // This file must be in the application folder

uint32_t* spi_master_init(SPIModuleNumber module_number, SPIMode mode, bool lsb_first)
{
    uint32_t config_mode;

    NRF_SPI_Type *spi_base_address = (SPI0 == module_number)? NRF_SPI0 : (NRF_SPI_Type *)NRF_SPI1;

    if(SPI0 == module_number)
    {
        /* Configure GPIO pins used for pselsck, pselmosi, pselmiso and pselss for SPI0 */
        nrf_gpio_cfg_output(SPI_PSELSCK0);
        nrf_gpio_cfg_output(SPI_PSELMOSI0);
        nrf_gpio_cfg_input(SPI_PSELMISO0, NRF_GPIO_PIN_NOPULL);
        nrf_gpio_cfg_output(SPI_PSELSS0);

        /* Configure pins, frequency and mode */
        spi_base_address->PSELSCK  = SPI_PSELSCK0;
        spi_base_address->PSELMOSI = SPI_PSELMOSI0;
        spi_base_address->PSELMISO = SPI_PSELMISO0;
        nrf_gpio_pin_set(SPI_PSELSS0); /* disable Set slave select (inactive high) */
    }
    else
    {
        /* Configure GPIO pins used for pselsck, pselmosi, pselmiso and pselss for SPI1*/
        nrf_gpio_cfg_output(SPI_PSELSCK1);
        nrf_gpio_cfg_output(SPI_PSELMOSI1);
        nrf_gpio_cfg_input(SPI_PSELMISO1, NRF_GPIO_PIN_NOPULL);
        nrf_gpio_cfg_output(SPI_PSELSS1);

        /* Configure pins, frequency and mode */
        spi_base_address->PSELSCK  = SPI_PSELSCK1;
        spi_base_address->PSELMOSI = SPI_PSELMOSI1;
        spi_base_address->PSELMISO = SPI_PSELMISO1;
        nrf_gpio_pin_set(SPI_PSELSS1);         /* disable Set slave select (inactive high) */
    }

    spi_base_address->FREQUENCY = (uint32_t) SPI_OPERATING_FREQUENCY;

    switch (mode )
    {
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
    if (lsb_first)
    {
        /*lint -e{845} // A zero has been given as right argument to operator '|'" */
        spi_base_address->CONFIG = (config_mode | (SPI_CONFIG_ORDER_LsbFirst << SPI_CONFIG_ORDER_Pos));
    }
    else
    {
        /*lint -e{845} // A zero has been given as right argument to operator '|'" */
        spi_base_address->CONFIG = (config_mode | (SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos));
    }

    spi_base_address->EVENTS_READY = 0U;

    /* Enable */
    spi_base_address->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);

    return (uint32_t *)spi_base_address;
}

bool spi_master_tx_rx(uint32_t *spi_base_address, uint16_t transfer_size, const uint8_t *tx_data, uint8_t *rx_data)
{
    uint32_t counter = 0;
    uint16_t number_of_txd_bytes = 0;
    uint32_t SEL_SS_PINOUT;
    /*lint -e{826} //Are too small pointer conversion */
    NRF_SPI_Type *spi_base = (NRF_SPI_Type *)spi_base_address;

    if( (uint32_t *)NRF_SPI0 == spi_base_address)
    {
        SEL_SS_PINOUT = SPI_PSELSS0;
    }
    else
    {
        SEL_SS_PINOUT = SPI_PSELSS1;
    }

    /* enable slave (slave select active low) */
    nrf_gpio_pin_clear(SEL_SS_PINOUT);

    while(number_of_txd_bytes < transfer_size)
    {
        spi_base->TXD = (uint32_t)(tx_data[number_of_txd_bytes]);

        /* Wait for the transaction complete or timeout (about 10ms - 20 ms) */
        while ((spi_base->EVENTS_READY == 0U) && (counter < TIMEOUT_COUNTER))
        {
            counter++;
        }

        if (counter == TIMEOUT_COUNTER)
        {
            /* timed out, disable slave (slave select active low) and return with error */
            nrf_gpio_pin_set(SEL_SS_PINOUT);
            return false;
        }
        else
        {   /* clear the event to be ready to receive next messages */
            spi_base->EVENTS_READY = 0U;
        }

        rx_data[number_of_txd_bytes] = (uint8_t)spi_base->RXD;
        number_of_txd_bytes++;
    };

    /* disable slave (slave select active low) */
    nrf_gpio_pin_set(SEL_SS_PINOUT);

    return true;
}
