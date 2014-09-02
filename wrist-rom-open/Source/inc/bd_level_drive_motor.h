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
#ifndef _BD_LEVEL_DRIVE_MOTOR_H_
#define _BD_LEVEL_DRIVE_MOTOR_H_

#include <stdint.h>

typedef enum {
    VIBRATE_NONE = 0,
    FAST_VIBRATE = 1,
    SLOW_VIBRATE = 2
}MOTOR_ACTION;


typedef enum {
    VIBRATE = 0,
    SLIENT  = 1
}MOTOR_OP;

void motor_init(void);

uint32_t motor_control_framework_init(void);

void motor_action_control_start(MOTOR_ACTION action, uint16_t virbrate_times);

void motor_action_control_stop(void );

#endif
