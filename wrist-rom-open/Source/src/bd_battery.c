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
#include "config.h"
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble_stack_handler.h"
#include "ble_bas.h"
#include "bd_battery.h"
#include "bd_charging.h"
#include "app_util.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "board_config_pins.h"
#include "simple_uart.h"


static uint8_t percentage_batt_lvl = 0;
uint16_t g_battery_voltage_mv = 0;
extern ble_bas_t    m_bas;
static app_sched_event_handler_t ad_event_handle = NULL;
#define VOLTAGE_AVG_NUM 6
static uint32_t battery_voltage_arrary[VOLTAGE_AVG_NUM]={0};
static uint8_t battery_voltage_arrary_index = 0;

static const float VoltageTable[] =
    {
        4142.2,4011.2,3912.9,3831.0,3774.9,3734.4,3688.4,3605.3
    };
static const uint8_t PercentageTable[] =
    {
        100,86,72,58,43,29,15,1
    };
static app_timer_id_t m_battery_timer_id;                               /**< Battery timer. */

uint8_t cal_percentage(uint16_t volatage)
{
    uint8_t length = ARRAY_LEN(VoltageTable);
    uint8_t i  = 0;
    
    if(charger_status() == ChargingComplete){
        return 100;
    }
    
    //find the first value which is < volatage
    for(i = 0; i< length ; ++i) {

        if(volatage >= VoltageTable[i]) {
            break;
        }
    }

    if( i>= length) {
        return 0;
    }

    if(i == 0) {
        return 100;
    }
    
    return (volatage - VoltageTable[i])/((VoltageTable[i-1] - VoltageTable[i])/(PercentageTable[i-1] - PercentageTable[i])) + PercentageTable[i];

}

/**@brief ADC interrupt handler.
 * @details  This function will fetch the conversion result from the ADC, convert the value into
 *           percentage and send it to peer.
 */
void ADC_IRQHandler(void)
{
    uint32_t adc_result = 0;
    uint32_t batt_lvl_in_milli_volts;
    if (NRF_ADC->EVENTS_END != 0) {
        uint32_t    err_code;

        NRF_ADC->EVENTS_END     = 0;
        adc_result              = NRF_ADC->RESULT;
        NRF_ADC->TASKS_STOP     = 1;

        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result);
#ifdef DEBUG_PHILL
        LOG(LEVEL_INFO,"batt_lvl_in_milli_volts:[%d]\n",batt_lvl_in_milli_volts);
#endif

        //when charging, battery voltage need adjust
        if(charger_status()!= NoCharge) {
#if 1
            if( battery_voltage_arrary[0] == 0 ){
                battery_voltage_arrary_index = 0;
                for( int i = 0; i < VOLTAGE_AVG_NUM; i++ ){
                    battery_voltage_arrary[i] = batt_lvl_in_milli_volts;
                }
            }else{
                battery_voltage_arrary[battery_voltage_arrary_index] = batt_lvl_in_milli_volts;
                battery_voltage_arrary_index = ( battery_voltage_arrary_index + 1 )%VOLTAGE_AVG_NUM;
                batt_lvl_in_milli_volts = 0;
                for( int i = 0; i < VOLTAGE_AVG_NUM; i++ ){
                    batt_lvl_in_milli_volts += battery_voltage_arrary[i];
                }
                batt_lvl_in_milli_volts /= VOLTAGE_AVG_NUM;
            }
#endif
#ifdef DEBUG_PHILL
            LOG(LEVEL_INFO,"batt_lvl_in_milli_volts avg:[%d]\n",batt_lvl_in_milli_volts);
#endif
            batt_lvl_in_milli_volts -= get_battery_voltage_adjustment();
#ifdef DEBUG_PHILL
            LOG(LEVEL_INFO,"batt_lvl_in_milli_volts after adjustment:[%d]\n",batt_lvl_in_milli_volts);
#endif
        }else{
            battery_voltage_arrary[0] = 0;
        }
        g_battery_voltage_mv = batt_lvl_in_milli_volts & 0xffff;
        percentage_batt_lvl = cal_percentage(batt_lvl_in_milli_volts & 0xffff);

        if(ad_event_handle) {
            err_code = app_sched_event_put(&percentage_batt_lvl,1,ad_event_handle);
            APP_ERROR_CHECK(err_code);
        }
    }
    NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Disabled;
}

/**
 * Read battery ADC value once, be sure call this when ADC_IRQn disabled
 *
 */
void battery_adc_read_once(uint16_t* batt_lvl_in_milli_volts)
{
    uint32_t adc_result = 0;

    battery_start();
    nrf_delay_ms(10);
    while( !(NRF_ADC->EVENTS_END) ) {
        nrf_delay_ms(10);
    }

    NRF_ADC->EVENTS_END     = 0;
    adc_result              = NRF_ADC->RESULT;
    NRF_ADC->TASKS_STOP     = 1;

    *batt_lvl_in_milli_volts = ((uint32_t)(ADC_RESULT_IN_MILLI_VOLTS(adc_result)) & 0xffff);
    //when charging, battery voltage need adjust
    if(charger_status()!= NoCharge) {
        *batt_lvl_in_milli_volts -= BATTERY_VOLTAGE_ADJUSTMENT;
    }
}

static void battery_adc_interrupt_disable(void)
{
    NRF_ADC->INTENSET   = ADC_INTENSET_END_Disabled;
    NVIC_DisableIRQ(ADC_IRQn);
}

/*********************************************************************
* ADC read once for charging detect, which returns raw data
**********************************************************************/
void battery_adc_interrupt_init(void);
void battery_adc_read_once_for_charging_detect(uint16_t * batt_lvl_in_milli_volts)
{
    uint32_t adc_result = 0;
    
    //disable adc irq
    battery_adc_interrupt_disable();
    //adc stop
    NRF_ADC->EVENTS_END     = 0;
    NRF_ADC->TASKS_STOP     = 1;

    battery_start();
    nrf_delay_ms(10);
    while( !(NRF_ADC->EVENTS_END) ) {
        nrf_delay_ms(10);
    }

    NRF_ADC->EVENTS_END     = 0;
    adc_result              = NRF_ADC->RESULT;
    NRF_ADC->TASKS_STOP     = 1;

    *batt_lvl_in_milli_volts = ((uint32_t)(ADC_RESULT_IN_MILLI_VOLTS(adc_result)) & 0xffff);

    //reinit interrupt
    battery_adc_interrupt_init();
}


void battery_start(void)
{
    //start evey time use ADC
    NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Enabled;

    NRF_ADC->EVENTS_END  = 0;    // Stop any running conversions.
    NRF_ADC->TASKS_START = 1;
}

/**************************************************************************************
* config: 
*       10bit resolution, 1.2V reference, no internal prescal
***************************************************************************************/

void battery_adc_dev_init(void)
{
    // 1. Delay for ADC stable
    //    nrf_delay_ms(500);


    // Configure ADC
    NRF_ADC->CONFIG     = BATTERY_ADC_RESOLUTION << ADC_CONFIG_RES_Pos    |
                          ADC_CONFIG_INPSEL_AnalogInputNoPrescaling  << ADC_CONFIG_INPSEL_Pos |
                          ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos |
                          ADC_INPUT_CHANNEL << ADC_CONFIG_PSEL_Pos;

    NRF_ADC->EVENTS_END = 0;
}

void battery_adc_interrupt_init(void)
{
    NRF_ADC->INTENSET   = ADC_INTENSET_END_Msk;
    NVIC_SetPriority(ADC_IRQn,1);
    NVIC_EnableIRQ(ADC_IRQn);
}


/**@brief Battery measurement timer timeout handler.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    battery_start();
}

void register_battery_up_callback(app_sched_event_handler_t handle)
{
#ifdef DEBUG_LOG
//    LOG(LEVEL_INFO,"register_battery_up_callback \r\n");
    //  simple_uart_putstring((const uint8_t *)"register_battery_up_callback \r\n");
#endif

    ad_event_handle = handle;
    percentage_batt_lvl = cal_percentage(g_battery_voltage_mv);
    if(ad_event_handle) {
        (*ad_event_handle)(&percentage_batt_lvl,1);
    }
}

void battery_measure_init()
{
    //        uint16_t batt_lvl_in_milli_volts;
    uint32_t err_code;

    if(!g_battery_voltage_mv)
        battery_adc_read_once(&g_battery_voltage_mv);

    NRF_ADC->ENABLE     = ADC_ENABLE_ENABLE_Disabled;
    battery_adc_interrupt_init();

    // Create battery timer
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}


/**************************************************************************
* get current battery percentage from battery service
****************************************************************************/
uint8_t get_battery_power_percentage(void)
{
    extern ble_bas_t    m_bas;
    return m_bas.battery_level_last;
}

void battery_measure_timer_start(uint32_t interval)
{
    uint32_t err_code;

    // Start battery timer
    err_code = app_timer_start(m_battery_timer_id, interval, NULL);
    APP_ERROR_CHECK(err_code);
}

void battery_measure_timer_stop()
{
    uint32_t err_code;

    // Stop battery timer
    err_code = app_timer_stop(m_battery_timer_id);
    APP_ERROR_CHECK(err_code);

}

/**@brief Check voltage and charger status
 *
 * @details First check if the voltage is higher enough, if not, enter cherging state while
 *          charger connected utill the voltage is enough, otherwise return false
 *
 * @return true if the voltage is higher enough
 *         false if the voltage is not higher enough and charger is not connected 
 */

uint8_t voltage_detect_n_precharging(void)
{
    uint8_t ret = false;
    battery_measure_timer_start(CHARGER_CHECK_INTERVAL_MS);
    do {
        LOG(LEVEL_DEBUG,"g_battery_voltage_mv:%d\n",g_battery_voltage_mv);
        if( MIN_BAT_MV <= g_battery_voltage_mv ) {
            ret = true;
            break;
        }

        __WFI();
        app_sched_execute();
    } while(charger_connected());

    battery_measure_timer_stop();
    return ret;
}
/**
 * @}
 */
