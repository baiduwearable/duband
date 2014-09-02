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
#ifndef WX1000_BOARD_CONFIG_PINS_H__
#define WX1000_BOARD_CONFIG_PINS_H__

// Device Information

#define DEVICE_NAME       "du-Band"   /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME "Baidu Inc"   /**< Manufacturer. Will be passed to Device Information Service. */
#define FW_REV_STR        "1.0.0"
#define MODULE_NUM        "du100"     /**< Model Number String. */
#define SERIAL_NUM        "0123456789abcde"   /**< Serial Number String. */
#define HW_REV_STR        "1.0.0"     /**< Hardware Revision String. */

#define BATTERY_VOLTAGE_ADJUSTMENT           60     /**< Adjustment for charging */

//#define FEATURE_BUTTON
#define FEATURE_LED_PROGRESS
//#define FEATURE_LED_CLOCK
#define FEATURE_MOTOR
//#define FEATURE_MOTOR_LRA
//#define FEATURE_TWI
#define FEATURE_SENSOR_LIS3DH
#define FEATURE_PRE_INIT
//#define FEATURE_AUTO_SLEEP
#define FEATURE_AUTO_WAKEUP
/*********************************************************************
* pre_init feature control 
**********************************************************************/


//GPIO pin number for LEDs
#ifdef FEATURE_LED_PROGRESS
  #define BAIDU_LED_0           7
  #define BAIDU_LED_1           6 
  #define BAIDU_LED_2           5
  #define BAIDU_LED_3           2
  #define BAIDU_LED_4           18
#endif

//GPIO pin number for MOTOR
#ifdef FEATURE_MOTOR
  #define BAIDU_MOTOR_0         29
#endif
//GPIO pin number for LRA
#ifdef FEATURE_MOTOR_LRA
  #define LRA_EN_PIN     13 /*!< GPIO pin number for LRA_EN           */
  #define LRA_PWM_PIN     14 /*!< GPIO pin number for LRA_PWM           */
  #define LRA_SCL_PIN     15 /*!< GPIO pin number for LRA_SCL           */
  #define LRA_SDA_PIN     16 /*!< GPIO pin number for LRA_SDA           */

//I2C define
#define TWI_MASTER_CONFIG_CLOCK_PIN_NUMBER LRA_SCL_PIN
  #define TWI_MASTER_CONFIG_DATA_PIN_NUMBER LRA_SDA_PIN
#endif

//GPIO pin number for BUTTON
#ifdef FEATURE_BUTTON
  #define BAIDU_BUTTON_0           /*not use */
#endif

//XXX
#define ADVERTISING_LED_PIN_NO      BAIDU_LED_0
#define CONNECTED_LED_PIN_NO        BAIDU_LED_1
#define ASSERT_LED_PIN_NO           BAIDU_LED_2

//GPIO pin number for Charging
#define CHARGER_CONNECTED_PIN 11
#define CHARGER_CHARGING_PIN  12  /*!< GPIO pin number for Charging State           */

/* GPIO pin number for SPI */
#define SPI_PSELSCK0              4   /*!< GPIO pin number for SPI clock (note that setting this to 31 will only work for loopback purposes as it not connected to a pin) */
#define SPI_PSELMOSI0             3   /*!< GPIO pin number for Master Out Slave In    */
#define SPI_PSELMISO0             21   /*!< GPIO pin number for Master In Slave Out    */
#define SPI_PSELSS0               22   /*!< GPIO pin number for Slave Select           */

/* GPIO pin number for LIS3DH */
#ifdef FEATURE_SENSOR_LIS3DH
  #define LIS3DH_INT1     8 /*!< GPIO pin number for LIS3DH_INT1           */
  #define LIS3DH_INT2     28 /*!< GPIO pin number for LIS3DH_INT2          */
#endif


/*****************************************************************
* uart pin config
******************************************************************/
#if defined(DEBUG_LOG) || defined (DEBUG_ACC) || defined (DEBUG_PHILL)

#define RX_PIN_NUMBER   25    // UART RX pin number.
#define TX_PIN_NUMBER   23    // UART TX pin number.
//the follow pin is useless
#define CTS_PIN_NUMBER 43    // UART Clear To Send pin number. Not used if HWFC is set to false
#define RTS_PIN_NUMBER 43    // Not used if HWFC is set to false
#define HWFC           false // UART hardware flow control

#endif  //DEBUG_LOG

#endif // WX1000_BOARD_CONFIG_PINS_H__

/** @} */
