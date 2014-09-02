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
#ifndef _BD_BLUETOOTH_LOG_H_
#define _BD_BLUETOOTH_LOG_H_
#include "bd_communicate_protocol.h"

void bluetooth_log(const char * func, uint32_t line, const char * restrict format, ...);

#define BLUETOOTH_LOG(format, ...) bluetooth_log(__func__, __LINE__, format, ##__VA_ARGS__); 

void resolve_log_command_id(log_command_key_t key);
void set_bluetooth_log_state(bool state); 
bool get_bluetooth_log_state(void);

#endif //_BD_BLUETOOTH_LOG_H_
