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

#include <string.h>
#include "ble_debug_assert_handler.h"
#include "ble_error_log.h"
#include "bd_communicate_protocol.h"
#include "bd_led_flash.h"
#include "app_util.h"


#define  ERROR_DUMP_INFO ("fail to get dump info")



extern uint8_t global_reponse_buffer[];
extern ble_error_log_data_t   m_ble_error_log;                   /* defined in file ble_error_log */
static uint32_t get_assert_dump_info(void)
{

    uint8_t  error_log_size = CEIL_DIV(sizeof(ble_error_log_data_t), sizeof(uint32_t));
    uint32_t err_code       = NRF_SUCCESS;

    err_code = ble_flash_page_read(FLASH_PAGE_ERROR_LOG, (uint32_t *) (&m_ble_error_log), &error_log_size);

    //erase the page
    ble_flash_page_delay_erase(FLASH_PAGE_ERROR_LOG);

    return err_code;
}

static uint16_t encode_assert_pos_to_buffer(uint8_t * addr)
{
    //copy the header info to buffer
    uint16_t length = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + ERROR_MESSAGE_LENGTH;
    memcpy(addr,(uint8_t *)(&m_ble_error_log),length);

    return length;
}

static void report_assert_code_position(void)
{

    L2_Send_Content sendContent;
    uint16_t length = 0;

    length = encode_assert_pos_to_buffer(global_reponse_buffer + L2_FIRST_VALUE_POS);

    global_reponse_buffer[0] = GET_STACK_DUMP;        /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;       /*L2 header version */
    global_reponse_buffer[2] = 0x02;         /*01 represent get filen name info,02 represent get stack dump info*/
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = length;       /* length  = 1 */

    sendContent.callback = NULL;
    sendContent.length = length + L2_FIRST_VALUE_POS;
    sendContent.content = global_reponse_buffer;

    L1_send(&sendContent);

}

static uint16_t encode_stack_info_to_buffer(uint8_t * addr)
{
    uint16_t length = STACK_DUMP_LENGTH;
    memcpy(addr,(uint8_t *)(&m_ble_error_log.stack_info),length);

    return length;
}

static void report_assert_stack_info(void)
{
    L2_Send_Content sendContent;
    uint16_t length = 0;

    length = encode_stack_info_to_buffer(global_reponse_buffer + L2_FIRST_VALUE_POS);

    global_reponse_buffer[0] = GET_STACK_DUMP;        /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;       /*L2 header version */
    global_reponse_buffer[2] = 0x04;         /*01 represent get filen name info,02 represent get stack dump info*/
    global_reponse_buffer[3] = length >> 8;
    global_reponse_buffer[4] = length & 0xFF;       /* length  = 256 */

    sendContent.callback = NULL;
    sendContent.length = length + L2_FIRST_VALUE_POS;
    sendContent.content = global_reponse_buffer;

    L1_send(&sendContent);

}

static void report_get_dump_info_fail(void)
{
    L2_Send_Content sendContent;

    uint16_t length = strlen(ERROR_DUMP_INFO);
    strcpy((char *)(global_reponse_buffer + L2_FIRST_VALUE_POS),ERROR_DUMP_INFO);

    global_reponse_buffer[0] = GET_STACK_DUMP;        /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;       /*L2 header version */
    global_reponse_buffer[2] = 0x05;         /*01 represent get filen name info,02 represent get stack dump info*/
    global_reponse_buffer[3] = 0;
    global_reponse_buffer[4] = length;       /* length  = 1 */

    sendContent.callback = NULL;
    sendContent.length = length + L2_FIRST_VALUE_POS;
    sendContent.content = global_reponse_buffer;

    L1_send(&sendContent);

}

void get_stack_dump_command_resolve(uint8_t key)
{
    uint32_t err_code = get_assert_dump_info();

    if(key == 0x05) { //get run status

        return;
    }


    if(err_code == NRF_SUCCESS) {
        switch(key) {
            case 0x01: //get assert position
                report_assert_code_position();
                break;
            case 0x03: //get stack dump
                report_assert_stack_info();
                break;
            case 0x05: //get run status
                break;
            default:
                break;
        }
    } else {
        report_get_dump_info_fail();
    }

}

