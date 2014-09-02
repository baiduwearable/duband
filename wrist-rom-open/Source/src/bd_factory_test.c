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
#include "bd_factory_test.h"
#include "app_timer.h"
#include "config.h"
#include "bd_led_flash.h"
#include "app_error.h"
#include "bd_battery.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "nrf_delay.h"
#include "hal_acc.h"
#include "bd_spi_master.h"
#include "spi_master_config.h" // This file must be in the application folder
#include "nrf_gpio.h"
#include "nrf_assert.h"
#include "bd_communicate_protocol.h"
#include "app_scheduler.h"
#include "nrf_soc.h"
#include "bd_low_power_mode.h"
#include "nordic_common.h"
#include "ble_flash.h"

extern AxesRaw_t accData[32]; // Read acc data from FIFO
extern uint8_t global_reponse_buffer[50];
extern uint16_t g_battery_voltage_mv;

uint8_t g_send_flag = true;
uint8_t g_write_sn_done = true;
uint8_t g_write_flag_done = true;

uint8_t g_testMode = 0;
uint8_t SERIAL_NUMBER[SERIAL_NUMBER_LENGTH];

uint8_t is_factory_test_done(void)
{
    uint32_t *flag_addr;
    ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &flag_addr );

    return (*((uint8_t*)(flag_addr + FACTORY_TEST_FLAG_OFF))) == (uint8_t)0x00;
}

void get_sensor_data(uint8_t *value)
{
    uint8_t transfer_size;

    hal_acc_GetFifoData(&transfer_size);
    value[0] = accData[0].AXIS_X >> 8;
    value[1] = accData[0].AXIS_X & 0x00FF;
    value[2] = accData[0].AXIS_Y >> 8;
    value[3] = accData[0].AXIS_Y & 0x00FF;
    value[4] = accData[0].AXIS_Z >> 8;
    value[5] = accData[0].AXIS_Z & 0x00FF;

}

uint8_t sensor_test()
{
    uint8_t result = true;
    uint8_t transfer_size;
    int16_t accX,accY,accZ;
    int32_t compose = 0;
    uint8_t index = 0;

    hal_acc_GetFifoData(&transfer_size);

    for(index = 0; index <= transfer_size; index++) {
        //printAccRaw(accData[index].AXIS_X,accData[index].AXIS_Y,accData[index].AXIS_Z);
        accX = accData[index].AXIS_X;
        accY = accData[index].AXIS_Y;
        accZ = accData[index].AXIS_Z;
        compose = compose + (accX*accX + accY*accY + accZ*accZ);
    }

    if( !(MIN_SENSOR_VALUE <= (compose / transfer_size) <= MAX_SENSOR_VALUE )) {
        result = false;
    }

    return result;
}

void all_led_flash()
{
    nrf_gpio_port_clear((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT0, 0xE0);//BAIDU_LED_0, BAIDU_LED_1, BAIDU_LED_2
    nrf_gpio_port_clear((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT2, 0x0C);//BAIDU_LED_3, BAIDU_LED_4
    nrf_delay_ms(400);
    nrf_gpio_port_set((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT0, 0xE0);//BAIDU_LED_0, BAIDU_LED_1, BAIDU_LED_2
    nrf_gpio_port_set((nrf_gpio_port_select_t)NRF_GPIO_PORT_SELECT_PORT2, 0x0C);//BAIDU_LED_3, BAIDU_LED_4
    nrf_delay_ms(400);
}


void bootup_check()
{
    //1.Check LEDs first
    all_led_flash();

    //2.check test flag
    if(is_factory_test_done())
        return;

    //3.SENSOR TEST
    if(sensor_test()) {
        nrf_gpio_pin_clear(LED_SENSOR_TEST_RESULT);
        nrf_delay_ms(400);
        nrf_gpio_pin_set(LED_SENSOR_TEST_RESULT);
    }

    //4.init flash sn & factory test flag
    uint32_t *flag_addr = 0;
    ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &flag_addr );
    if( *flag_addr != 0xFFFFFFFF){
        ble_flash_page_erase(FLASH_PAGE_SN_FACTORY_FLAG);
    }
}

void system_off(void* data, uint16_t length)
{
    UNUSED_VARIABLE(data);
    UNUSED_VARIABLE(length);
    sd_system_off();
}

void vibrator_test()
{
#ifdef FEATURE_MOTOR
    nrf_gpio_pin_set(BAIDU_MOTOR_0);
    nrf_delay_ms(500);
    nrf_gpio_pin_clear(BAIDU_MOTOR_0);
#endif
}

void send_package(L2_Send_Content *content)
{
    if(g_send_flag) {
        L1_send(content);
        g_send_flag = false;
    }
}

void send_callback(SEND_STATUS status )
{
    if(status == SEND_SUCCESS) {
        g_send_flag = true;
    }
}

void generate_l2_package(
    L2_Send_Content *content,
    BLUETOOTH_COMMUNICATE_COMMAND id,
    uint8_t key,
    uint16_t length,
    uint8_t* value)
{

    global_reponse_buffer[0] = id;     /*command id*/
    global_reponse_buffer[1] = L2_HEADER_VERSION;           /*L2 header version */
    global_reponse_buffer[2] = key;             /*echo return*/
    global_reponse_buffer[3] = length >> 8;
    global_reponse_buffer[4] = (uint8_t)(length & 0x00FF);

    for(int i = 0; i < length; i++) {
        global_reponse_buffer[5+i] = value[i];
    }

    content->callback    = send_callback;
    content->content     = global_reponse_buffer;
    content->length      = L2_HEADER_SIZE + L2_PAYLOAD_HEADER_SIZE + length;   /*length of whole L2*/
}

void write_flash_handler(void * data, uint16_t length)
{
    uint8_t size;
    uint32_t *addr;
    uint32_t *value;

    switch(*((uint8_t*)data)) {
        case KEY_WRITE_SN:
            size = SERIAL_NUMBER_LENGTH/sizeof(uint32_t);
            ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &addr );
            value = (uint32_t*)SERIAL_NUMBER;
            ble_flash_block_write(addr, value, size);
            g_write_sn_done = true;
            break;
        case KEY_WRITE_FLAG:
            ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &addr );
            addr += FACTORY_TEST_FLAG_OFF;
            ble_flash_word_write(addr, (uint32_t)0);
            g_write_flag_done = true;
            break;
        default:
            return;
    }

}

void write_test_flag()
{
    uint32_t err_code;
    uint8_t data = KEY_WRITE_FLAG;
    g_write_flag_done = false;
    err_code = app_sched_event_put(&data, 1, (app_sched_event_handler_t)write_flash_handler);
    APP_ERROR_CHECK(err_code);
}

void read_flag(void *data, uint16_t length)
{
    if(g_send_flag) {
        L2_Send_Content content;
        uint32_t *addr;
        ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &addr );
        addr += FACTORY_TEST_FLAG_OFF;
        generate_l2_package(&content, FACTORY_TEST_COMMAND_ID, KEY_RETURN_FLAG, 1, (uint8_t*)addr );
        send_package(&content);
    } else {
        uint32_t err_code;
        err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)read_flag);
        APP_ERROR_CHECK(err_code);
    }
}

void write_sn(uint8_t* data, int16_t length)
{
    uint32_t err_code;
    uint8_t value = KEY_WRITE_SN;
    g_write_sn_done = false;
    err_code = app_sched_event_put(&value, 1, (app_sched_event_handler_t)write_flash_handler);
    APP_ERROR_CHECK(err_code);
    for(int i=0;i<length;i++) {
        SERIAL_NUMBER[i]=data[i+L2_PAYLOAD_HEADER_SIZE];
    }
}

void read_sn(void *data, uint16_t length)
{
    if(g_send_flag && g_write_sn_done) {
        L2_Send_Content content;
        uint32_t *addr;
        ble_flash_page_addr(FLASH_PAGE_SN_FACTORY_FLAG, &addr );
        generate_l2_package(&content, FACTORY_TEST_COMMAND_ID, KEY_RETURN_SN, SERIAL_NUMBER_LENGTH, (uint8_t*)addr);
        send_package(&content);
    } else {
        uint32_t err_code;
        err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)read_sn);
        APP_ERROR_CHECK(err_code);
    }
}

void request_echo(void *data, uint16_t length)
{
    if(g_send_flag) {
        L2_Send_Content content;
        generate_l2_package(&content, FACTORY_TEST_COMMAND_ID, KEY_RETURN_ECHO, length, data);
        send_package(&content);
    } else {
        uint32_t err_code;
        err_code = app_sched_event_put(data, length , (app_sched_event_handler_t)request_echo);
        APP_ERROR_CHECK(err_code);
    }
}

void request_sensor_data(void *data, uint16_t length)
{
    if(g_send_flag) {
        uint8_t value[6];
        get_sensor_data(value);
        L2_Send_Content content;
        generate_l2_package(&content, FACTORY_TEST_COMMAND_ID, KEY_RETURN_SENSOR, 6, value);
        send_package(&content);
    }
}

void enter_test_mode()
{
    if(!g_testMode) {
        g_testMode = 1;
    }
}

void exit_test_mode()
{
    g_testMode = 0;
    uint32_t err_code;
    err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)system_off);
    APP_ERROR_CHECK(err_code);
}

void request_charge(void * data, uint16_t length)
{
    if(g_send_flag) {
        uint8_t value[2];

        value[0] = g_battery_voltage_mv >> 8;
        value[1] = g_battery_voltage_mv & 0x00FF;
        L2_Send_Content content;
        generate_l2_package(&content, FACTORY_TEST_COMMAND_ID, KEY_RETURN_CHARGE, 2, value);
        send_package(&content);
    } else {
        uint32_t err_code;
        err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)request_charge);
        APP_ERROR_CHECK(err_code);
    }
}

void do_test(uint8_t *data, uint16_t length)
{
    if((KEY_ENTER_TEST_MODE != *data) && !g_testMode)
        return;
    uint32_t err_code;
    switch((FACTORY_TEST_KEY)(*data)) {
        case KEY_REQUEST_ECHO:
            err_code = app_sched_event_put(data + L2_PAYLOAD_HEADER_SIZE, length , (app_sched_event_handler_t)request_echo);
            APP_ERROR_CHECK(err_code);
            break;
        case KEY_REQUEST_SENSOR:
            err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)request_sensor_data);
            APP_ERROR_CHECK(err_code);
            break;
        case KEY_LED_TEST:
            all_led_flash();
            break;
        case KEY_VIBRATOR_TEST:
            vibrator_test();
            break;
        case KEY_WRITE_FLAG:
            if(!is_factory_test_done())
                write_test_flag();
            break;
        case KEY_WRITE_SN:
            if(!is_factory_test_done())
                write_sn(data, length);
            break;
        case KEY_READ_FLAG:
            err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)read_flag);
            APP_ERROR_CHECK(err_code);
            break;
        case KEY_READ_SN:
            err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)read_sn);
            APP_ERROR_CHECK(err_code);
            break;
        case KEY_ENTER_TEST_MODE:
            enter_test_mode();
            break;
        case KEY_EXIT_TEST_MODE:
            exit_test_mode();
            break;
        case KEY_REQUEST_CHARGE:
            err_code = app_sched_event_put(NULL, 0, (app_sched_event_handler_t)request_charge);
            APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }
}
