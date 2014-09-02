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

#ifndef BATTERY_H__
#define BATTERY_H__
#include "app_scheduler.h"

#define ADC_INPUT_CHANNEL  ADC_CONFIG_PSEL_AnalogInput3
#define ADC_REF_VOLTAGE_IN_MILLIVOLTS        1200                                     /**< Reference voltage (in  milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION         1                                         /**< The ADC is configured to use VDD with no prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define ADC_HW_PRE_SCALING_COMPENSATION      4

#define BATTERY_ADC_RESOLUTION  (ADC_CONFIG_RES_10bit)
#define BATTERY_ADC_DIV         (1023)

#define MIN_BAT_MV (3600)
#define CHARGER_CHECK_INTERVAL_MS (APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER))

/**@brief Macro to convert the result of ADC conversion in millivolts.
 *
 * @param[in]  ADC_VALUE   ADC result.
 * @retval     Result converted to 0.1 millivolts.
 */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS)  * ADC_PRE_SCALING_COMPENSATION * ADC_HW_PRE_SCALING_COMPENSATION)/ BATTERY_ADC_DIV)

/**@brief Function to make the ADC start a battery level conversion
 */
void battery_start(void);
void battery_measure_init(void);
void battery_measure_timer_start(uint32_t interval);
void register_battery_up_callback(app_sched_event_handler_t handle);
void battery_adc_dev_init(void);

uint8_t get_battery_power_percentage(void);
void battery_adc_read_once(uint16_t* batt_lvl_in_milli_volts);
void battery_adc_read_once_for_charging_detect(uint16_t * batt_lvl_in_milli_volts);
uint8_t voltage_detect_n_precharging(void);
#endif // BATTERY_H__

/** @} */
/** @endcond */
