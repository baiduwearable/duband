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

#ifndef BUZZER_H__
#define BUZZER_H__


/**@brief   Function to start flashing the LED.
 * @details This will start the TIMER1 and enable the GPIOTE task that toggles the LED.
 *          The PPI and GPIOTE configurations done by this app will make this action result in the
 *          flashing of the LED.
 * @pre Can only be called after the SoftDevice is enabled - uses nrf_soc API
 */
void buzzer_start(void);

/**@brief   Function to stop flashing the LED.
 * @details This will stop the TIMER1 and disable the GPIOTE task that toggles the LED.
 *          The PPI and GPIOTE configurations done by this app will
 *          make this action result in the turning off the LED.
 */
void buzzer_stop(void);

#endif // buzzer_H__

/** @} */
/** @endcond */
