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
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "board_config_pins.h"
#include "hal_acc.h"
#include "bd_charging.h"

void power_wfi()
{
#ifdef CHARGER_CONNECTED_PIN
    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CONNECTED_PIN);//charger wakeup
#else

    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CHARGING_PIN);//charger wakeup
#endif

    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
    NVIC_EnableIRQ(GPIOTE_IRQn);
    while(true) {
        __WFI();
        if(charger_connected()) {
            break;
        }
    }

    //wakeup, disable irq
    NVIC_DisableIRQ(GPIOTE_IRQn);
}

void power_system_off()
{
#ifdef CHARGER_CONNECTED_PIN
    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CONNECTED_PIN);//charger wakeup
#else

    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CHARGING_PIN);//charger wakeup
#endif
    // close sensor
    hal_acc_PowerDown();
    //enter system off
    NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter;
}

void sd_system_off()
{
#ifdef CHARGER_CONNECTED_PIN
    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CONNECTED_PIN);//charger wakeup
#else

    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CHARGING_PIN);//charger wakeup
#endif
    // close sensor
    hal_acc_PowerDown();
    //enter system off
    sd_power_system_off();
}
