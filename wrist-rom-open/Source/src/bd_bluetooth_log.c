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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "config.h"
#include "nrf_error.h"
#include "bd_communicate_protocol.h"
#include "simple_uart.h"
#include "bd_wall_clock_timer.h"

#define BLUETOOTH_LOG_BUFFER_LEN    (200)

static uint8_t __bluetooth_log_buffer[BLUETOOTH_LOG_BUFFER_LEN + L1L2_HEAD_LEN];
static char * bluetooth_log_buffer = (char *)(__bluetooth_log_buffer + L1L2_HEAD_LEN);

static bool enable_bluetooth_log = false;
static bool in_sending_progress = false;

void set_bluetooth_log_state(bool state) 
{
    enable_bluetooth_log = state;
}

bool get_bluetooth_log_state(void)
{
    return enable_bluetooth_log;
}


//send & callback
void log_send_callback(SEND_STATUS status)
{
    (void)status;

    in_sending_progress = false;
    memset(__bluetooth_log_buffer,0,BLUETOOTH_LOG_BUFFER_LEN + L1L2_HEAD_LEN);
}

void bluetooth_log(const char * func, uint32_t line, const char * restrict format, ...)
{
    if(enable_bluetooth_log && (!in_sending_progress)) { //enable log & no content in sending progress

        uint16_t length;
        va_list ap;
        uint32_t ret;
        
        L2_Send_Content send_content;
        
        UTCTimeStruct * tm = get_wall_clock_time();
        snprintf(bluetooth_log_buffer,BLUETOOTH_LOG_BUFFER_LEN,"[%d-%d-%d:%d:%d:%d]",tm->year,
                                                                                     tm->month,
                                                                                     tm->day,
                                                                                     tm->hour,
                                                                                     tm->minutes,
                                                                                     tm->seconds);

        length = strlen(bluetooth_log_buffer);

        snprintf(bluetooth_log_buffer + length,BLUETOOTH_LOG_BUFFER_LEN - length,"%s(%d):",func,line);
        length = strlen(bluetooth_log_buffer);

        va_start(ap, format);
        (void)vsnprintf(bluetooth_log_buffer + length, BLUETOOTH_LOG_BUFFER_LEN - length, format, ap);
        va_end(ap);
        
        length = strlen(bluetooth_log_buffer);
        
        //fill L2 header
        __bluetooth_log_buffer[0] = BLUETOOTH_LOG_COMMAND_ID;
        __bluetooth_log_buffer[1] = L2_HEADER_VERSION;
        __bluetooth_log_buffer[2] = KEY_BLUETOOTH_LOG_CONTENT;
        __bluetooth_log_buffer[3] = (length >> 8) & 0xff;
        __bluetooth_log_buffer[4] = length & 0xff;

        send_content.length = length + L1L2_HEAD_LEN;
        send_content.callback = log_send_callback;
        send_content.content = __bluetooth_log_buffer;

        ret = L1_send(&send_content);
        if(ret == NRF_SUCCESS){
            in_sending_progress = true;
        } else { //just delete the log
            in_sending_progress = false;
            memset(__bluetooth_log_buffer,0,BLUETOOTH_LOG_BUFFER_LEN + L1L2_HEAD_LEN);
        }
    }
}


void resolve_log_command_id(log_command_key_t key)
{
    switch(key) {
        case KEY_ENABLE_BLUETOOTH_LOG:
            set_bluetooth_log_state(true);
            break;
        case KEY_DISABLE_BLUETOOTH_LOG:
            set_bluetooth_log_state(false);
            break;
        default:
            LOG(LEVEL_INFO,"receive wrong key");
            break;
    };
}


