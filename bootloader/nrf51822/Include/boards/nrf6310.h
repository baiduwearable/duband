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
#ifndef NRF6310_H
#define NRF6310_H

#define LED_START      8
#define LED_STOP       15
#define LED_PORT       NRF_GPIO_PORT_SELECT_PORT1
#define LED_OFFSET     0

#define LED0           8
#define LED1           9
#define LED2           10
#define LED3           11
#define LED4           12
#define LED5           13
#define LED6           14
#define LED7           15

#define BUTTON_START   0
#define BUTTON_STOP    7
#define BUTTON0        0
#define BUTTON1        1
#define BUTTON2        2
#define BUTTON3        3
#define BUTTON4        4
#define BUTTON5        5
#define BUTTON6        6
#define BUTTON7        7



#define RX_PIN_NUMBER  16    // UART RX pin number.
#define TX_PIN_NUMBER  17    // UART TX pin number.
#define CTS_PIN_NUMBER 18    // UART Clear To Send pin number. Not used if HWFC is set to false
#define RTS_PIN_NUMBER 19    // Not used if HWFC is set to false
#define HWFC           false // UART hardware flow control

#define BLINKY_STATE_MASK   0x07

#endif
