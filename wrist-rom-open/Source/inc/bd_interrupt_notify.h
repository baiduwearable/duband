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
#ifndef INTERRUPT_NOTIFY_H__
#define INTERRUPT_NOTIFY_H__

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "nrf_gpio.h"

typedef void (* io_int_event_callback_t)(uint8_t pin_no); // 带一个参数，可以用一个callback来处理多个引脚的中断回调

typedef struct
{
    uint8_t pin_no;
    bool    active_high;
    nrf_gpio_pin_pull_t pull_cfg;
    io_int_event_callback_t callback;

}
io_int_event_cfg_t;



uint32_t interrupt_notify_register( io_int_event_cfg_t *      p_io_pins,
                                    uint8_t                 pin_count);

uint32_t app_io_evt_register_enable(void);

uint32_t app_io_evt_register_disable(void);



#endif //INTERRUPT_NOTIFY_H__
