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
#include "board_config_pins.h"
/**************************************************************************
* Charging state defination
***************************************************************************/
typedef enum {
    InCharging = 0,       /* Device connect to the plugs*/
    ChargingComplete,      /* Show charging complete & still connect to plugs*/
    NoCharge           /* Show not connect to plugs*/
}Charging_State;

void charger_init(void);
void charger_framework_init(void);
Charging_State charger_status(void);
uint8_t charger_connected(void);

void update_charging_status(uint8_t refresh);
uint8_t get_battery_voltage_adjustment(void);
void function_charger_event_handler(uint8_t pin_no);
void function_chargingcomplete_event_handler(uint8_t pin_no);

