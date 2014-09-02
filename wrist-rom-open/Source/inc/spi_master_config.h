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
#ifndef SPI_MASTER_CONFIG_H
#define SPI_MASTER_CONFIG_H

#define SPI_OPERATING_FREQUENCY  ( 0x02000000UL << (uint32_t)Freq_8Mbps)  /*!< Slave clock frequency. */
#define NUMBER_OF_TEST_BYTES     2    /*!< number of bytes to send to slave to test if Initialization was successful */
#define TEST_BYTE                0xBB /*!< Randomly chosen test byte to transmit to spi slave */
#define TIMEOUT_COUNTER          0x3000UL  /*!< timeout for getting rx bytes from slave */

#ifdef SPI1
/*  SPI1 */
#define SPI_PSELSCK1              29   /*!< GPIO pin number for SPI clock              */
#define SPI_PSELMOSI1             21   /*!< GPIO pin number for Master Out Slave In    */
#define SPI_PSELMISO1             23   /*!< GPIO pin number for Master In Slave Out    */
#define SPI_PSELSS1               28   /*!< GPIO pin number for Slave Select           */
#endif
//#define DEBUG
#ifdef DEBUG
#define DEBUG_EVENT_READY_PIN0    10    /*!< when DEBUG is enabled, this GPIO pin is toggled everytime READY_EVENT is set for SPI0, no toggling means something has gone wrong */
#define DEBUG_EVENT_READY_PIN1    11    /*!< when DEBUG is enabled, this GPIO pin is toggled everytime READY_EVENT is set for SPI1, no toggling means something has gone wrong */



/** @def  TX_RX_MSG_LENGTH
 * number of bytes to transmit and receive. This amount of bytes will also be tested to see that
 * the received bytes from slave are the same as the transmitted bytes from the master */
#define TX_RX_MSG_LENGTH   100

/** @def ERROR_PIN_SPI0
 * This pin is set active high when there is an error either in TX/RX for SPI0 or if the received bytes does not totally match the transmitted bytes.
 * This functionality can be tested by temporarily disconnecting the MISO pin while running this example.
 */
#define ERROR_PIN_SPI0   8UL

/** @def ERROR_PIN_SPI1
 * This pin is set active high when there is an error either in TX/RX for SPI1 or if the received bytes does not totally match the transmitted bytes.
 * This functionality can be tested by temporarily disconnecting the MISO pin while running this example.
 */
#define ERROR_PIN_SPI1   9UL
#endif
#endif /* SPI_MASTER_CONFIG_H */
