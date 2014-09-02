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
#ifndef _PRIVATE_BOND_H_
#define _PRIVATE_BOND_H_
#include <stdint.h>
#include "config.h"

#define   USER_ID_LENGTH (32)

#define  FLASH_PAGE_USER_ID                     FLASH_PAGE_PRIVATE_BOND


uint32_t bond_read_user_id(void);
uint32_t bond_store_user_id(void);

void bond_press_handle(void );
void bond_success_event_dispatch(void);
void bond_fail_event_dispatch(void);
uint32_t check_user_id_bonded(const uint8_t* user_id, uint8_t length);

void login_success_event_dispatch(void);
void login_fail_event_dispatch(void);
bool check_has_bonded(void);
void set_device_has_bonded(bool value);

#endif //_PRIVATE_BOND_H_
