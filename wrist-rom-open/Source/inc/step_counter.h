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
#ifndef _STEP_COUNTER_H_
#define _STEP_COUNTER_H_

#include <stdint.h>
#include "health-algorithm.h"
#include "bd_wall_clock_timer.h"

#include <nrf_assert.h>
#include "app_timer.h"


//#define USE_FLASH
extern app_timer_id_t                        m_sensor_timer_id;
// extern app_timer_id_t                        m_minute_timer_id;
#define PACKET_SIZE                     512                                                     /**< Size of each data packet. Also used for initial receiving of packets from transport layer. */
#define PACKET_HEADER_SIZE              sizeof(uint32_t)                                        /**< Size of the data packet header. */
#define CODE_REGION_1_START             0x00014000                                              /**< This field should correspond to the size of Code Region 0, (Which is identical to Start of Code Region 1), found in UICR.CLEN0 register. This value is used for compile safety, as the linker will fail if application expands into bootloader. Runtime, the bootloader will use the value found in UICR.CLEN0. */
#define BOOTLOADER_REGION_START         0x00038000                                              /**< This field should correspond to start address of the bootloader, found in UICR.RESERVED, 0x10001014, register. This value is used for sanity check, so the bootloader will fail immediately if this value differs from runtime value. The value is used to determine max application size for updating. */
#define BOOTLOADER_SETTINGS_ADDRESS     0x0003EC00                                              /**< The field specifies the page location of the bootloader settings address. */
#define APP_IMAGE_MAX_SIZE              BOOTLOADER_REGION_START - CODE_REGION_1_START           /**< Maximum size of data image we current support for update. */
#define CODE_PAGE_SIZE                  1024                                                    /**< Size of a flash codepage. Used for size of the reserved flash space in the bootloader region. Will be runtime checked against NRF_UICR->CODEPAGESIZE to ensure the region is correct. */
#define EMPTY_FLASH_MASK                0xFFFFFFFF                                              /**< Bit mask that defines an empty address in flash. */

#define INVALID_BANK                    0xFF                                                    /**< Invalid bank identifier. */
#define BANK_0                          0x1                                                     /**< Bank 0 identifier. */
#define BANK_1                          0x2                                                     /**< Bank 1 identifier. */
#define VALID_BANK                      0x1                                                     /**< Valid bank identifier. */
#define DEFAULT_STEP_TASK     10000              /**<default task 10000 steps>*/

#define DEFAULT_HIGHT_CM  (170)
#define DEFAULT_WEIGHT_CM  (65)
#define DEFAULT_AGE   (25)
#define DEFAULT_GENDER   1
#define GENDER_MALE   1
#define GENDER_FEMALE  0

typedef uint32_t DateType;
/* time bit field */
typedef struct
{
uint16_t day   :
    5;
uint16_t month  :
    4;
uint16_t year   :
    6;
uint16_t reserved   :
    1;
}
Date_bit_field_type_t;
typedef union
{
    uint16_t data;
    Date_bit_field_type_t date;
} Date_union_t;

typedef struct SportsHead
{
    uint8_t key;
    uint8_t length;
    Date_union_t Date;
}
SportsHead_t;

typedef struct SleepHead
{
    Date_union_t Date;
    uint16_t length;
}
SleepHead_t;

typedef struct
{
uint64_t Distance  :
    16;
uint64_t Calory   :
    19;
uint64_t active_time :
    4;
uint64_t steps   :
    12;
uint64_t mode   :
    2;
uint64_t offset   :
    11;
}
SportsData_bit_field_type_t;

typedef union SportsData {
    uint64_t data;
    SportsData_bit_field_type_t bits;
} SportsData_U;

typedef struct
{
uint32_t mode   :
    4;
uint32_t sleeping_flag :
    1;
uint32_t reserved  :
    11;
uint32_t timeStamp  :
    16;
}
SleepData_bit_field_type_t;

typedef union SleepDataU {
    uint32_t data;
    SleepData_bit_field_type_t bits;
} SleepData_U;


void set_quarter_steps(uint16_t steps);
void set_quarter_distance(int distance);
void set_quarter_calories(int calories);
void set_quater_sport_mode(uint8_t mode);
void set_quater_active_time(uint8_t active_time);
uint16_t get_quarter_steps(void);
int get_quarter_distance(void);
int get_quarter_calories(void);


void sensor_timer_handle(void * val);
void button_event_handler(uint8_t pin_no);
static uint8_t readQuarterData(void);
void LIS3DH_INT1_event_handler(uint8_t pin_no);
void LIS3DH_INT2_event_handler(uint8_t pin_no);
void LIS3DH_GetFifoData(void* val);
void led_schedual(void);
uint8_t StepCounter_Init(void);
void minute_timer_handler(void * val);
void tap_timer_handler(void * val);

void restartStepCounter(void);
void sensor_timers_start(void);
void sensor_timers_stop(void);
void restart_health_algorithm(void);
bool is_algorithm_started(void);
uint8_t reset_health_algorithm_data(void);
void set_algorithm_timestamp(void);
void update_algorithm_timestamp(void);
void save_quarter_distance_data(uint8_t sportMode);
uint8_t sports_mode(void);
void set_daily_target_achieved(bool value);
bool get_daily_target_achieved(void);

#endif
