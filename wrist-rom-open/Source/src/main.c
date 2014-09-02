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
#include "app_util.h"
#include "config.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_tps.h"
#include "ble_bas.h"
#include "ble_conn_params.h"
#include "board_config_pins.h"
#include "ble_sensorsim.h"
#include "ble_stack_handler.h"
#include "app_timer.h"
#include "ble_bondmngr.h"
#include "ble_error_log.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "ble_radio_notification.h"
#include "ble_flash.h"
#include "ble_debug_assert_handler.h"
#include "app_scheduler.h"
#include "bd_wall_clock_timer.h"
#include "ble_dis.h"
#include "bd_ble_nus.h"
#include "bd_buzzer.h"
#include "bd_charging.h"
#include "bd_battery.h"
#include "bd_communicate_protocol.h"
#include "bd_input_event_source.h"
#include "bd_led_flash.h"
#include "bd_work_state.h"
#include "step_counter.h"
#include "bd_private_bond.h"
#include "bd_level_drive_motor.h"
#include "bd_factory_test.h"
#include "bd_low_power_mode.h"
#include "bd_interaction.h"
#include "hal_acc.h"
#include "bd_switch_sleep.h"
#include "bd_wdt.h"
#include "bd_sync_data.h"
#include "bd_data_manager.h"


//OTA package should not enable DEBUG_LOG
#ifdef BUILD_FOR_OTA
#ifdef DEBUG_LOG
#error "build ota package should not enable DEBUG_LOG";
#endif
#endif
#include "simple_uart.h"

typedef enum {
    BLE_NO_ADV,                                                                             /**< No advertising running. */
    BLE_FAST_ADV_WHITELIST,                                                                 /**< Advertising whith whitelist. */
    BLE_FAST_ADV,                                                                           /**< Fast advertising running. */
    BLE_SLOW_ADV,                                                                           /**< Slow advertising running. */
    BLE_SLEEP                                                                               /**< Go to system-off. */
} ble_advertising_mode_t;

/****************************************************************************
* global instance of key struct
*****************************************************************************/                                             /**< steps counts one day */
extern  bool                             should_update_prv_bond_info;

static ble_tps_t                         m_tps;                                            /**< Structure used to identify the TX Power service. */

ble_bas_t                                m_bas;                                            /**< Structure used to identify the battery service. */

ble_nus_t                                m_nus;                                                                                                         /**< ble_nus service struct*/

static ble_gap_sec_params_t               m_sec_params;                                     /**< Security requirements for this application. */
static ble_advertising_mode_t             m_advertising_mode;                               /**< Variable to keep track of when we are advertising. */

static volatile bool                      m_is_high_alert_signalled;                        /**< Variable to indicate whether or not high alert is signalled to the peer. */

#ifdef HAS_BUTTON
app_timer_id_t  btn_identi;                                                                 /** <Button identified*/
#endif

/******************************************************************************
 * timers for OTA delay
 ******************************************************************************/
app_timer_id_t  ota_delay_timer;

extern app_timer_id_t user_action_delay_timer;    /* wait for user action timeout timer */
/******************************************************************************
* global bluetooth connect handle
******************************************************************************/
uint16_t global_connect_handle = BLE_CONN_HANDLE_INVALID;
extern BLUETOOTH_BOND_STATE private_bond_machine;

extern bool notify_steps_enable;


/*****************************************************************************
* Predeclear
******************************************************************************/
static void advertising_init(uint8_t adv_flags);


/**@brief Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Error handler function, which is called when an error has occurred.
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
    nrf_gpio_pin_clear(ASSERT_LED_PIN_NO);

    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
#ifdef BUILD_FOR_OTA

    trigger_ota_mode();
#else

    ble_debug_assert_handler(error_code, line_num, p_file_name);
    // On assert, the system can only recover on reset
    NVIC_SystemReset();
#endif
}


/**@brief Assert macro callback function.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Start advertising.
 */
static void advertising_start(void)
{
    uint32_t             err_code;
    ble_gap_adv_params_t adv_params;
    ble_gap_whitelist_t  whitelist;

    // Initialize advertising parameters with defaults values
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    adv_params.p_whitelist = NULL;

#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"%s-----",DEVICE_NAME);
#endif

    // Configure advertisement according to current advertising state
    switch (m_advertising_mode) {
        case BLE_NO_ADV:
            m_advertising_mode = BLE_FAST_ADV;
            // fall through

        case BLE_FAST_ADV_WHITELIST:
            err_code = ble_bondmngr_whitelist_get(&whitelist);
            APP_ERROR_CHECK(err_code);

            if ((whitelist.addr_count != 0) || (whitelist.irk_count != 0)) {
                adv_params.fp          = BLE_GAP_ADV_FP_FILTER_CONNREQ;
                adv_params.p_whitelist = &whitelist;

                advertising_init(BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED);
                m_advertising_mode = BLE_FAST_ADV;
            } else {
                m_advertising_mode = BLE_SLOW_ADV;
            }

            adv_params.interval = APP_ADV_INTERVAL_FAST;
            adv_params.timeout  = APP_FAST_ADV_WHITELIST_TIMEOUT;
            set_global_bluetooth_status(FastAdvertising);
            break;

        case BLE_FAST_ADV:
            advertising_init(BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE);

            adv_params.interval = APP_ADV_INTERVAL_FAST;
            adv_params.timeout  = APP_FAST_ADV_TIMEOUT;
            m_advertising_mode  = BLE_SLOW_ADV;
            set_global_bluetooth_status(FastAdvertising);
            break;

        case BLE_SLOW_ADV:
            advertising_init(BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE);

            adv_params.interval = APP_ADV_INTERVAL_SLOW;
            adv_params.timeout  = APP_SLOW_ADV_TIMEOUT;
            m_advertising_mode  = BLE_SLOW_ADV;

            set_global_bluetooth_status(SlowAdvertising);
            break;

        default:
            break;
    }

    // Start advertising
    err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);

}

/**@brief LEDs initialization.
 *
 * @details Initializes all LEDs used by this application.
 */
static void leds_init(void)
{
    GPIO_LED_CONFIG(BAIDU_LED_0);
    GPIO_LED_CONFIG(BAIDU_LED_1);
    GPIO_LED_CONFIG(BAIDU_LED_2);
    GPIO_LED_CONFIG(BAIDU_LED_3);
    GPIO_LED_CONFIG(BAIDU_LED_4);

    nrf_gpio_pin_set(BAIDU_LED_0);
    nrf_gpio_pin_set(BAIDU_LED_1);
    nrf_gpio_pin_set(BAIDU_LED_2);
    nrf_gpio_pin_set(BAIDU_LED_3);
    nrf_gpio_pin_set(BAIDU_LED_4);
}


void ota_time_out_handle(void * context);
void timer_init()
{
    // Initialize timer module
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, true);
}

/**@brief Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
extern app_timer_id_t m_erase_flash_timer_id;

extern app_timer_id_t m_double_tap_timer_id;

static void timers_create(void)
{
    uint32_t err_code;


#ifdef HAS_BUTTON

    err_code = app_timer_create(&btn_identi, APP_TIMER_MODE_SINGLE_SHOT, btn_timeout_handler);
    APP_ERROR_CHECK(err_code);
#endif

    err_code = led_flash_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_sensor_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                sensor_timer_handle);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_double_tap_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                tap_timer_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&ota_delay_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                ota_time_out_handle);
    APP_ERROR_CHECK(err_code);

    create_send_data_timer();

    err_code = motor_control_framework_init();
    APP_ERROR_CHECK(err_code);

}


/**@brief GAP initialization.
 *
 * @details This function shall be used to setup all the necessary GAP (Generic Access Profile)
 *          parameters of the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_KEYRING);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_tx_power_set(TX_POWER_LEVEL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Advertising functionality initialization.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 *
 * @param[in]  adv_flags  Indicates which type of advertisement to use, see @ref BLE_GAP_DISC_MODES.
 *
 */
static void advertising_init(uint8_t adv_flags)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    int8_t        tx_power_level = TX_POWER_LEVEL;


    m_advertising_mode = BLE_FAST_ADV;

    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags.size              = sizeof(adv_flags);
    advdata.flags.p_data            = &adv_flags;
    advdata.p_tx_power_level        = &tx_power_level;
    advdata.uuids_complete.uuid_cnt = 0;
    advdata.uuids_complete.p_uuids  = NULL;
    advdata.uuids_more_available.uuid_cnt = 0;
    advdata.uuids_solicited.uuid_cnt = 0;
    advdata.p_slave_conn_int        = NULL;

    /*********************************************
    * Mac address
    **********************************************/
    uint64_t mac_address = 0;
    uint8_t mac_add_array[BLE_GAP_ADDR_LEN - sizeof(uint16_t)];
    int8_t i = 0;

    ble_gap_addr_t device_addr;
    if(NRF_SUCCESS == sd_ble_gap_address_get(&device_addr)) {
        for(i = 0; i < BLE_GAP_ADDR_LEN; ++i) {
           mac_address |= ((uint64_t)device_addr.addr[i]) << (i * 8); 
        }
    } else {
        mac_address = ((((uint64_t)(NRF_FICR->DEVICEADDR[1] & 0xFFFF)) << 32) | ((uint64_t)NRF_FICR->DEVICEADDR[0]));    
    }
    
    for(i = (BLE_GAP_ADDR_LEN - sizeof(uint16_t) - 1); i>=0; --i) {
            mac_add_array[(BLE_GAP_ADDR_LEN - sizeof(uint16_t) - 1) - i] =  (mac_address >> 8*i) & 0xff;
    }

    ble_advdata_manuf_data_t adv_manu;
    adv_manu.data.p_data = mac_add_array;
    adv_manu.data.size = BLE_GAP_ADDR_LEN - sizeof(uint16_t);
    adv_manu.company_identifier = ((mac_address >> 40) & 0xFF) | (((mac_address >> 32)& 0xFF) << 8);

    advdata.p_manuf_specific_data = &adv_manu;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Initialize Tx Power Service.
 */
static void tps_init(void)
{
    uint32_t       err_code;
    ble_tps_init_t tps_init_obj;

    memset(&tps_init_obj, 0, sizeof(tps_init_obj));
    tps_init_obj.initial_tx_power_level = TX_POWER_LEVEL;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&tps_init_obj.tps_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&tps_init_obj.tps_attr_md.write_perm);

    err_code = ble_tps_init(&m_tps, &tps_init_obj);
    APP_ERROR_CHECK(err_code);
}

void battery_up(void * p_event_data, uint16_t event_size);
/**@brief Initialize Battery Service.
 */
static void bas_init(void)
{
    uint32_t       err_code;
    ble_bas_init_t bas_init_obj;

    memset(&bas_init_obj, 0, sizeof(bas_init_obj));

    bas_init_obj.evt_handler          = NULL;
    bas_init_obj.support_notification = false; /* Warning: donot change it to true if you do not undstand what you are doing & what it will cause */
    bas_init_obj.p_report_ref         = NULL;
    bas_init_obj.initial_batt_level   = 100;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init_obj.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_report_read_perm);

    err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);
    register_battery_up_callback(battery_up);
}


/*******************************************************************************
* Device information init
********************************************************************************/
static void ble_devinfo_serv_init(void)
{
    uint32_t        err_code;
    ble_dis_init_t  dis_init;

    // Initialize Device Information Service
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, MANUFACTURER_NAME);
    ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, FW_REV_STR);
    ble_srv_ascii_to_utf8(&dis_init.model_num_str,MODULE_NUM);
    ble_srv_ascii_to_utf8(&dis_init.hw_rev_str,HW_REV_STR);


    uint8_t serial_num[33];
    serial_num[32] = 0;

    uint32_t *addr;
    ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &addr );

    if(is_factory_test_done()) {//has do factory test
        for(uint8_t i = 0 ; i<SERIAL_NUMBER_LENGTH ; ++i) {
            serial_num[i] = *(((uint8_t *) (addr)) + i);
        }
        ble_srv_ascii_to_utf8(&dis_init.serial_num_str,(char *)serial_num);
    } else {
        ble_srv_ascii_to_utf8(&dis_init.serial_num_str,SERIAL_NUM);
    }

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}
/****************************************************************
* ble_nus_service_init
*****************************************************************/
static void ble_nus_service_init()
{
    uint32_t err_code;
    static ble_nus_init_t nus_init;

    memset(&nus_init, 0, sizeof nus_init);
    nus_init.data_handler = L1_receive_data;
    nus_init.connect_handler = bluetooth_l0_reset;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Initialize services that will be used by the application.
 */
static void services_init(void)
{
    tps_init();
    bas_init();
    ble_nus_service_init();
    ble_devinfo_serv_init();
}


/**@brief Initialize security parameters.
 */
static void sec_params_init(void)
{
    m_sec_params.timeout      = SEC_PARAM_TIMEOUT;
    m_sec_params.bond         = SEC_PARAM_BOND;
    m_sec_params.mitm         = SEC_PARAM_MITM;
    m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    m_sec_params.oob          = SEC_PARAM_OOB;
    m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}


/**@brief Connection Parameters module error handler.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Initialize the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/*****************************************************************************
* dev loss alert level
******************************************************************************/
static int16_t rssi_average = 0x0fff;
static void init_rssi_average(void)
{
    rssi_average = 0x0fff;
}

static void dev_loss_alert_handle(int8_t rssi,DEV_LOSS_ALERT_LEVEL alert_level)
{
   //alert only when current connect is bonded
    if(private_bond_machine != PRIVATE_BOND_SUCCESS) {
        return;
    }

    //smooth
    if(rssi_average == 0x0fff){
        rssi_average = rssi;
    } else {
        rssi_average = rssi_average/3 * 2 + rssi/3;
    }


    if(get_global_dev_loss_should_alert() == 0 &&  rssi_average > DEV_LOSS_RESUME_RSSI_THRESHOLD) {
        set_global_dev_loss_should_alert(1);
    } 
}


/******************************************************
* To show Bluetooth bond info is full so should Delete
*******************************************************/
static bool should_delete_bluetooth_bond_info = false;
/**@brief Application's BLE Stack event handler.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
#ifdef TEST_SLEEP
extern uint8_t *g_sleep_send_buffer;
extern  SleepHead_t  mSleepHead;
uint8_t sleepD[34]= {0x05,0x00,0x03,0x00,0x1c,0x1b,0x98,0x00,0x06,0x04,0x9a,0x00,0x02,0x04,0xa3,0x00,0x01,0x04,0xbb,00,
                     0x02,0x04,0xbc,0x00,0x01,0x04,0xd3,0x00,0x03,0x04,0xd4,0x00,0x02
                    };
void test_sleep_setting_data(void)
{
    mSleepHead.length = 6;
    for(int ii = 0; ii<34; ii ++ )
        g_sleep_send_buffer[ii] = sleepD[ii];
}
#endif
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t        err_code      = NRF_SUCCESS;
    int8_t rssi_value;

    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:

            m_advertising_mode = BLE_FAST_ADV;
            global_connect_handle = p_ble_evt->evt.gap_evt.conn_handle;

            sd_ble_gap_rssi_stop(global_connect_handle);
            //start get RSSI value of the current connection
            sd_ble_gap_rssi_start(global_connect_handle);

            //init rssi_average every time connect to a device
            init_rssi_average();
            
            app_timer_start(user_action_delay_timer,WAIT_BOND_COMMAND_TIMEOUT,(void *)DO_WAIT_BOND_COMMAND);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
#ifdef TEST_SLEEP

            test_sleep_setting_data();
#endif

            notify_steps_enable = false;

           set_sport_data_sync_process_send(false);

            global_connect_handle = BLE_CONN_HANDLE_INVALID;

            //check notification status before stop timer
            if(notification_status() == NOTIFICATION_BONDING) {
                led_action_stop();
            }
            app_timer_stop(user_action_delay_timer);
            
            //store private bondinfo to flash
            if(should_update_prv_bond_info == true) {
                should_update_prv_bond_info = false;
                bond_store_user_id();
            }

            //comes here after set flags
            if(get_global_should_trigger_ota_flag()) {
                trigger_ota_mode(); 
            }

            //link loss alert
            if(get_should_checkdev_loss_on_disconnect() && get_global_dev_loss_should_alert() && (get_global_dev_loss_alert_level() > NO_ALERT)) {
                set_global_dev_loss_should_alert(0);
                set_should_checkdev_loss_on_disconnect(false);

                if(rssi_average < DEV_LOSS_RESUME_RSSI_THRESHOLD) {
                    notification_start(NOTIFICATION_LOSE,(get_global_dev_loss_alert_level() == HIGH_ALERT));
                }
            }

            if(should_delete_bluetooth_bond_info == true) {
                should_delete_bluetooth_bond_info = false;
                ble_bondmngr_bonded_masters_delete();
            } else {
                // Since we are not in a connection and have not started advertising, store bonds
                err_code = ble_bondmngr_bonded_masters_store();
                APP_ERROR_CHECK(err_code);
            }

            ble_bondmngr_bonded_masters_check();
            
            // should never stop button event,for sensor should work
            //err_code = app_button_disable();
            //APP_ERROR_CHECK(err_code);

            advertising_start();
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(global_connect_handle,
                                                   BLE_GAP_SEC_STATUS_SUCCESS,
                                                   &m_sec_params);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            notify_steps_enable = false;
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT) {
                if (m_advertising_mode == BLE_SLEEP) {
                    m_advertising_mode = BLE_SLOW_ADV;
                }
                advertising_start();
            }
            break;

        case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server and Client timeout events.
            notify_steps_enable = false;

            set_sport_data_sync_process_send(false);
            
            err_code = sd_ble_gap_disconnect(global_connect_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            break;
        case BLE_GAP_EVT_RSSI_CHANGED:
            rssi_value = p_ble_evt->evt.gap_evt.params.rssi_changed.rssi;
            dev_loss_alert_handle(rssi_value,get_global_dev_loss_alert_level());
            break;

        default:
            break;
    }

    APP_ERROR_CHECK(err_code);
}


/**@brief Dispatches a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    if(p_ble_evt->header.evt_id == BLE_GAP_EVT_CONNECTED) {
        set_global_bluetooth_status(ShortConnectInterval);
    } else if(p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED) {
        set_global_bluetooth_status(NotWork); //disconnected and not start advertising
    }

    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    ble_bondmngr_on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_bas_on_ble_evt(&m_bas, p_ble_evt);
    on_ble_evt(p_ble_evt);
}


/**@brief BLE stack initialization.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
#ifdef NO_XTAL //represent no external 32.768k
    BLE_STACK_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_8000MS_CALIBRATION,
#else
    BLE_STACK_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM,
#endif
                           BLE_L2CAP_MTU_DEF,
                           ble_evt_dispatch,
                           true);
}


/**@brief Bond Manager module error handler.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void bond_manager_error_handler(uint32_t nrf_error)
{
    if(nrf_error == NRF_ERROR_NO_MEM) {
        if(is_bluetooth_connected()) { //blue tooth must connected, here double check
            should_delete_bluetooth_bond_info = true;
            sd_ble_gap_disconnect(global_connect_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        }
    } else {
        APP_ERROR_HANDLER(nrf_error);
    }

}

void ble_bondmngr_evt_handler (ble_bondmngr_evt_t * p_evt)
{
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"bond event: ");
    //simple_uart_putstring((const uint8_t *)"bond event: ");
#endif

    switch(p_evt->evt_type) {
        case BLE_BONDMNGR_EVT_NEW_BOND:
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO," BLE_BONDMNGR_EVT_NEW_BOND\r\n");
            //simple_uart_putstring((const uint8_t *)" BLE_BONDMNGR_EVT_NEW_BOND\r\n");
#endif

            break;
        case BLE_BONDMNGR_EVT_CONN_TO_BONDED_MASTER:
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO," BLE_BONDMNGR_EVT_CONN_TO_BONDED_MASTER\r\n");
            //simple_uart_putstring((const uint8_t *)" BLE_BONDMNGR_EVT_CONN_TO_BONDED_MASTER\r\n");
#endif

            break;
        case BLE_BONDMNGR_EVT_ENCRYPTED:
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO," BLE_BONDMNGR_EVT_ENCRYPTED\r\n");
            //simple_uart_putstring((const uint8_t *)" BLE_BONDMNGR_EVT_ENCRYPTED\r\n");
#endif

            break;
        case BLE_BONDMNGR_EVT_AUTH_STATUS_UPDATED:
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO," BLE_BONDMNGR_EVT_AUTH_STATUS_UPDATED\r\n");
            //simple_uart_putstring((const uint8_t *)" BLE_BONDMNGR_EVT_AUTH_STATUS_UPDATED\r\n");
#endif

            break;
        case BLE_BONDMNGR_EVT_BOND_FLASH_FULL:
#ifdef DEBUG_LOG

            LOG(LEVEL_INFO," BLE_BONDMNGR_EVT_BOND_FLASH_FULL\r\n");
            //simple_uart_putstring((const uint8_t *)" BLE_BONDMNGR_EVT_BOND_FLASH_FULL\r\n");
#endif

            break;
    }
}

/**@brief Bond Manager initialization.
 */
static void bond_manager_init(void)
{
    uint32_t            err_code;
    ble_bondmngr_init_t bond_init_data;
    bool                bonds_delete;

    bonds_delete = false;
    // Clear all bonded masters if the Bonds Delete button is pushed
    //err_code = app_button_is_pushed(BONDMNGR_DELETE_BUTTON_PIN_NO, &bonds_delete);
    //APP_ERROR_CHECK(err_code);

    // Initialize the Bond Manager
    bond_init_data.flash_page_num_bond     = FLASH_PAGE_BOND;
    bond_init_data.flash_page_num_sys_attr = FLASH_PAGE_SYS_ATTR;
    bond_init_data.evt_handler             = ble_bondmngr_evt_handler;
    bond_init_data.error_handler           = bond_manager_error_handler;
    bond_init_data.bonds_delete            = bonds_delete;

    err_code = ble_bondmngr_init(&bond_init_data);
    APP_ERROR_CHECK(err_code);
}


/**@brief Initialize Radio Notification event handler.
 */
static void radio_notification_init(void)
{
    uint32_t err_code;

    err_code = ble_radio_notification_init(NRF_APP_PRIORITY_HIGH,
                                           NRF_RADIO_NOTIFICATION_DISTANCE_4560US,
                                           ble_flash_on_radio_active_evt);
    APP_ERROR_CHECK(err_code);
}


/**@brief Initialize GPIOTE handler module.
 */
static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}


/**@brief Initialize button handler module.
 */
static void gpiote_int_init(void)
{
    if(true/*is_factory_test_done()*/) {
        static app_button_cfg_t buttons[] = {
                                                //{FUNCTION_BUTTON, false, NRF_GPIO_PIN_PULLUP ,function_button_event_handler ,false},
#ifdef FEATURE_SENSOR_LIS3DH
                                                {LIS3DH_INT1, ACTIVE_HIGH, NRF_GPIO_PIN_NOPULL ,LIS3DH_INT1_event_handler},
                                                {LIS3DH_INT2, ACTIVE_HIGH, NRF_GPIO_PIN_NOPULL ,LIS3DH_INT2_event_handler},
#endif
                                                //charger GPIO init
                                                //Charger: Plug In -> GPIO Low; Plug Out -> GPIO High
                                                //ChargingComplete: FULL -> GPIO Low; otherwise -> GPIO High
#ifdef CHARGER_CONNECTED_PIN
                                                {CHARGER_CONNECTED_PIN, ACTIVE_BOTH, NRF_GPIO_PIN_NOPULL ,function_charger_event_handler},
#endif
#ifdef CHARGER_CHARGING_PIN
                                                {CHARGER_CHARGING_PIN, ACTIVE_BOTH, NRF_GPIO_PIN_NOPULL ,function_chargingcomplete_event_handler},
#endif
                                            };
        APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, false);
    } else {
        static app_button_cfg_t buttons[] = {
                                                //charger GPIO init
                                                //Charger: Plug In -> GPIO Low; Plug Out -> GPIO High
                                                //ChargingComplete: FULL -> GPIO Low; otherwise -> GPIO High
#ifdef CHARGER_CONNECTED_PIN
                                                {CHARGER_CONNECTED_PIN, ACTIVE_BOTH, NRF_GPIO_PIN_NOPULL ,function_charger_event_handler},
#endif
#ifdef CHARGER_CHARGING_PIN
                                                {CHARGER_CHARGING_PIN, ACTIVE_BOTH, NRF_GPIO_PIN_NOPULL ,function_chargingcomplete_event_handler},
#endif
                                            };
        APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, false);
    }
    //GPIO_WAKEUP_BUTTON_CONFIG(FUNCTION_BUTTON);
}


/**@brief Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code;
    err_code = sd_app_event_wait();
    APP_ERROR_CHECK(err_code);
}

/* update the battery in main loop */
void battery_up(void * p_event_data, uint16_t event_size)
{
    (void)event_size;
    uint32_t err_code;
    uint8_t batt_lvl = *((uint8_t*)p_event_data);
    static int low_power_count = 0;

    err_code = ble_bas_battery_level_update(&m_bas, batt_lvl);
    if (
        (err_code != NRF_SUCCESS)
        &&
        (err_code != NRF_ERROR_INVALID_STATE)
        &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS)
        &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
    ) {
        APP_ERROR_HANDLER(err_code);
    }
    if( batt_lvl <= 0 ) {
        low_power_count ++;
        if( ( low_power_count > 3 ) && ( charger_status() == NoCharge ) ) {
            //TODO Low Power
            //1. BT disable
            err_code = sd_ble_gap_disconnect(global_connect_handle, BLE_HCI_REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF);
            APP_ERROR_HANDLER(err_code);
            //2. Sync Data
            low_power_info_store();
            //3. System Off
            power_system_off();
        }
    } else {
        low_power_count = 0;
    }
    
    //update charging status, when battery level is changed.
    update_charging_status(0);
}


void reset_check(void)
{

    uint32_t page_num;

    LOG(LEVEL_INFO,"RESETREAS:%x; POWER_RESETREAS_RESETPIN_Msk:%x",NRF_POWER->RESETREAS,POWER_RESETREAS_RESETPIN_Msk);
    if(NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk) {
        LOG(LEVEL_WARNING,"Must Erase All User Data!!!!");
        NRF_POWER->RESETREAS = POWER_RESETREAS_RESETPIN_Msk;

#ifndef INTERNAL_DEV
        //clear data block
        page_num = DATA_BLOCK_START_PAGE;
        for( ; page_num < (DATA_BLOCK_START_PAGE + PAGE_NUM_FOR_DATA); page_num ++) {
            ble_flash_page_erase(page_num);
        }

        //clear config block
        page_num = CONFIG_BLOCK_START_PAGE;
        for( ; page_num < (CONFIG_BLOCK_START_PAGE + PAGE_NUM_FOR_CONFIG); page_num++ ) {
            if(page_num != FLASH_PAGE_SN_FACTORY_FLAG) { // SN page should not be erased
                ble_flash_page_erase(page_num);
            }
        }
#endif

        /*        if(!charger_connected()){
                    hal_acc_PowerDown();
                    power_wfi();
                }
        */
    }
    LOG(LEVEL_INFO,"RESETREAS:%x; POWER_RESETREAS_RESETPIN_Msk:%x",NRF_POWER->RESETREAS,POWER_RESETREAS_RESETPIN_Msk);


    //notice wakeup
    nrf_gpio_pin_clear(BAIDU_LED_0);
    nrf_delay_ms(400);
    nrf_gpio_pin_set(BAIDU_LED_0);
}

void gpio_init()
{
    uint32_t i = 0;
    for(i = 0; i< 32 ; ++i ) {
        NRF_GPIO->PIN_CNF[i] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                               | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                               | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                               | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                               | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);
    }
}

/**@brief Function for configuring the Low Frequency Clock on the nRF51 Series.
 *        It will return when the clock has started.
 *
 */
static void lfclk_config(void)
{
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    //  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    //  {
    //  }

    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
}

void init1()
{
#if defined(DEBUG_LOG) || defined (DEBUG_ACC) || defined (DEBUG_PHILL)
    simple_uart_config(RTS_PIN_NUMBER, TX_PIN_NUMBER, CTS_PIN_NUMBER, RX_PIN_NUMBER, HWFC);
    LOG(LEVEL_INFO," init1 uart inited\n");
    //simple_uart_putstring((const uint8_t *)"init1 uart inited\r\n");
#endif

    leds_init();
    motor_init();
    lfclk_config();
    timer_init();
    scheduler_init();
    charger_init();
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"charger_init inited\r\n");
    // simple_uart_putstring((const uint8_t *)"charger_init inited\r\n");
#endif

#ifndef ADC_WRONG

    battery_adc_dev_init();
    battery_measure_init();
#endif
#ifndef USE_DVK_6310

    hal_acc_init();
#endif
}

void init2()
{
    uint32_t err_code;
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"init2 inited\r\n");
    //simple_uart_putstring((const uint8_t *)"init2 uart inited\r\n");
#endif

    wdt_init();
    timers_create();
    gpiote_init();
    system_clock_init();

    //init ble stack
    bond_manager_init();
    ble_stack_init();
    gap_params_init();
    advertising_init(BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE);
    services_init();
    conn_params_init();
    sec_params_init();
    radio_notification_init();
    set_global_bluetooth_status(NotWork);

    gpiote_int_init();
    charger_framework_init();

    //init bluetooth communicate protocol
    bluetooth_l0_init();
    err_code = bond_read_user_id();

    if(err_code == NRF_SUCCESS) {
        set_device_has_bonded(true);
        LOG(LEVEL_INFO,"bond crc check success\r\n");
    } else {
        set_device_has_bonded(false);
        LOG(LEVEL_ERROR,"bond check fail!!!!\r\n");
    }

#ifndef USE_DVK_6310

    hal_acc_enable();

#endif
}

#ifdef FEATURE_PRE_INIT
/** RTC0 interrupt handler.
 * Triggered on TICK and COMPARE0 match.
 */
void RTC0_IRQHandler()
{
    NRF_RTC0->EVENTS_COMPARE[0] = 0;
    NRF_RTC0->TASKS_STOP = 1;
    NRF_RTC0->INTENCLR = RTC_INTENSET_COMPARE0_Msk;
    NVIC_DisableIRQ(RTC0_IRQn);
}

#define LFCLK_FREQUENCY           (32768UL)                 /*!< LFCLK frequency in Hertz, constant */
#define RTC_FREQUENCY             (8UL)                     /*!< required RTC working clock RTC_FREQUENCY Hertz. Changable */
#define COMPARE_COUNTERTIME       (1UL)                     /*!< Get Compare event COMPARE_TIME seconds after the counter starts from 0 */
#define COUNTER_PRESCALER         ((LFCLK_FREQUENCY/RTC_FREQUENCY) - 1)  /* f = LFCLK/(prescaler + 1) */

/** Configures the RTC with TICK for 100Hz and COMPARE0 to 10 sec
 */
static void wakeup_rtc_config(void)
{
    NVIC_EnableIRQ(RTC0_IRQn);                                 // Enable Interrupt for the RTC in the core
    NRF_RTC0->PRESCALER = COUNTER_PRESCALER;                   // Set prescaler to a TICK of RTC_FREQUENCY
    NRF_RTC0->CC[0] = COMPARE_COUNTERTIME * RTC_FREQUENCY;     // Compare0 after approx COMPARE_COUNTERTIME seconds

    // Enable COMPARE0 event and COMPARE0 interrupt:
    //  NRF_RTC0->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
    NRF_RTC0->INTENSET = RTC_INTENSET_COMPARE0_Msk;
    NRF_RTC0->TASKS_START = 1;
}

void pre_init()
{
    //NRF_GPIO->DIRSET = 0;
    lfclk_config();
    wakeup_rtc_config();
    __WFI();
}
#endif

/**@brief Application main function.
 */
void start_click_algorithm(void);


int main(void)
{
    // 0. init all gpio to reduce power consumption
    gpio_init();

    // 1. wait for power suply to be stable
#ifdef FEATURE_PRE_INIT

    pre_init();
#endif

    // 2. first stage initialize
    init1();

#ifndef ADC_WRONG

    // 3. pre-charging
    if(!voltage_detect_n_precharging()) {
        power_system_off();
    }
#endif
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"voltage_detect_n_precharging ed\n");
    //  simple_uart_putstring((const uint8_t *)"\t\t[main]voltage_detect_n_precharging ed\r\n");
#endif

    // 4. Bootup Check
    bootup_check();
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"bootup_check ed\n");
    // simple_uart_putstring((const uint8_t *)"\t\t[main]bootup_check ed\r\n");
#endif

    // 5. WFI
    reset_check();
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"reset_check ed\n");
    //simple_uart_putstring((const uint8_t *)"\t\t[main]reset_check ed\r\n");
#endif

    // 6. second stage initialize
    init2();

    //read time and erase page before advertising start
    uint32_t err_code;
    err_code = restore_backup_info_from_flash();
    if(err_code == NRF_SUCCESS) {

        restore_SleepSettingsFromFlash();
        // if has profile, start step counter after set time
        if(check_has_bonded()) {
#ifdef DEBUG_LOG
            LOG(LEVEL_INFO," restart_health_algorithm \n");
#endif

            load_alarm();
            load_user_profile();
            load_daily_target();
            restart_health_algorithm();
        }

    }
#ifdef DEBUG_LOG
    else {
        LOG(LEVEL_INFO," no time read from flash\n");
        //   simple_uart_putstring((const uint8_t *) "\t\t[main] no time read from flash\r\n");
    }
#endif
    
    check_need_erase_sleep_page_for_when_boot();
    check_need_erase_sport_page_for_when_boot();
    // check the format of data in flash
    check_sports_data_in_flash();
    check_sleep_data_in_flash();


    start_click_algorithm();

    sensor_timers_start();
  
    /**********************************************************************************
    * check page  maybe used in other place
    ***********************************************************************************/
    /**********************************************************************************
    ble_flash_page_erase(FLASH_PAGE_ERROR_LOG);
    nrf_delay_ms(1000);
    uint16_t i = 0;
    uint32_t * log_address = 0;

    ble_flash_page_addr(FLASH_PAGE_ERROR_LOG,&log_address);

    if(log_address) {
        for(i=0 ; i < (BLE_FLASH_PAGE_SIZE /sizeof(uint32_t)) ; ++i )
        {
            if(*(log_address + i) != 0xFFFFFFFF) {
                LOG(LEVEL_WARNING, "get none 0xFFFFFFFF value \r\n");
                break;
            }
        }

        if(i == (BLE_FLASH_PAGE_SIZE /sizeof(uint32_t))) {
            LOG(LEVEL_WARNING,"check success \r\n");
        }

        for(i=0 ; i < (BLE_FLASH_PAGE_SIZE /sizeof(uint32_t)) ; ++i ) {
            ble_flash_word_write((log_address + i),0);
        }

        for(i=0 ; i < (BLE_FLASH_PAGE_SIZE /sizeof(uint32_t)) ; ++i )
        {
            if(*(log_address + i) != 0) {
                LOG(LEVEL_WARNING, "get none 0x00000000 value \r\n");
                break;
            }
        }


        if(i == (BLE_FLASH_PAGE_SIZE /sizeof(uint32_t))) {
            LOG(LEVEL_WARNING,"check write 0 success \r\n");
        }

    }
    ***********************************************************************************/

    // 7. Working
    advertising_start();
#ifdef DEBUG_LOG

    LOG(LEVEL_INFO,"advertising_start ed\n");
    //  simple_uart_putstring((const uint8_t *)"\t\t[main]advertising_start ed\r\n");
#endif
    //always enable button event
    app_button_enable();
//    battery_measure_timer_start(BATTERY_LEVEL_MEAS_INTERVAL);

    wdt_start();
    // Enter main loop
    for (;;) {
        app_sched_execute();

        wdt_feed();
        power_manage();
    }
}

/**
 * @}
 */
