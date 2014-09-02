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
#include "config.h"
#include "bd_charging.h"
#include "bd_battery.h"
#include "nrf_error.h"
#include "app_timer.h"
#include "bd_led_flash.h"
#include "bd_interaction.h"
#include "bd_wall_clock_timer.h"
#include "nrf_gpio.h"
#include "app_button.h"

#ifdef DEBUG_LOG
#include "simple_uart.h"
#endif


//#define CHARGING_INTERVAL         APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)
//#define CHARGE_COMPLETE_INTERVAL    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)
//#define TIME_INTERVAL          APP_TIMER_TICKS(500, 0)

//static  app_timer_id_t               m_charing_period_id;

//used to reflect the global charging state
Charging_State global_device_charging_state = NoCharge;//ChargingComplete;//NoCharge;

static uint8_t battery_level = 0;

//for Test
//static void test_all_led_flash(int count);
//For internal using
//static void charging_flash_battery_start(void);
//static void charging_flash_battery_stop(void);
static void show_charging_percentage(void * p_context);

static void show_charging_percentage(void * p_context)
{
    Charging_State * state = (Charging_State *)p_context;
    switch(*state) {
        case InCharging: {
                //get battery percentage and show
                uint8_t batt = get_battery_power_percentage();
                uint8_t m = ( batt / 20 ) + 1;
                if( battery_level == m ){ 
                    break;
                }
                battery_level = m;
                
                if(batt < 20) {
                    led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_20,0);
                //    led_action_control_with_interval(NOTIFICATION_CHARGING,STEP_PERCENTAGE_20,5,TIME_INTERVAL);
                } else if(batt<40) {
                    led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_40,0);
                //    led_action_control_with_interval(NOTIFICATION_CHARGING,STEP_PERCENTAGE_40,5,TIME_INTERVAL);
                } else if(batt<60) {
                    led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_60,0);
                //    led_action_control_with_interval(NOTIFICATION_CHARGING,STEP_PERCENTAGE_60,5,TIME_INTERVAL);
                } else if(batt<80) {
                    led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_80,0);
                //    led_action_control_with_interval(NOTIFICATION_CHARGING,STEP_PERCENTAGE_80,5,TIME_INTERVAL);
                } else if(batt<100) {
                    led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_100,0);
                //    led_action_control_with_interval(NOTIFICATION_CHARGING,STEP_PERCENTAGE_100,5,TIME_INTERVAL);
                } else if(batt==100) {
                    led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_FULL,0);
                //    led_action_control_with_interval(NOTIFICATION_CHARGING,PERCENTAGE_100,10,TIME_INTERVAL);
                }
            }
            break;
        case ChargingComplete:
            led_action_control(NOTIFICATION_CHARGING,PERCENTAGE_FULL,0);
        //    led_action_control_with_interval(NOTIFICATION_CHARGING,ALL_FLASH,1,TIME_INTERVAL);
            break;
        default:
            break;
    }
#ifdef DEBUG_LOG

    {
        //    char str[32];
        //    UTCTimeStruct * tm = get_wall_clock_time();
        //    LOG(LEVEL_INFO,"RTC: [%d/%02d/%02d %02d:%02d:%02d]\r\n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
        // snprintf(str,sizeof(str),"RTC: [%d/%02d/%02d %02d:%02d:%02d]\r\n",tm->year,tm->month,tm->day,tm->hour,tm->minutes,tm->seconds);
        // simple_uart_putstring((const uint8_t *)str);
    }
#endif
}

uint8_t get_battery_voltage_adjustment()
{
    return BATTERY_VOLTAGE_ADJUSTMENT - battery_level*5;
}

void update_charging_status( uint8_t refresh )
{
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"global_device_charging_state: [%d]\r\n",global_device_charging_state);
#endif
    
    if( refresh ){
        battery_level = 0;
    }
    
    if( global_device_charging_state == NoCharge ) {
        return;
    } else {
        show_charging_percentage(&global_device_charging_state);
    }
}
/*
static void test_all_led_flash(int count){
 for(int i=0;i<count;i++){
    nrf_gpio_port_clear((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT0, 0xE0);//BAIDU_LED_0, BAIDU_LED_1, BAIDU_LED_2
    nrf_gpio_port_clear((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT2, 0x0C);//BAIDU_LED_3, BAIDU_LED_4
    nrf_delay_ms(1000);
    nrf_gpio_port_set((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT0, 0xE0);//BAIDU_LED_0, BAIDU_LED_1, BAIDU_LED_2
    nrf_gpio_port_set((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT2, 0x0C);//BAIDU_LED_3, BAIDU_LED_4
    nrf_delay_ms(1000);
 }
}
*/
#ifdef CHARGER_CONNECTED_PIN
void function_charger_event_handler(uint8_t pin_no)
{
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"pin_no: [%d]\r\n",pin_no);
#endif
    if(pin_no != CHARGER_CONNECTED_PIN) {
        return;
    }
    battery_level = 0;
    global_device_charging_state = charger_status();
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"global_device_charging_state: [%d]\r\n",global_device_charging_state);
#endif
    if( global_device_charging_state == NoCharge ) {
        if( notification_status() == NOTIFICATION_CHARGING ){
            led_action_stop();
        }
//        charging_flash_battery_stop();
    } else {
        //show voltage
        global_device_charging_state = InCharging;
//        charging_flash_battery_start();
        show_charging_percentage(&global_device_charging_state);
    }
}
#endif

#ifdef CHARGER_CHARGING_PIN
void function_chargingcomplete_event_handler(uint8_t pin_no)
{
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"pin_no: [%d]\r\n",pin_no);
#endif
    if(pin_no != CHARGER_CHARGING_PIN) {
        return;
    }

    global_device_charging_state = charger_status();
#ifdef DEBUG_LOG
    LOG(LEVEL_INFO,"global_device_charging_state: [%d]\r\n",global_device_charging_state);
#endif
#ifdef CHARGER_CONNECTED_PIN

    if( global_device_charging_state != NoCharge ) {
    //    charging_flash_battery_start();
    }
#else
    if( global_device_charging_state == NoCharge ) {
        charging_flash_battery_stop();
    } else {
        charging_flash_battery_start();
    }
#endif
}
#endif

/*************************************************************************
* charger init
**************************************************************************/
void charger_init(void)
{
#ifdef CHARGER_CONNECTED_PIN
    GPIO_PIN_CONFIG(CHARGER_CONNECTED_PIN,
                    GPIO_PIN_CNF_DIR_Input,
                    GPIO_PIN_CNF_INPUT_Connect,
                    GPIO_PIN_CNF_PULL_Disabled,
                    GPIO_PIN_CNF_DRIVE_S0S1,
                    GPIO_PIN_CNF_SENSE_Low);
#endif
#ifdef CHARGER_CHARGING_PIN

    GPIO_PIN_CONFIG(CHARGER_CHARGING_PIN,
                    GPIO_PIN_CNF_DIR_Input,
                    GPIO_PIN_CNF_INPUT_Connect,
                    GPIO_PIN_CNF_PULL_Disabled,
                    GPIO_PIN_CNF_DRIVE_S0S1,
                    GPIO_PIN_CNF_SENSE_Low);
#endif
}
void charger_framework_init(void)
{
    //Get Init Charging Status
    global_device_charging_state = charger_status();

    //Start Charger Timer ,when Pluged in.
    if( global_device_charging_state != NoCharge ) {
//        charging_flash_battery_start();
        show_charging_percentage(&global_device_charging_state);
    }
#ifdef CHARGER_CONNECTED_PIN
    //Wakeup
    GPIO_WAKEUP_BUTTON_CONFIG(CHARGER_CONNECTED_PIN); //charger wakeup
#endif
}

/*************************************************************************
* charger status
* 2 -> NoCharge; 1 -> ChargingComplete£» 0 -> inCharging
**************************************************************************/
Charging_State charger_status(void)
{
    uint8_t charger = 0;
#ifdef CHARGER_CHARGING_PIN

    uint8_t charging = 0;
#endif
#ifdef CHARGER_CONNECTED_PIN

    charger = !(nrf_gpio_pin_read(CHARGER_CONNECTED_PIN) & 0xFF);
#else

    charger = (nrf_gpio_pin_read(CHARGER_CHARGING_PIN) & 0xFF);
#endif
#ifdef CHARGER_CHARGING_PIN

    charging = !(nrf_gpio_pin_read(CHARGER_CHARGING_PIN) & 0xFF);
#endif

    if(charger) {
#ifdef CHARGER_CHARGING_PIN
        if(charging)
            return InCharging;
        return ChargingComplete;
#else

        return InCharging;
#endif

    } else {
        return NoCharge;
    }
}

uint8_t charger_connected(void)
{
    if( charger_status() != NoCharge )
        return 1;
    return 0;
}

/*************************************************************************
* Show charging percentage
* Every three seconds flash the led according to the power percentage 
* of the battery
**************************************************************************/
/*
static void charging_flash_battery_start(void)
{
    //single thread singlton
    static bool m_timer_init = false;
    uint32_t err_code;

    if(!m_timer_init) {
        // Create charging timer
        err_code = app_timer_create(&m_charing_period_id,
                                    APP_TIMER_MODE_REPEATED,
                                    show_charging_percentage);
        if(err_code != NRF_SUCCESS) {
            return; //how to handle this error?
        }

        m_timer_init = true;
    }

    //stop current timer before start it
    app_timer_stop(m_charing_period_id);

    //timer has been created
    if(global_device_charging_state == InCharging) {
        err_code = app_timer_start(m_charing_period_id, CHARGING_INTERVAL, &global_device_charging_state);
    } else if(global_device_charging_state == ChargingComplete) {
        err_code = app_timer_start(m_charing_period_id, CHARGE_COMPLETE_INTERVAL, &global_device_charging_state);
    }
}

static void charging_flash_battery_stop(void)
{
    app_timer_stop(m_charing_period_id);
}
*/
