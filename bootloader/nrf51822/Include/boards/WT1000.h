#ifndef _WT1000_H_
#define _WT1000_H_

#define BAIDU_LED_0           7
#define BAIDU_LED_1           19
#define BAIDU_LED_2           18
#define BAIDU_LED_3           5
#define BAIDU_LED_4           6
#define BAIDU_MOTOR_0         29


#define ADVERTISING_LED_PIN_NO  	BAIDU_LED_0
#define CONNECTED_LED_PIN_NO    	BAIDU_LED_1
#define ASSERT_LED_PIN_NO       	BAIDU_LED_2

#define LED0    BAIDU_LED_0
#define LED1    BAIDU_LED_1
#define LED2    BAIDU_LED_2
#define LED3    BAIDU_LED_3
#define LED4    BAIDU_LED_4

/* charger */
#define CHARGER_CONNECTED_PIN 1
#define CHARGER_CHARGING_PIN 17

#ifdef DEBUG_LOG
  #define RX_PIN_NUMBER  25    // UART RX pin number.
  #define TX_PIN_NUMBER  23    // UART TX pin number.
	//the follow pin is useless
  #define CTS_PIN_NUMBER 43    // UART Clear To Send pin number. Not used if HWFC is set to false
  #define RTS_PIN_NUMBER 43    // Not used if HWFC is set to false
  #define HWFC           false // UART hardware flow control
#endif

#endif //_WT1000_H_
