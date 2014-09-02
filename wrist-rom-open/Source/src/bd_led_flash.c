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
#include "config.h"
#include "board_config_pins.h"
#include "bd_led_flash.h"
#include "bd_interaction.h"
#include "bd_charging.h"
#include "app_scheduler.h"
#include "app_error.h"
#include "app_timer.h"
#include "nrf_gpio.h"

static LED_FLASH_TYPE current_flash_style = FLASH_NONE;
static uint16_t flash_loop_times = 0;
static uint16_t current_flash_loop = 0;

/* pin ordered by layout order */
const uint8_t led_pin_array[LED_NUMBER] =
    {
        BAIDU_LED_0,BAIDU_LED_1,BAIDU_LED_2,BAIDU_LED_3,BAIDU_LED_4
    };
/* defines the polarity of pins, corresponding to order defined above */
/*
  The followding bit map represents the corresponding led 
  |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|   
  | x| x| x| x| x| x| x| x| x| x| x| x| x| x| x| x|  
 
  x=0 represent: 0 drives the led light
  x=1 represent: 1 drives the led light
*/
#ifdef USE_DVK_6310

const uint16_t led_polarity_bit_array = 0x007F;
#else

const uint16_t led_polarity_bit_array = 0x0000;
#endif

/* flash method */
const uint16_t all_flash[]    =
    {
        0x001f,0x0000
    };
const uint16_t serial_flash[]   =
    {
        0x0001,0x0002,0x0004,0x0008,0x0010
    };
const uint16_t serial_close[]   =
    {
        0x001e,0x001d,0x001b,0x0017,0x000f
    };
const uint16_t celebration[]   =
    {
        0x0004,0x000A,0x0011,0x000A,0x0004
    };
//border to center
const uint16_t border_to_central[]  =
    {
        0x0011,0x000A,0x0004,0x0000
    };
//const uint16_t eye_style[];//´ý¶¨
const uint16_t percentage_20[]  =
    {
    //    0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f
        0x0001, 0x0000
    };
const uint16_t percentage_40[]  =
    {
    //    0x0001, 0x0003, 0x0007, 0x000f, 0x001f
        0x0003, 0x0000
    };
const uint16_t percentage_60[]  =
    {
    //    0x0003, 0x0007, 0x000f, 0x001f
        0x0007, 0x0000
    };
const uint16_t percentage_80[]  =
    {
     //   0x0007, 0x000f, 0x001f
        0x000f, 0x0000
    };
const uint16_t percentage_100[]  =
    {
        0x001f, 0x0000
    };
const uint16_t percentage_full[]  =
    {
        0x001f
    };

//step percentage
const uint16_t  step_percentage_20[]  =
    {
        0x0001,0x0000
    };
const uint16_t  step_percentage_40[]  =
    {
        0x0003,0x0001
    };
const uint16_t  step_percentage_60[]  =
    {
        0x0007,0x0003
    };
const uint16_t  step_percentage_80[]  =
    {
        0x000f,0x0007
    };
const uint16_t  step_percentage_100[] =
    {
        0x001f,0x000f
    };


/* single led flash */
const uint16_t first_led_flash[]          =
    {
        0x0001,0x0000
    };
const uint16_t second_led_flash[]         =
    {
        0x0002,0x0000
    };
const uint16_t third_led_flash[]          =
    {
        0x0004,0x0000
    };
const uint16_t forth_led_flash[]          =
    {
        0x0008,0x0000
    };
const uint16_t fifth_led_flash[]          =
    {
        0x0010,0x0000
    };

//blank led action
const uint16_t action_blank[]             =
    {
        0x0000,0x0000
    };



#define   DEFAULT_TIMER_PERIOD     APP_TIMER_TICKS(400, 0)
static    app_timer_id_t   flash_schedule_timer;   /* flash schedule timer */
static          uint8_t current_notification    = NOTIFICATION_STATE;

/* drive only one led */
static void single_led_control(uint8_t num, uint8_t status)
{
    if(status) {
        ((led_polarity_bit_array >> num) & 0x01) ? nrf_gpio_pin_set(led_pin_array[num]) : nrf_gpio_pin_clear(led_pin_array[num]);
    } else {
        ((led_polarity_bit_array >> num) & 0x01) ? nrf_gpio_pin_clear(led_pin_array[num]): nrf_gpio_pin_set(led_pin_array[num]);
    }

}

/* use a number to control LED status */
static void all_led_flash_control(uint16_t para)
{
    uint8_t pin = 0;
    for(; pin < LED_NUMBER ; ++pin) {
        single_led_control(pin,((para >> pin) & 0x01));
    }
}

static void led_schedule_timer_handle(void * context);
/* control flash style and times */
/*
static void _led_action_control(LED_FLASH_TYPE flash_type, uint16_t loop)
{
    uint32_t err_code = 0;
    current_flash_loop = 0;
    current_flash_style  =  flash_type;

    if(app_query_timer(flash_schedule_timer) == NRF_SUCCESS) {
        app_timer_stop(flash_schedule_timer);
    }

    switch(flash_type) {
        case ALL_FLASH:
            flash_loop_times    =  loop * ARRAY_LEN(all_flash);
            break;
        case SERIAL_FLASH:
        case ANIMATED_FLASH:
            flash_loop_times    =  loop * ARRAY_LEN(serial_flash);
            break;
        case SERIAL_CLOSE:
            flash_loop_times    =  loop * ARRAY_LEN(serial_close);
            break;
        case CELEBRATE:
            flash_loop_times    =  loop * ARRAY_LEN(celebration);
            break;
        case EYE_STYLE:
            flash_loop_times    =  loop * ARRAY_LEN(all_flash);
            break;
        case PERCENTAGE_20:
            flash_loop_times   =  loop * ARRAY_LEN(percentage_20);
            break;
        case PERCENTAGE_40:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_40);
            break;
        case PERCENTAGE_60:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_60);
            break;
        case PERCENTAGE_80:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_80);
            break;
        case PERCENTAGE_100:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_100);
            break;
        case FIRST_LED_FLASH:
        case SECOND_LED_FLASH:
        case THIRD_LED_FLASH:
        case FORTH_LED_FLASH:
        case FIFTH_LED_FLASH:
            flash_loop_times                =   loop * ARRAY_LEN(first_led_flash);
            break;
        case BORDER_TO_CENTRAL:
            flash_loop_times                =   loop * ARRAY_LEN(border_to_central);
            break;
        case LED_BLANK:
            flash_loop_times                =   loop * ARRAY_LEN(action_blank);
            break;
        case STEP_PERCENTAGE_20:
        case STEP_PERCENTAGE_40:
        case STEP_PERCENTAGE_60:
        case STEP_PERCENTAGE_80:
        case STEP_PERCENTAGE_100:
            flash_loop_times                =   loop *ARRAY_LEN(step_percentage_20);
            break;
        default:
            break;
    }

    if((current_flash_style != FLASH_NONE) && ( loop != 0 )) {
        led_schedule_timer_handle(NULL);

        if(current_flash_style == ANIMATED_FLASH){            
            err_code = app_timer_start(flash_schedule_timer, ANIMATION_TIMER_PERIOD,NULL);
        }
        else{
            err_code = app_timer_start(flash_schedule_timer,DEFAULT_TIMER_PERIOD,NULL);
        }        
        APP_ERROR_CHECK(err_code);
    }
}
uint8_t led_action_control(uint8_t notification_type, LED_FLASH_TYPE flash_type, uint16_t loop)
{
    if( charger_status() != NoCharge ) {
        if( notification_type > NOTIFICATION_CHARGING )
            return 0;
    }

    if( notification_type <= current_notification ) {
        current_notification = notification_type;

        _led_action_control(flash_type,loop);
        return 1;
    }
    return 0;
}
*/
/* This function can set customer flash action interval*/
static void _led_action_control_with_interval(LED_FLASH_TYPE flash_type, uint16_t loop,uint32_t time_interval)
{
    current_flash_loop = 0;
    current_flash_style  =  flash_type;

    if(app_query_timer(flash_schedule_timer) == NRF_SUCCESS) {
        app_timer_stop(flash_schedule_timer);
    }

    switch(flash_type) {
        case ALL_FLASH:
            flash_loop_times    =  loop * ARRAY_LEN(all_flash);
            break;
        case SERIAL_FLASH:
        case ANIMATED_FLASH:
            flash_loop_times    =  loop * ARRAY_LEN(serial_flash);
            break;
        case SERIAL_CLOSE:
            flash_loop_times    =  loop * ARRAY_LEN(serial_close);
            break;
        case CELEBRATE:
            flash_loop_times    =  loop * ARRAY_LEN(celebration);
            break;
        case EYE_STYLE:
            flash_loop_times    =  loop * ARRAY_LEN(all_flash);
            break;
        case PERCENTAGE_20:
            flash_loop_times   =  loop * ARRAY_LEN(percentage_20);
            break;
        case PERCENTAGE_40:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_40);
            break;
        case PERCENTAGE_60:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_60);
            break;
        case PERCENTAGE_80:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_80);
            break;
        case PERCENTAGE_100:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_100);
            break;
        case PERCENTAGE_FULL:
            flash_loop_times    =  loop * ARRAY_LEN(percentage_full);
            break;
        case FIRST_LED_FLASH:
        case SECOND_LED_FLASH:
        case THIRD_LED_FLASH:
        case FORTH_LED_FLASH:
        case FIFTH_LED_FLASH:
            flash_loop_times                =   loop * ARRAY_LEN(first_led_flash);
            break;
        case BORDER_TO_CENTRAL:
            flash_loop_times                =   loop * ARRAY_LEN(border_to_central);
            break;
        case LED_BLANK:
            flash_loop_times                =   loop * ARRAY_LEN(action_blank);
            break;
        case STEP_PERCENTAGE_20:
        case STEP_PERCENTAGE_40:
        case STEP_PERCENTAGE_60:
        case STEP_PERCENTAGE_80:
        case STEP_PERCENTAGE_100:
            flash_loop_times                =   loop *ARRAY_LEN(step_percentage_20);
            break;
        default:
            break;
    }

    if( (current_flash_style != FLASH_NONE) /*&& ( loop != 0 )*/ ) {
        led_schedule_timer_handle(NULL);
        app_timer_start(flash_schedule_timer,time_interval,NULL);
    }

}
uint8_t led_action_control_with_interval(uint8_t notification_type, LED_FLASH_TYPE flash_type, uint16_t loop,uint32_t time_interval)
{
    if( charger_status() != NoCharge ) {
        if( notification_type > NOTIFICATION_CHARGING )
            return 0;
    }
    if( notification_type <= current_notification ) {
        current_notification = notification_type;
        _led_action_control_with_interval(flash_type, loop,time_interval);
        return 1;
    }
    return 0;
}
uint8_t led_action_control(uint8_t notification_type, LED_FLASH_TYPE flash_type, uint16_t loop)
{
    return led_action_control_with_interval(notification_type, flash_type, loop, DEFAULT_TIMER_PERIOD);
}
void led_action_stop(void)
{
    app_timer_stop(flash_schedule_timer);
    current_flash_style = FLASH_NONE;
    current_flash_loop = 0;
    current_notification    = NOTIFICATION_STATE;
    if(charger_status()!= NoCharge) {
        update_charging_status(1);
    }else{
        all_led_flash_control(0x0000); //close all led
    }
}

static void led_schedule_timer_handle(void * context)
{
    (void)context;
    uint16_t *array_ptr;
    uint8_t len;
    switch(current_flash_style) {
        case ALL_FLASH:
            array_ptr = (uint16_t *)all_flash;
            len = ARRAY_LEN(all_flash);
            break;
        case SERIAL_FLASH:
        case ANIMATED_FLASH:

            array_ptr = (uint16_t *)serial_flash;
            len = ARRAY_LEN(serial_flash);
            break;
        case SERIAL_CLOSE:
            array_ptr = (uint16_t *)serial_close;
            len = ARRAY_LEN(serial_close);
            break;
        case CELEBRATE:
            array_ptr = (uint16_t *)celebration;
            len = ARRAY_LEN(celebration);
            break;
        case EYE_STYLE:
            array_ptr = (uint16_t *)all_flash;
            len = ARRAY_LEN(all_flash);
            break;
        case PERCENTAGE_20:
            array_ptr = (uint16_t *)percentage_20;
            len = ARRAY_LEN(percentage_20);
            break;
        case PERCENTAGE_40:
            array_ptr = (uint16_t *)percentage_40;
            len = ARRAY_LEN(percentage_40);
            break;
        case PERCENTAGE_60:
            array_ptr = (uint16_t *)percentage_60;
            len = ARRAY_LEN(percentage_60);
            break;
        case PERCENTAGE_80:
            array_ptr = (uint16_t *)percentage_80;
            len = ARRAY_LEN(percentage_80);
            break;
        case PERCENTAGE_100:
            array_ptr = (uint16_t *)percentage_100;
            len = ARRAY_LEN(percentage_100);
            break;
        case PERCENTAGE_FULL:
            array_ptr = (uint16_t *)percentage_full;
            len = ARRAY_LEN(percentage_full);
            break;
        case FIRST_LED_FLASH:
            array_ptr = (uint16_t *)first_led_flash;
            len = ARRAY_LEN(first_led_flash);
            break;
        case SECOND_LED_FLASH:
            array_ptr = (uint16_t *)second_led_flash;
            len = ARRAY_LEN(second_led_flash);
            break;
        case THIRD_LED_FLASH:
            array_ptr = (uint16_t*)third_led_flash;
            len = ARRAY_LEN(third_led_flash);
            break;
        case FORTH_LED_FLASH:
            array_ptr = (uint16_t*)forth_led_flash;
            len = ARRAY_LEN(forth_led_flash);
            break;
        case FIFTH_LED_FLASH:
            array_ptr = (uint16_t*)fifth_led_flash;
            len = ARRAY_LEN(fifth_led_flash);
            break;
        case BORDER_TO_CENTRAL:
            array_ptr = (uint16_t *)border_to_central;
            len = ARRAY_LEN(border_to_central);
            break;
        case LED_BLANK:
            array_ptr = (uint16_t *)action_blank;
            len = ARRAY_LEN(action_blank);
            break;
        case STEP_PERCENTAGE_20:
            array_ptr = (uint16_t *)step_percentage_20;
            len = ARRAY_LEN(step_percentage_20);
            break;
        case STEP_PERCENTAGE_40:
            array_ptr = (uint16_t *)step_percentage_40;
            len = ARRAY_LEN(step_percentage_40);
            break;
        case STEP_PERCENTAGE_60:
            array_ptr = (uint16_t *)step_percentage_60;
            len = ARRAY_LEN(step_percentage_60);
            break;
        case STEP_PERCENTAGE_80:
            array_ptr = (uint16_t *)step_percentage_80;
            len = ARRAY_LEN(step_percentage_80);
            break;
        case STEP_PERCENTAGE_100:
            array_ptr = (uint16_t *)step_percentage_100;
            len = ARRAY_LEN(step_percentage_100);
            break;
        default:
            array_ptr = NULL;
            len = 0;
            break;
    }

    if(array_ptr && ((current_flash_loop < flash_loop_times)||(flash_loop_times == 0)) ) {
        all_led_flash_control(array_ptr[current_flash_loop % len]);
        current_flash_loop++;
    } else {
        led_action_stop();
    }

}

uint8_t notification_status()
{
    return current_notification;
}

uint32_t led_flash_timer_init(void)
{
    return app_timer_create(&flash_schedule_timer,APP_TIMER_MODE_REPEATED,led_schedule_timer_handle);
}
