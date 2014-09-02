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
#ifndef _CONFIG_H__
#define _CONFIG_H__

#define GLOBAL_RECEIVE_BUFFER_SIZE 100
#define GLOBAL_RESPONSE_BUFFER_SIZE 200

#define APP_TIMER_PRESCALER               0                                /**< Value of the RTC1 PRESCALER register. */

//#define FUNCTION_BUTTON          BAIDU_BUTTON_0                 /* function button for wearable devices */
#define ALERT_LEVEL_MILD_LED_PIN_NO       BAIDU_LED_2                                     /**< Is on when we are in Mild Alert state. */
#define ALERT_LEVEL_HIGH_LED_PIN_NO       BAIDU_LED_3                                     /**< Is on when we are in High Alert state. */
#define ADV_INTERVAL_SLOW_LED_PIN_NO      BAIDU_LED_4                                     /**< Is on when we are doing slow advertising. */
#define PEER_SRV_DISC_LED_PIN_NO          BAIDU_MOTOR_0                                     /**< Is on when the Immediate Alert Service has been discovered at the peer. */

#define APP_ADV_INTERVAL_FAST             0x0050                                            /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 50 ms.). */

#define APP_ADV_INTERVAL_SLOW             0x0640                                            /**< Slow advertising interval (in units of 0.625 ms. This value corrsponds to 1 seconds). */
#define APP_SLOW_ADV_TIMEOUT              180                                               /**< The duration of the slow advertising period (in seconds). */
#define APP_FAST_ADV_TIMEOUT              40                                                /**< The duration of the fast advertising period (in seconds). */
#define APP_FAST_ADV_WHITELIST_TIMEOUT    20                                                /**< The duration of the fast advertising with whitelist period (in seconds). */


#define APP_TIMER_MAX_TIMERS              16                                                 /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE           10                                                 /**< Size of timer operation queues. */

#define BATTERY_LEVEL_MEAS_INTERVAL       APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)        /**< Battery level measurement interval (ticks). */
#define MIN_BATTERY_LEVEL                 81                                                /**< Minimum simulated battery level. */
#define MAX_BATTERY_LEVEL                 100                                               /**< Maximum simulated battery level. */
#define BATTERY_LEVEL_INCREMENT           1                                                 /**< Increment between each simulated battery level measurement. */

#define MIN_CONN_INTERVAL                 MSEC_TO_UNITS(400, UNIT_1_25_MS)                  /**< Minimum acceptable connection interval (20 ms). */
#define MAX_CONN_INTERVAL                 MSEC_TO_UNITS(500, UNIT_1_25_MS)                 /**< Maximum acceptable connection interval (1 second). */

#define SLAVE_LATENCY                     0                                                 /**< Slave latency. */
#define CONN_SUP_TIMEOUT                  MSEC_TO_UNITS(4000, UNIT_10_MS)                   /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(10 * 1000, APP_TIMER_PRESCALER)   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY     APP_TIMER_TICKS(5 * 1000, APP_TIMER_PRESCALER)    /**< Time between each call to sd_ble_gap_conn_param_update after the first (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT      3                                                 /**< Number of attempts before giving up the connection parameter negotiation. */

#define APP_GPIOTE_MAX_USERS              2                                                 /**< Maximum number of users of the GPIOTE handler. */

#define BUTTON_DETECTION_DELAY            APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)          /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define SEC_PARAM_TIMEOUT                 30                                                /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                    1                                                 /**< Perform bonding. */
#define SEC_PARAM_MITM                    0                                                 /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES         BLE_GAP_IO_CAPS_NONE                              /**< No I/O capabilities. */
#define SEC_PARAM_OOB                     0                                                 /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE            7                                                 /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE            16                                                /**< Maximum encryption key size. */

#define INITIAL_LLS_ALERT_LEVEL           BLE_CHAR_ALERT_LEVEL_NO_ALERT                     /**< Initial value for the Alert Level characteristic in the Link Loss service. */
#define TX_POWER_LEVEL                    (-4)                                              /**< TX Power Level value. This will be set both in the TX Power service, in the advertising data, and also used to set the radio transmit power. */

/********************************************************************************
* Device loss alert rssi threshold
*********************************************************************************/
#define DEV_LOSS_ALERT_RSSI_THRESHOLD      (-90)                                            /**< start alert threshold*/
#define DEV_LOSS_RESUME_RSSI_THRESHOLD     (-70)                                            /**<  threshold for restart alert judge */
#define DEV_LOSS_START_ALARM_COUNT     (5)     

#define PAGE_NUM_FOR_DATA                 (32)
#define PAGE_NUM_FOR_SWAP                 (60)
#define PAGE_NUM_FOR_CONFIG               (10)

#define OTA_BLOCK_START_PAGE              (NRF_FICR->CODESIZE-14)
#define CONFIG_BLOCK_START_PAGE           (NRF_FICR->CODESIZE-24)
#define DATA_BLOCK_START_PAGE             (NRF_FICR->CODESIZE-56)
#define SWAP_BLOCK_STAET_PAGE             (NRF_FICR->CODESIZE-116)

//half close half open area just like "[)"
#define ERASABLE_BLOCK_START              (DATA_BLOCK_START_PAGE)
#define ERASABLE_BLOCK_END                (CONFIG_BLOCK_START_PAGE + PAGE_NUM_FOR_CONFIG)

#define FLASH_PAGE_STORE_TIME             (NRF_FICR->CODESIZE-23)                           /**< used to store */
#define FLASH_PAGE_DAILY_TARGET      (NRF_FICR->CODESIZE-22)           /**< Flash page used for SN and factory test flag*/
#define FLASH_PAGE_ERROR_LOG       (NRF_FICR->CODESIZE-21)                           /**< Address in flash where stack trace can be stored. */
#define FLASH_PAGE_SYS_ATTR               (NRF_FICR->CODESIZE-20)                           /**< Flash page used for bond manager system attribute information. */
#define FLASH_PAGE_BOND                   (NRF_FICR->CODESIZE-19)                           /**< 232 Flash page used for bond manager bonding information. */
#define FLASH_PAGE_PRIVATE_BOND           (NRF_FICR->CODESIZE-18)
#define FLASH_PAGE_USER_PROFILE     (NRF_FICR->CODESIZE-17)           /**< Flash page used for user profile*/
#define FLASH_PAGE_ALARM_SETTINGS    (NRF_FICR->CODESIZE-16)           /**< Flash page used for alarm settings*/
#define FLASH_PAGE_SN_FACTORY_FLAG    (NRF_FICR->CODESIZE-15)           /**< Flash page used for SN and factory test flag*/

#define FLASH_PAGE_HEALTH_DATA            DATA_BLOCK_START_PAGE                             /**< start page used to store sports data > */
#define FLASH_PAGE_SLEEP_SETTINGS         (NRF_FICR->CODESIZE-25)                           /**< used to store sleep setting data*/

#define FLASH_PAGE_SIZE                   (NRF_FICR->CODEPAGESIZE)


#define DEAD_BEEF                         0xDEADBEEF                                        /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define MAX_ALARM_NUM       (8)
typedef enum
{
    BLE_DISCONNECTED,                                                                             /**< No advertising running. */
    BLE_CONNECTED,                                                                        /**< Go to system-off. */
} ble_connection_status_t;

/*****************************************************************************
* add codes for enalbe app schedule
******************************************************************************/
#define SCHED_MAX_EVENT_DATA_SIZE       sizeof(app_timer_event_t)                   /**< Maximum size of scheduler events. Note that scheduler BLE stack events do not contain any data, as the events are being pulled from the stack in the event handler. */
#define SCHED_QUEUE_SIZE                20                                          /**< Maximum number of events in the scheduler queue. */


#define ARRAY_LEN(a)            (sizeof(a)/sizeof(a[0]))          /* calculate array length */

//power limit
#define PRIVATE_BOND_POWER_LIMIT    (20)
#define FIRMWARE_UPDATE_POWER_LIMIT (40)

#endif  //_CONFIG_H__
