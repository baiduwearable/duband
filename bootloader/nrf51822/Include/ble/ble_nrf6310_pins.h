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
 * @defgroup ble_sdk_lib_nrf6310_pins nRF6310 Board LEDs and Buttons Pin Assignments
 * @{
 * @ingroup ble_sdk_lib
 * @brief Pin assignments for LEDs and Buttons on the nRF6310 board
 *
 * @details Pin assignments for LEDs and Buttons to be used in all SDK applications.
 */

#ifndef BLE_NRF6310_PINS_H__
#define BLE_NRF6310_PINS_H__

#define NRF6310_LED_0           8
#define NRF6310_LED_1           9
#define NRF6310_LED_2           10
#define NRF6310_LED_3           11
#define NRF6310_LED_4           12
#define NRF6310_LED_5           13
#define NRF6310_LED_6           14
#define NRF6310_LED_7           15

#define NRF6310_BUTTON_0        0
#define NRF6310_BUTTON_1        1
#define NRF6310_BUTTON_2        2
#define NRF6310_BUTTON_3        3
#define NRF6310_BUTTON_4        4
#define NRF6310_BUTTON_5        5
#define NRF6310_BUTTON_6        6
#define NRF6310_BUTTON_7        7

#define ADVERTISING_LED_PIN_NO  NRF6310_LED_0
#define CONNECTED_LED_PIN_NO    NRF6310_LED_1
#define ASSERT_LED_PIN_NO       NRF6310_LED_7

#endif // BLE_NRF6310_PINS_H__

/** @} */
