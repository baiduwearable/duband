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
#ifndef BD_FACTORY_TEST__
#define BD_FACTORY_TEST__

#include "board_config_pins.h"
#include <stdint.h>

#define LED_SENSOR_TEST_RESULT BAIDU_LED_3

#define GRAVITY_VALUE (8192)
#define MIN_SENSOR_VALUE ((GRAVITY_VALUE - 819)*(GRAVITY_VALUE - 819))
#define MAX_SENSOR_VALUE ((GRAVITY_VALUE + 819)*(GRAVITY_VALUE + 819))


#define FACTORY_TEST_FLAG_OFF (8)
#define SERIAL_NUMBER_LENGTH (32)


void bootup_check(void);
void do_test(uint8_t *data, uint16_t length);
uint8_t is_factory_test_done(void);

#endif
