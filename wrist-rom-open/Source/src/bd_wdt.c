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
#include "nrf51.h"
#include "nrf51_bitfields.h"

#define RELOAD_COUNT (32768*60*3-1)    //3 minutes

void wdt_init(void)
{
    NRF_WDT->TASKS_START = 0;
    NRF_WDT->CRV = RELOAD_COUNT;
    NRF_WDT->CONFIG =
        WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos |
        WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos;
    NRF_WDT->RREN = WDT_RREN_RR0_Enabled << WDT_RREN_RR0_Pos;
}

void wdt_start(void)
{
    NRF_WDT->TASKS_START = 1;
}

void wdt_feed(void)
{
    if(NRF_WDT->RUNSTATUS & WDT_RUNSTATUS_RUNSTATUS_Msk)
        NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void wdt_stop(void)
{
    NRF_WDT->TASKS_START = 0;
}
