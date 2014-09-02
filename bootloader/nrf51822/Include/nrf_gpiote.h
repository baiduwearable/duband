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
#ifndef NRF_GPIOTE_H__
#define NRF_GPIOTE_H__

#include "nrf51.h"

/**
* @defgroup nrf_gpiote GPIOTE abstraction
* @{
* @ingroup nrf_drivers
* @brief GPIOTE abstraction for configuration of channels.
*/


 /**
 * @enum nrf_gpiote_polarity_t
 * @brief Polarity for GPIOTE channel enumerator.
 */
typedef enum
{
  NRF_GPIOTE_POLARITY_LOTOHI = GPIOTE_CONFIG_POLARITY_LoToHi,       ///<  Low to high
  NRF_GPIOTE_POLARITY_HITOLO = GPIOTE_CONFIG_POLARITY_HiToLo,       ///<  High to low
  NRF_GPIOTE_POLARITY_TOGGLE = GPIOTE_CONFIG_POLARITY_Toggle        ///<  Toggle
} nrf_gpiote_polarity_t;


 /**
 * @enum nrf_gpiote_outinit_t
 * @brief Initial output value for GPIOTE channel enumerator.
 */
typedef enum
{
  NRF_GPIOTE_INITIAL_VALUE_LOW  = GPIOTE_CONFIG_OUTINIT_Low,       ///<  Low to high
  NRF_GPIOTE_INITIAL_VALUE_HIGH = GPIOTE_CONFIG_OUTINIT_High       ///<  High to low
} nrf_gpiote_outinit_t;


/**
 * @brief Function for configuring GPIOTE channel as output, setting the properly desired output level.
 *
 *
 * @param channel_number specifies the GPIOTE channel [0:3] to configure as an output channel.
 * @param pin_number specifies the pin number [0:30] to use in the GPIOTE channel.
 * @param polarity specifies the desired polarity in the output GPIOTE channel.
 * @param initial_value specifies the initial value of the GPIOTE channel input after the channel configuration.
 */
static __INLINE void nrf_gpiote_task_config(uint32_t channel_number, uint32_t pin_number, nrf_gpiote_polarity_t polarity, nrf_gpiote_outinit_t initial_value)
{
    /* Check if the output desired is high or low */
    if (initial_value == NRF_GPIOTE_INITIAL_VALUE_LOW)
    {
        /* Workaround for the OUTINIT PAN. When nrf_gpiote_task_config() is called a glitch happens
        on the GPIO if the GPIO in question is already assigned to GPIOTE and the pin is in the 
        correct state in GPIOTE but not in the OUT register. */
        NRF_GPIO->OUTCLR = (1 << pin_number);
        
        /* Configure channel to Pin31, not connected to the pin, and configure as a tasks that will set it to proper level */
        NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Task       << GPIOTE_CONFIG_MODE_Pos)     |
                                             (31UL                          << GPIOTE_CONFIG_PSEL_Pos)     |
                                             (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos);                                    
    } 
    else 
    {
        /* Workaround for the OUTINIT PAN. When nrf_gpiote_task_config() is called a glitch happens
        on the GPIO if the GPIO in question is already assigned to GPIOTE and the pin is in the 
        correct state in GPIOTE but not in the OUT register. */
        NRF_GPIO->OUTSET = (1 << pin_number);
        
        /* Configure channel to Pin31, not connected to the pin, and configure as a tasks that will set it to proper level */
        NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Task       << GPIOTE_CONFIG_MODE_Pos)     |
                                             (31UL                          << GPIOTE_CONFIG_PSEL_Pos)     |
                                             (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos);
    }

    /* Three NOPs are required to make sure configuration is written before setting tasks or getting events */
    __NOP();
    __NOP();
    __NOP(); 

    /* Launch the task to take the GPIOTE channel output to the desired level */
    NRF_GPIOTE->TASKS_OUT[channel_number] = 1;
    

    /* Finally configure the channel as the caller expects. If OUTINIT works, the channel is configured properly. 
       If it does not, the channel output inheritance sets the proper level. */
    NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos)     |
                                         ((uint32_t)pin_number    << GPIOTE_CONFIG_PSEL_Pos)     |
                                         ((uint32_t)polarity      << GPIOTE_CONFIG_POLARITY_Pos) |
                                         ((uint32_t)initial_value << GPIOTE_CONFIG_OUTINIT_Pos);

    /* Three NOPs are required to make sure configuration is written before setting tasks or getting events */
    __NOP();
    __NOP();
    __NOP(); 
}

/**
 * @brief Function for configuring GPIOTE channel as input, automatically clearing an event that appears in some cases under configuration.
 *
 * Note that the pin must be configured as connected input for this
 * function to have any effect.
 *
 * Note that when configuring the channel as input an event might be triggered. Care of disabling interrupts
 * for that channel is left to the user.
 *
 * @param channel_number specifies the GPIOTE channel [0:3] to configure as an input channel.
 * @param pin_number specifies the pin number [0:30] to use in the GPIOTE channel.
 * @param polarity specifies the desired polarity in the output GPIOTE channel.
 */
static __INLINE void nrf_gpiote_event_config(uint32_t channel_number, uint32_t pin_number, nrf_gpiote_polarity_t polarity)
{   
    /* Configure the channel as the caller expects */
    NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos)     |
                                         ((uint32_t)pin_number     << GPIOTE_CONFIG_PSEL_Pos)     |
                                         ((uint32_t)polarity       << GPIOTE_CONFIG_POLARITY_Pos);

    /* Three NOPs are required to make sure configuration is written before setting tasks or getting events */
    __NOP();
    __NOP();
    __NOP();
    
    /* Clear the event that appears in some cases */
    NRF_GPIOTE->EVENTS_IN[channel_number] = 0; 
}


/**
 * @brief Function for unconfiguring GPIOTE channel.
 *
 *
 * Note that when unconfiguring the channel, the pin is configured as GPIO PIN_CNF configuration.
 *
 * @param channel_number specifies the GPIOTE channel [0:3] to unconfigure.
 */
static __INLINE void nrf_gpiote_unconfig(uint32_t channel_number)
{   
    /* Unonfigure the channel as the caller expects */
    NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Disabled   << GPIOTE_CONFIG_MODE_Pos) |
                                         (31UL                          << GPIOTE_CONFIG_PSEL_Pos) |
                                         (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
}


/** @} */

#endif
