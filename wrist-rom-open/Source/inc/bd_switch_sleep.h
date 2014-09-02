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
#ifndef _BD_SWITCH_SLEEP_H_
#define _BD_SWITCH_SLEEP_H_
#include <stdbool.h>
#include <stdint.h>
#include "step_counter.h"
#define SETTING_SLEEP_MODE 0
#define SETTING_STEP_MODE  1
/**
 * handle 2 Double-click event, switch between normal mode and sleeping mode
 *
 * 
 * @note 
 *
 * @param spi_base_address  register base address of the selected SPI master module
 * @param transfer_size  number of bytes to transmit/receive over SPI master
 * @param tx_data pointer to the data that needs to be transmitted
 * @param rx_data pointer to the data that needs to be received
 * @return
 * @retval true if transmit/reveive of transfer_size were completed.
 * @retval false if transmit/reveive of transfer_size were not complete and tx_data/rx_data points to invalid data.
 */
void reset_sleep_setting(void);

SleepData_U * get_last_sleep_setting_event(void);
SleepHead_t * get_last_sleep_setting_event_head(void);
void send_last_time_sleep_mode(void);

void two_Double_tap_handle(void);
void update_sleep_status(void);

void restore_SleepSettingsFromFlash(void);
bool save_sleep_settings_to_flash(void);
void update_sleep_setting_head(void);
void reset_sleep_setting_head(void);
void set_curr_sleep_flag(bool sleeping);
bool get_curr_sleep_flag(void);
uint16_t sleep_setting_count(void);
bool save_sleep_settings_to_flash(void);
bool send_sleep_setting_data(void);
void send_sleep_setting_data_cb(void);
#endif
