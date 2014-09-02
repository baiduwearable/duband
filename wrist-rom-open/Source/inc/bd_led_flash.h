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
#ifndef _LED_FLASH_
#define _LED_FLASH_

#include "stdint.h"
#include "config.h"

#define LED_NUMBER (5)

typedef enum {
    ALL_FLASH       = 0,   /*ÆëÉÁ*/
    SERIAL_FLASH    = 1,    /*ÂÖÁ÷µãÁÁ*/
    SERIAL_CLOSE    = 2,   /*ÂÖÁ÷Ï¨Ãð*/
    CELEBRATE       = 3,   /*Çì×£ÌøÉÁ*/
    EYE_STYLE       = 4,   /*ÏñÐ¡ÑÛ¾¦Ò»ÑùÉÁ¶¯*/
    PERCENTAGE_20   = 5,   /* < 20%*/
    PERCENTAGE_40   = 6,   /* < 40%*/
    PERCENTAGE_60   = 7,   /* < 60%*/
    PERCENTAGE_80   = 8,   /* < 80%*/
    PERCENTAGE_100  = 9,   /* < 100%*/
    PERCENTAGE_FULL  = 100,   /* = 100%*/
    FIRST_LED_FLASH = 10,
    SECOND_LED_FLASH= 11,
    THIRD_LED_FLASH = 12,
    FORTH_LED_FLASH = 13,
    FIFTH_LED_FLASH = 14,
    BORDER_TO_CENTRAL= 15,
    LED_BLANK        = 16,
    STEP_PERCENTAGE_20   = 17,   /*20%*/
    STEP_PERCENTAGE_40   = 18,   /*40%*/
    STEP_PERCENTAGE_60   = 19,   /*60%*/
    STEP_PERCENTAGE_80   = 20,   /*80%*/
    STEP_PERCENTAGE_100  = 21,   /*100%*/
    ANIMATED_FLASH  = 22,   /*100%*/

    FLASH_NONE      = 0xff
}LED_FLASH_TYPE;

uint32_t led_flash_timer_init(void);
uint8_t led_action_control(uint8_t notification_type, LED_FLASH_TYPE flash_type, uint16_t loop);
uint8_t led_action_control_with_interval(uint8_t notification_type, LED_FLASH_TYPE flash_type, uint16_t loop,uint32_t time_interval);
void led_action_stop(void);
uint8_t notification_status(void);
#endif //_LED_FLASH_
