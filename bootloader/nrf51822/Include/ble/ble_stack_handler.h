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
 * @defgroup ble_stack_handler BLE Stack Event Handler
 * @{
 * @ingroup ble_sdk_lib
 * @brief Module for initializing the BLE stack, and propagating BLE stack events to the
 *        application.
 *
 * @details Use the USE_SCHEDULER parameter of the BLE_STACK_HANDLER_INIT() macro to select if the
 *          @ref app_scheduler is to be used or not.
 *
 * @note Even if the scheduler is not used, ble_stack_handler.h will include app_scheduler.h,
 *       so when compiling, app_scheduler.h must be available in one of the compiler include paths.
 */

#ifndef BLE_STACK_HANDLER_H__
#define BLE_STACK_HANDLER_H__

#include <stdlib.h>
#include "ble.h"
#include "nrf51.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "app_util.h"
#include "compiler_abstraction.h"

#define BLE_STACK_HANDLER_SCHED_EVT_SIZE  0     /**< Size of button events being passed through the scheduler (is to be used for computing the maximum size of scheduler events). For BLE stack events, this size is 0, since the events are being pulled in the event handler. */

/**@brief Application stack event handler type. */
typedef void (*ble_stack_evt_handler_t) (ble_evt_t * p_ble_evt);

/**@brief Type of function for passing events from the BLE stack handler module to the scheduler. */
typedef uint32_t (*ble_stack_evt_schedule_func_t) (void);

/**@brief Macro for initializing the BLE stack event handler.
 *
 * @details It will handle dimensioning and allocation of the memory buffer required for reading
 *          BLE events from the stack, making sure the buffer is correctly aligned. It will also
 *          connect the BLE stack event handler to the scheduler (if specified).
 *
 * @param[in] CLOCK_SOURCE    Low frequency clock source and accuracy (type nrf_clock_lfclksrc_t,
 *                            see sd_softdevice_enable() for details).
 * @param[in] MTU_SIZE        Maximum size of BLE transmission units to be used by this application.
 * @param[in] EVT_HANDLER     Pointer to function to be executed when an event has been received.
 * @param[in] USE_SCHEDULER   TRUE if the application is using the event scheduler, FALSE otherwise.
 *
 * @note Since this macro allocates a buffer, it must only be called once (it is OK to call it
 *       several times as long as it is from the same location, e.g. to do a reinitialization).
 */
/*lint -emacro(506, BLE_STACK_HANDLER_INIT) */ /* Suppress "Constant value Boolean */
#define BLE_STACK_HANDLER_INIT(CLOCK_SOURCE, MTU_SIZE, EVT_HANDLER, USE_SCHEDULER)                 \
    do                                                                                             \
    {                                                                                              \
        static uint32_t EVT_BUFFER[CEIL_DIV(sizeof(ble_evt_t) + (MTU_SIZE), sizeof(uint32_t))];    \
        uint32_t        ERR_CODE;                                                                  \
        ERR_CODE = ble_stack_handler_init((CLOCK_SOURCE),                                          \
                                          EVT_BUFFER,                                              \
                                          sizeof(EVT_BUFFER),                                      \
                                          (EVT_HANDLER),                                           \
                                          (USE_SCHEDULER) ? ble_stack_evt_schedule : NULL);        \
        APP_ERROR_CHECK(ERR_CODE);                                                                 \
    } while (0)

/**@brief Function for initializing stack module.
 *
 * @details Enables the SoftDevice and the BLE stack event interrupt handler.
 *
 * @note This function must be called before calling any function in the SoftDevice API.
 *
 * @note Normally initialization should be done using the BLE_STACK_HANDLER_INIT() macro, as that
 *       will both allocate the BLE event buffer, and also align the buffer correctly.
 *
 * @param[in]  clock_source        Low frequency clock source to be used by the SoftDevice.
 * @param[in]  p_evt_buffer        Buffer for holding one BLE stack event. Since we are
 *                                 not using the heap, this buffer must be provided by
 *                                 the application. The buffer must be large enough to hold
 *                                 the biggest stack event the application is supposed to
 *                                 handle. The buffer must be aligned to a 4 byte boundary.
 * @param[in]  evt_buffer_size     Size of BLE stack event buffer.
 * @param[in]  evt_handler         Handler to be called for each received BLE stack event.
 * @param[in]  evt_schedule_func   Function for passing BLE events to the scheduler. Point to
 *                                 ble_stack_evt_schedule() to connect to the scheduler. Set to NULL
 *                                 to make the BLE stack handler module call the event handler
 *                                 directly from the BLE stack event interrupt handler.
 *
 * @retval     NRF_SUCCESS               Successful initialization.
 * @retval     NRF_ERROR_INVALID_PARAM   Invalid parameter (buffer not aligned to a 4 byte
 *                                       boundary) or NULL.
 */
uint32_t ble_stack_handler_init(nrf_clock_lfclksrc_t          clock_source,
                                void *                        p_evt_buffer,
                                uint16_t                      evt_buffer_size,
                                ble_stack_evt_handler_t       evt_handler,
                                ble_stack_evt_schedule_func_t evt_schedule_func);


// Functions for connecting the BLE Stack Event Handler to the scheduler:

/**@cond NO_DOXYGEN */
void intern_ble_stack_events_execute(void);

static __INLINE void ble_stack_evt_get(void * p_event_data, uint16_t event_size)
{
    APP_ERROR_CHECK_BOOL(event_size == 0);
    intern_ble_stack_events_execute();
}

static __INLINE uint32_t ble_stack_evt_schedule(void)
{
    return app_sched_event_put(NULL, 0, ble_stack_evt_get);
}
/**@endcond */

#endif // BLE_STACK_HANDLER_H__

/** @} */
