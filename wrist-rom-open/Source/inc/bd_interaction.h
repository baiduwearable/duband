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
#ifndef _INTERACTION_
#define _INTERACTION_

#include "stdint.h"
#include "config.h"

#define NOTIFICATION_DEBUG          0
#define NOTIFICATION_BONDING        1
#define NOTIFICATION_BONDED         2
#define NOTIFICATION_TEST           3
#define NOTIFICATION_CHARGING       4
#define NOTIFICATION_CALLING        5
#define NOTIFICATION_ALARM          6
#define NOTIFICATION_LOSE           7
#define NOTIFICATION_SWITCH         8
#define NOTIFICATION_TARGET         9
#define NOTIFICATION_STATE          10

#define NOTIFICATION_ANIMATE        11



// typedef enum {
//     Bonding = 0,
//     Charging,
//     Calling,
//     Alarm,
//     Counter,
//     Sleep,
//     Unknown
// }Notification_Status;

void notification_start( uint8_t type, uint16_t value );
void notification_stop( void );

#endif //_INTERACTION_
