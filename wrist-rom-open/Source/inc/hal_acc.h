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
#ifndef HAL_ACC_H
#define HAL_ACC_H

#include "bd_lis3dh_driver.h"
#define LIS3DH_ODR_FREQ LIS3DH_ODR_100Hz
#define ACC_UNIT            0.031
#define TAP_TIME_UNIT
#define TAP_TIME_LIMIT      3
#define TAP_LATENCY         14
#define TAP_WINDOW          30
#define TAP_THRESHOLD       1.5/ACC_UNIT

void hal_acc_init(void);
void hal_acc_PowerDown(void);
void hal_acc_enable(void);
bool hal_acc_ConfigClick(LIS3DH_ODR_t Freq, LIS3DH_Fullscale_t FullScale);
void hal_acc_GetFifoData(uint8_t * val);
void hal_acc_config_wake_int(bool enable);
void hal_acc_config_Dtap(bool enable);
void hal_acc_reset(void);
/*lint --flb "Leave library region" */
#endif

