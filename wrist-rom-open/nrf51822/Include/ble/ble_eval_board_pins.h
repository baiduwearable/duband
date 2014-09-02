/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_lib_eval_board_pins Evaluation Board LEDs and Buttons Pin Assignments
 * @{
 * @ingroup ble_sdk_lib
 * @brief Pin assignments for LEDs and Buttons on the evaluation board
 *
 * @details Pin assignments for LEDs and Buttons in evaluation board applications.
 */

#ifndef BLE_EVAL_BOARD_PINS_H__
#define BLE_EVAL_BOARD_PINS_H__

#define EVAL_BOARD_LED_0           18                   /**< Pin connected to LED 0 in the pca10001 evaluation board.*/
#define EVAL_BOARD_LED_1           19                   /**< Pin connected to LED 1 in the pca10001 evaluation board.*/

#define EVAL_BOARD_BUTTON_0        16                   /**< Pin connected to BUTTON 0 in the pca10001 evaluation board.*/
#define EVAL_BOARD_BUTTON_1        17                   /**< Pin connected to BUTTON 1 in the pca10001 evaluation board.*/

#define ADVERTISING_LED_PIN_NO     EVAL_BOARD_LED_0     /**< Pin that can be used by applications to indicate advertising state.*/
#define CONNECTED_LED_PIN_NO       EVAL_BOARD_LED_1     /**< Pin that can be used by applications to indicate connected state.*/

#endif // BLE_EVAL_BOARD_PINS_H__

/** @} */
