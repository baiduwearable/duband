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
 * @defgroup nrf_dfu_types Types and definitions.
 * @{
 *
 * @ingroup nrf_dfu
 *
 * @brief Device Firmware Update module type and definitions.
 */

#ifndef DFU_TYPES_H__
#define DFU_TYPES_H__

#include <stdint.h>
#include "nrf51.h"
#include "app_util.h"

#define NRF_UICR_BOOT_START_ADDRESS     (NRF_UICR_BASE + 0x14)                                  /**< Register where the bootloader start address is stored in the UICR register. */

#define PACKET_SIZE                     512                                                     /**< Size of each data packet. Also used for initial receiving of packets from transport layer. */
#define PACKET_HEADER_SIZE              sizeof(uint32_t)                                        /**< Size of the data packet header. */

#define CODE_REGION_1_START             0x00014000                                              /**< This field should correspond to the size of Code Region 0, (which is identical to Start of Code Region 1), found in UICR.CLEN0 register. This value is used for compile safety, as the linker will fail if application expands into bootloader. Runtime, the bootloader will use the value found in UICR.CLEN0. */

#define BOOTLOADER_REGION_START         0x0003C800                                              /**< This field should correspond to start address of the bootloader, found in UICR.RESERVED, 0x10001014, register. This value is used for sanity check, so the bootloader will fail immediately if this value differs from runtime value. The value is used to determine max application size for updating. */
#define BOOTLOADER_SETTINGS_ADDRESS     0x0003FC00                                              /**< The field specifies the page location of the bootloader settings address. */

#define DFU_REGION_TOTAL_SIZE           (BOOTLOADER_REGION_START - CODE_REGION_1_START)         /**< Total size of the region between SD and Bootloader. */

/***********************************************************
* size reserved for application data is 42K
************************************************************/
#define DFU_APP_DATA_RESERVED           0xA800                                                  /**< Size of Application Data that must be preserved between application updates. This value must be a multiple of page size. Page size is 0x400 (1024d) bytes, thus this value must be 0x0000, 0x0400, 0x0800, 0x0C00, 0x1000, etc. */
#define DFU_IMAGE_MAX_SIZE_FULL         (DFU_REGION_TOTAL_SIZE - DFU_APP_DATA_RESERVED)         /**< Maximum size of a application, excluding save data from the application. */
/************************************************************
* we only has one app data region
*************************************************************/
#define DFU_IMAGE_MAX_SIZE_BANKED       ((DFU_REGION_TOTAL_SIZE - DFU_APP_DATA_RESERVED)/2)     /**< Maximum size of a application, excluding save data from the application. */

#define DFU_BANK_0_REGION_START         CODE_REGION_1_START                                     /**< Bank 0 region start. */
#define DFU_BANK_1_REGION_START         (DFU_BANK_0_REGION_START + DFU_IMAGE_MAX_SIZE_BANKED)   /**< Bank 1 region start. */

#define CODE_PAGE_SIZE                  1024                                                    /**< Size of a flash codepage. Used for size of the reserved flash space in the bootloader region. Will be runtime checked against NRF_UICR->CODEPAGESIZE to ensure the region is correct. */
#define EMPTY_FLASH_MASK                0xFFFFFFFF                                              /**< Bit mask that defines an empty address in flash. */

#define INVALID_PACKET                  0x00                                                    /**< Invalid packet identifies. */
#define INIT_PACKET                     0x01                                                    /**< Packet identifies for initialization packet. */
#define START_DATA_PACKET               0x02                                                    /**< Packet identifies for the Data Start Packet. */
#define DATA_PACKET                     0x03                                                    /**< Packet identifies for a Data Packet. */
#define STOP_DATA_PACKET                0x04                                                    /**< Packet identifies for the Data Stop Packet. */

// Safe guard to ensure during compile time that the DFU_APP_DATA_RESERVED is a multiple of page size.
STATIC_ASSERT((((DFU_APP_DATA_RESERVED) & (CODE_PAGE_SIZE - 1)) == 0x00));

/**@brief Structure holding a bootloader packet received on the UART.
 */
typedef struct
{
    uint32_t   packet_type;                                                            /**< Packet type, used to identify the content of the received packet referenced by data packet. */
    uint32_t   packet_length;                                                          /**< Packet length of the data packet. Each data is word size, meaning length of 4 is 4 words, not bytes. */
    uint32_t * p_data_packet;                                                          /**< Data Packet received. Each data is a word size entry. */
} dfu_update_packet_t;

/**@brief DFU status error codes.
*/
typedef enum
{
    DFU_UPDATE_COMPLETE,                                                                        /**< Status update complete.*/
    DFU_BANK_0_ERASED,                                                                          /**< Status bank 0 erased.*/
    DFU_BANK_1_ERASED,                                                                          /**< Status bank 1 erased.*/
    DFU_TIMEOUT                                                                                 /**< Status timeout.*/
} dfu_update_status_code_t;

/**@brief Structure holding DFU complete event.
*/
typedef struct
{
    dfu_update_status_code_t status_code;                                                       /**< Device Firmware Update status. */
    uint16_t                 app_crc;                                                           /**< CRC of the recieved application. */
    uint32_t                 app_size;                                                          /**< Size of the recieved application. */
} dfu_update_status_t;

/**@brief Update complete handler type. */
typedef void (*dfu_complete_handler_t)(dfu_update_status_t dfu_update_status);

#endif // DFU_TYPES_H__

/**@} */
