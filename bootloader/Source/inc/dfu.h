/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
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
 
/**@file
 *
 * @defgroup nrf_dfu Device Firmware Update API.
 * @{     
 *
 * @brief Device Firmware Update module interface.
 */

#ifndef DFU_H__
#define DFU_H__

#include "dfu_types.h"
#include <stdbool.h>
#include <stdint.h>

/**@brief Function for initializing the Device Firmware Update module.
 * 
 * @return    NRF_SUCCESS on success, an error_code otherwise.
 */
uint32_t dfu_init(void);

/**@brief Function for setting the DFU image size. 
 *
 * @details Function sets the DFU image size. This function must be called when an update is started 
 *          in order to notify the DFU of the new image size.
 * 
 * @param[in] image_size Size of the image to be transmitted.
 *
 * @return    NRF_SUCCESS on success, an error_code otherwise.
 */
uint32_t dfu_image_size_set(uint32_t image_size);

/**@brief Function for handling DFU data packets.
 *
 * @param[in] p_packet   Pointer to the DFU packet.
 *
 * @return    NRF_SUCCESS on success, an error_code otherwise.
 */
uint32_t dfu_data_pkt_handle(dfu_update_packet_t * p_packet);

/**@brief Function for handling DFU init packets.
 *
 * @return    NRF_SUCCESS on success, an error_code otherwise.
 */
uint32_t dfu_init_pkt_handle(dfu_update_packet_t * p_packet);

/**@brief Function for validating a transferred image after the transfer has completed.
 * 
 * @return    NRF_SUCCESS on success, an error_code otherwise.
 */
uint32_t dfu_image_validate(void);

/**@brief Function for activating the transfered image after validation has successfully completed.
 *
 * @return    NRF_SUCCESS on success, an error_code otherwise.
 */
uint32_t dfu_image_activate(void);

#endif // DFU_H__

/** @} */
