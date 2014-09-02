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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIS3DH_DRIVER__H
#define __LIS3DH_DRIVER__H
#include <stdbool.h>
#include <stdint.h>

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

//these could change accordingly with the architecture

#ifndef __ARCHDEP__TYPES
#define __ARCHDEP__TYPES

typedef unsigned char u8_t;
typedef unsigned short int u16_t;
typedef short int i16_t;
typedef signed char i8_t;

#endif /*__ARCHDEP__TYPES*/

typedef u8_t LIS3DH_IntPinConf_t;
typedef u8_t LIS3DH_Axis_t;
typedef u8_t LIS3DH_Int1Conf_t;
#define LIS3DH_FULLSCALE LIS3DH_FULLSCALE_4

#define LIS3DH_READBIT                (0x80)
#define LIS3DH_MSBIT                (0x40)
#define BIT_0 0x01 /**< The value of bit 0 */
#define BIT_1 0x02 /**< The value of bit 1 */
#define BIT_2 0x04 /**< The value of bit 2 */
#define BIT_3 0x08 /**< The value of bit 3 */
#define BIT_4 0x10 /**< The value of bit 4 */
#define BIT_5 0x20 /**< The value of bit 5 */
#define BIT_6 0x40 /**< The value of bit 6 */
#define BIT_7 0x80 /**< The value of bit 7 */

// Bit twiddling keys for use with different registers
enum
{
    LIS3DH_STATUS_REG_ZYXDA             = 0x08,   // STATUS_REG: XYZ - sample ready
    LIS3DH_STATUS_REG_ZYXOR             = 0x80,   // STATUS_REG: XYZ - new set of data has overwritten the previous ones
    LIS3DH_CTRL_REG1_XEN                = 0x01,   // CTRL_REG1: X enable
    LIS3DH_CTRL_REG1_YEN                = 0x02,   // CTRL_REG1: Y enable
    LIS3DH_CTRL_REG1_ZEN                = 0x04,   // CTRL_REG1: Z enable
    LIS3DH_CTRL_REG1_XYZEN              = 0x07,   // CTRL_REG1: X+Y+Z enable
    LIS3DH_CTRL_REG1_LPEN               = 0x08,   // CTRL_REG1: Low power enable
    LIS3DH_CTRL_REG1_DATARATE_POWERDOWN = 0x00,   // CTRL_REG1: 0000 xxxx
    LIS3DH_CTRL_REG1_DATARATE_1HZ       = 0x10,   // CTRL_REG1: 0001 xxxx
    LIS3DH_CTRL_REG1_DATARATE_10HZ      = 0x20,   // CTRL_REG1: 0010 xxxx
    LIS3DH_CTRL_REG1_DATARATE_25HZ      = 0x30,   // CTRL_REG1: 0011 xxxx
    LIS3DH_CTRL_REG1_DATARATE_50HZ      = 0x40,   // CTRL_REG1: 0100 xxxx
    LIS3DH_CTRL_REG1_DATARATE_100HZ     = 0x50,   // CTRL_REG1: 0101 xxxx
    LIS3DH_CTRL_REG1_DATARATE_200HZ     = 0x60,   // CTRL_REG1: 0110 xxxx
    LIS3DH_CTRL_REG1_DATARATE_400HZ     = 0x70,   // CTRL_REG1: 0111 xxxx
    LIS3DH_CTRL_REG1_DATARATE_1500HZ    = 0x80,   // CTRL_REG1: 1000 xxxx
    LIS3DH_CTRL_REG1_DATARATE_5000HZ    = 0x90,   // CTRL_REG1: 1001 xxxx
    LIS3DH_CTRL_REG4_BLOCKDATAUPDATE    = 0x80,   // CTRL_REG4: 1xxx xxxx
    LIS3DH_CTRL_REG4_SCALE_2G           = 0x00,   // CTRL_REG4: xx00 xxxx
    LIS3DH_CTRL_REG4_SCALE_4G           = 0x10,   // CTRL_REG4: xx01 xxxx
    LIS3DH_CTRL_REG4_SCALE_8G           = 0x20,   // CTRL_REG4: xx10 xxxx
    LIS3DH_CTRL_REG4_SCALE_16G          = 0x30,   // CTRL_REG4: xx11 xxxx
};

//define structure
#ifndef __SHARED__TYPES
#define __SHARED__TYPES

typedef enum {
    MEMS_SUCCESS    =  0x01,
    MEMS_ERROR    =  0x00
} status_t;

typedef enum {
    MEMS_ENABLE    =  0x01,
    MEMS_DISABLE    =  0x00
} State_t;

typedef struct
{
    i16_t AXIS_X;
    i16_t AXIS_Y;
    i16_t AXIS_Z;
}
AxesRaw_t;

#endif /*__SHARED__TYPES*/

typedef enum {
    LIS3DH_ODR_1Hz          =  0x01,
    LIS3DH_ODR_10Hz                      =  0x02,
    LIS3DH_ODR_25Hz          =  0x03,
    LIS3DH_ODR_50Hz          =  0x04,
    LIS3DH_ODR_100Hz          =  0x05,
    LIS3DH_ODR_200Hz          =  0x06,
    LIS3DH_ODR_400Hz          =  0x07,
    LIS3DH_ODR_1620Hz_LP          =  0x08,
    LIS3DH_ODR_1344Hz_NP_5367HZ_LP       =  0x09
} LIS3DH_ODR_t;

typedef enum {
    LIS3DH_POWER_DOWN                    =  0x00,
    LIS3DH_LOW_POWER    =  0x01,
    LIS3DH_NORMAL   =  0x02
} LIS3DH_Mode_t;

typedef enum {
    LIS3DH_HPM_NORMAL_MODE_RES           =               0x00,
    LIS3DH_HPM_REF_SIGNAL                =               0x01,
    LIS3DH_HPM_NORMAL_MODE               =               0x02,
    LIS3DH_HPM_AUTORESET_INT             =               0x03
} LIS3DH_HPFMode_t;

typedef enum {
    LIS3DH_HPFCF_0                       =               0x00,
    LIS3DH_HPFCF_1                       =               0x01,
    LIS3DH_HPFCF_2                       =   0x02,
    LIS3DH_HPFCF_3                       =               0x03
} LIS3DH_HPFCutOffFreq_t;

typedef struct
{
    u16_t AUX_1;
    u16_t AUX_2;
    u16_t AUX_3;
}
LIS3DH_Aux123Raw_t;

typedef enum {
    LIS3DH_FULLSCALE_2                   =               0x00,
    LIS3DH_FULLSCALE_4                   =               0x01,
    LIS3DH_FULLSCALE_8                   =               0x02,
    LIS3DH_FULLSCALE_16                  =               0x03
} LIS3DH_Fullscale_t;

typedef enum {
    LIS3DH_BLE_LSB   =  0x00,
    LIS3DH_BLE_MSB   =  0x01
} LIS3DH_Endianess_t;

typedef enum {
    LIS3DH_SELF_TEST_DISABLE             =               0x00,
    LIS3DH_SELF_TEST_0                   =               0x01,
    LIS3DH_SELF_TEST_1                   =               0x02
} LIS3DH_SelfTest_t;

typedef enum {
    LIS3DH_FIFO_BYPASS_MODE              =               0x00,
    LIS3DH_FIFO_MODE                     =               0x01,
    LIS3DH_FIFO_STREAM_MODE              =               0x02,
    LIS3DH_FIFO_TRIGGER_MODE             =               0x03,
    LIS3DH_FIFO_DISABLE                  =               0x04
} LIS3DH_FifoMode_t;

typedef enum {
    LIS3DH_TRIG_INT1                     =  0x00,
    LIS3DH_TRIG_INT2    =  0x01
} LIS3DH_TrigInt_t;

typedef enum {
    LIS3DH_SPI_4_WIRE                    =               0x00,
    LIS3DH_SPI_3_WIRE                    =               0x01
} LIS3DH_SPIMode_t;

typedef enum {
    LIS3DH_X_ENABLE                      =               0x01,
    LIS3DH_X_DISABLE                     =               0x00,
    LIS3DH_Y_ENABLE                      =               0x02,
    LIS3DH_Y_DISABLE                     =               0x00,
    LIS3DH_Z_ENABLE                      =               0x04,
    LIS3DH_Z_DISABLE                     =               0x00
} LIS3DH_AXISenable_t;

typedef enum {
    LIS3DH_INT1_6D_4D_DISABLE            =               0x00,
    LIS3DH_INT1_6D_ENABLE                =               0x01,
    LIS3DH_INT1_4D_ENABLE                =               0x02
} LIS3DH_INT_6D_4D_t;

typedef enum {
    LIS3DH_UP_SX                         =               0x44,
    LIS3DH_UP_DX                         =               0x42,
    LIS3DH_DW_SX                         =               0x41,
    LIS3DH_DW_DX                         =               0x48,
    LIS3DH_TOP                           =               0x60,
    LIS3DH_BOTTOM                        =               0x50
} LIS3DH_POSITION_6D_t;

typedef enum {
    LIS3DH_INT_MODE_OR                   =               0x00,
    LIS3DH_INT_MODE_6D_MOVEMENT          =               0x01,
    LIS3DH_INT_MODE_AND                  =               0x02,
    LIS3DH_INT_MODE_6D_POSITION          =               0x03
} LIS3DH_Int1Mode_t;


//interrupt click response
//  b7 = don't care   b6 = IA  b5 = DClick  b4 = Sclick  b3 = Sign
//  b2 = z      b1 = y     b0 = x
typedef enum {
    LIS3DH_DCLICK_Z_P                      =               0x24,
    LIS3DH_DCLICK_Z_N                      =               0x2C,
    LIS3DH_SCLICK_Z_P                      =               0x14,
    LIS3DH_SCLICK_Z_N                      =               0x1C,
    LIS3DH_DCLICK_Y_P                      =               0x22,
    LIS3DH_DCLICK_Y_N                      =               0x2A,
    LIS3DH_SCLICK_Y_P                      =               0x12,
    LIS3DH_SCLICK_Y_N   =  0x1A,
    LIS3DH_DCLICK_X_P                      =               0x21,
    LIS3DH_DCLICK_X_N                      =               0x29,
    LIS3DH_SCLICK_X_P                      =               0x11,
    LIS3DH_SCLICK_X_N                      =               0x19,
    LIS3DH_NO_CLICK                        =               0x00
} LIS3DH_Click_Response;

//TODO: start from here and manage the shared macros etc before this

/* Exported constants --------------------------------------------------------*/

#ifndef __SHARED__CONSTANTS
#define __SHARED__CONSTANTS

#define MEMS_SET                                        0x01
#define MEMS_RESET                                      0x00

#endif /*__SHARED__CONSTANTS*/


//Register Definition
#define LIS3DH_WHO_AM_I    0x0F  // device identification register

// CONTROL REGISTER 1
#define LIS3DH_CTRL_REG1    0x20
#define LIS3DH_ODR_BIT            BIT(4)
#define LIS3DH_LPEN     BIT(3)
#define LIS3DH_ZEN     BIT(2)
#define LIS3DH_YEN     BIT(1)
#define LIS3DH_XEN     BIT(0)

//CONTROL REGISTER 2
#define LIS3DH_CTRL_REG2    0x21
#define LIS3DH_HPM         BIT(6)
#define LIS3DH_HPCF     BIT(4)
#define LIS3DH_FDS     BIT(3)
#define LIS3DH_HPCLICK     BIT(2)
#define LIS3DH_HPIS2     BIT(1)
#define LIS3DH_HPIS1     BIT(0)

//CONTROL REGISTER 3
#define LIS3DH_CTRL_REG3    0x22
#define LIS3DH_I1_CLICK    BIT(7)
#define LIS3DH_I1_AOI1     BIT(6)
#define LIS3DH_I1_AOI2            BIT(5)
#define LIS3DH_I1_DRDY1    BIT(4)
#define LIS3DH_I1_DRDY2    BIT(3)
#define LIS3DH_I1_WTM     BIT(2)
#define LIS3DH_I1_ORUN     BIT(1)

//CONTROL REGISTER 6
#define LIS3DH_CTRL_REG6    0x25
#define LIS3DH_I2_CLICK    BIT(7)
#define LIS3DH_I2_INT1     BIT(6)
#define LIS3DH_I2_BOOT            BIT(4)
#define LIS3DH_H_LACTIVE    BIT(1)

//TEMPERATURE CONFIG REGISTER
#define LIS3DH_TEMP_CFG_REG    0x1F
#define LIS3DH_ADC_PD            BIT(7)
#define LIS3DH_TEMP_EN     BIT(6)

//CONTROL REGISTER 4
#define LIS3DH_CTRL_REG4    0x23
#define LIS3DH_BDU     BIT(7)
#define LIS3DH_BLE     BIT(6)
#define LIS3DH_FS     BIT(4)
#define LIS3DH_HR     BIT(3)
#define LIS3DH_ST           BIT(1)
#define LIS3DH_SIM     BIT(0)

//CONTROL REGISTER 5
#define LIS3DH_CTRL_REG5    0x24
#define LIS3DH_BOOT                                    BIT(7)
#define LIS3DH_FIFO_EN                                 BIT(6)
#define LIS3DH_LIR_INT1                                BIT(3)
#define LIS3DH_D4D_INT1                                BIT(2)

//REFERENCE/DATA_CAPTURE
#define LIS3DH_REFERENCE_REG                  0x26
#define LIS3DH_REF                   BIT(0)

//STATUS_REG_AXIES
#define LIS3DH_STATUS_REG    0x27
#define LIS3DH_ZYXOR                                   BIT(7)
#define LIS3DH_ZOR                                     BIT(6)
#define LIS3DH_YOR                                     BIT(5)
#define LIS3DH_XOR                                     BIT(4)
#define LIS3DH_ZYXDA                                   BIT(3)
#define LIS3DH_ZDA                                     BIT(2)
#define LIS3DH_YDA                                     BIT(1)
#define LIS3DH_XDA                                     BIT(0)

//STATUS_REG_AUX
#define LIS3DH_STATUS_AUX    0x07

//INTERRUPT 1 CONFIGURATION
#define LIS3DH_INT1_CFG    0x30
#define LIS3DH_ANDOR                                   BIT(7)
#define LIS3DH_INT_6D                                  BIT(6)
#define LIS3DH_ZHIE                                    BIT(5)
#define LIS3DH_ZLIE                                    BIT(4)
#define LIS3DH_YHIE                                    BIT(3)
#define LIS3DH_YLIE                                    BIT(2)
#define LIS3DH_XHIE                                    BIT(1)
#define LIS3DH_XLIE                                    BIT(0)

//FIFO CONTROL REGISTER
#define LIS3DH_FIFO_CTRL_REG                           0x2E
#define LIS3DH_FM                                      BIT(6)
#define LIS3DH_TR                                      BIT(5)
#define LIS3DH_FTH                                     BIT(0)

//CONTROL REG3 bit mask
#define LIS3DH_CLICK_ON_PIN_INT1_ENABLE                0x80
#define LIS3DH_CLICK_ON_PIN_INT1_DISABLE               0x00
#define LIS3DH_I1_INT1_ON_PIN_INT1_ENABLE              0x40
#define LIS3DH_I1_INT1_ON_PIN_INT1_DISABLE             0x00
#define LIS3DH_I1_INT2_ON_PIN_INT1_ENABLE              0x20
#define LIS3DH_I1_INT2_ON_PIN_INT1_DISABLE             0x00
#define LIS3DH_I1_DRDY1_ON_INT1_ENABLE                 0x10
#define LIS3DH_I1_DRDY1_ON_INT1_DISABLE                0x00
#define LIS3DH_I1_DRDY2_ON_INT1_ENABLE                 0x08
#define LIS3DH_I1_DRDY2_ON_INT1_DISABLE                0x00
#define LIS3DH_WTM_ON_INT1_ENABLE                      0x04
#define LIS3DH_WTM_ON_INT1_DISABLE                     0x00
#define LIS3DH_INT1_OVERRUN_ENABLE                     0x02
#define LIS3DH_INT1_OVERRUN_DISABLE                    0x00

//CONTROL REG6 bit mask
#define LIS3DH_CLICK_ON_PIN_INT2_ENABLE                0x80
#define LIS3DH_CLICK_ON_PIN_INT2_DISABLE               0x00
#define LIS3DH_I2_INT1_ON_PIN_INT2_ENABLE              0x40
#define LIS3DH_I2_INT1_ON_PIN_INT2_DISABLE             0x00
#define LIS3DH_I2_INT2_ON_PIN_INT2_ENABLE              0x20
#define LIS3DH_I2_INT2_ON_PIN_INT2_DISABLE             0x00
#define LIS3DH_I2_BOOT_ON_INT2_ENABLE                  0x10
#define LIS3DH_I2_BOOT_ON_INT2_DISABLE                 0x00
#define LIS3DH_INT_ACTIVE_HIGH                         0x00
#define LIS3DH_INT_ACTIVE_LOW                          0x02

//INT1_CFG bit mask
#define LIS3DH_INT1_AND                                0x80
#define LIS3DH_INT1_OR                                 0x00
#define LIS3DH_INT1_ZHIE_ENABLE                        0x20
#define LIS3DH_INT1_ZHIE_DISABLE                       0x00
#define LIS3DH_INT1_ZLIE_ENABLE                        0x10
#define LIS3DH_INT1_ZLIE_DISABLE                       0x00
#define LIS3DH_INT1_YHIE_ENABLE                        0x08
#define LIS3DH_INT1_YHIE_DISABLE                       0x00
#define LIS3DH_INT1_YLIE_ENABLE                        0x04
#define LIS3DH_INT1_YLIE_DISABLE                       0x00
#define LIS3DH_INT1_XHIE_ENABLE                        0x02
#define LIS3DH_INT1_XHIE_DISABLE                       0x00
#define LIS3DH_INT1_XLIE_ENABLE                        0x01
#define LIS3DH_INT1_XLIE_DISABLE                       0x00

//INT1_SRC bit mask
#define LIS3DH_INT1_SRC_IA                             0x40
#define LIS3DH_INT1_SRC_ZH                             0x20
#define LIS3DH_INT1_SRC_ZL                             0x10
#define LIS3DH_INT1_SRC_YH                             0x08
#define LIS3DH_INT1_SRC_YL                             0x04
#define LIS3DH_INT1_SRC_XH                             0x02
#define LIS3DH_INT1_SRC_XL                             0x01

//INT1 REGISTERS
#define LIS3DH_INT1_THS                                0x32
#define LIS3DH_INT1_DURATION                           0x33

//INTERRUPT 1 SOURCE REGISTER
#define LIS3DH_INT1_SRC    0x31

//FIFO Source Register bit Mask
#define LIS3DH_FIFO_SRC_WTM                            0x80
#define LIS3DH_FIFO_SRC_OVRUN                          0x40
#define LIS3DH_FIFO_SRC_EMPTY                          0x20

//INTERRUPT CLICK REGISTER
#define LIS3DH_CLICK_CFG    0x38
//INTERRUPT CLICK CONFIGURATION bit mask
#define LIS3DH_ZD_ENABLE                               0x20
#define LIS3DH_ZD_DISABLE                              0x00
#define LIS3DH_ZS_ENABLE                               0x10
#define LIS3DH_ZS_DISABLE                              0x00
#define LIS3DH_YD_ENABLE                               0x08
#define LIS3DH_YD_DISABLE                              0x00
#define LIS3DH_YS_ENABLE                               0x04
#define LIS3DH_YS_DISABLE                              0x00
#define LIS3DH_XD_ENABLE                               0x02
#define LIS3DH_XD_DISABLE                              0x00
#define LIS3DH_XS_ENABLE                               0x01
#define LIS3DH_XS_DISABLE                              0x00

//INTERRUPT CLICK SOURCE REGISTER
#define LIS3DH_CLICK_SRC                               0x39
//INTERRUPT CLICK SOURCE REGISTER bit mask
#define LIS3DH_IA                                      0x40
#define LIS3DH_DCLICK                                  0x20
#define LIS3DH_SCLICK                                  0x10
#define LIS3DH_CLICK_SIGN                              0x08
#define LIS3DH_CLICK_Z                                 0x04
#define LIS3DH_CLICK_Y                                 0x02
#define LIS3DH_CLICK_X                                 0x01

//Click-click Register
#define LIS3DH_CLICK_THS                               0x3A
#define LIS3DH_TIME_LIMIT                              0x3B
#define LIS3DH_TIME_LATENCY                            0x3C
#define LIS3DH_TIME_WINDOW                             0x3D

//OUTPUT REGISTER
#define LIS3DH_OUT_X_L     0x28
#define LIS3DH_OUT_X_H     0x29
#define LIS3DH_OUT_Y_L     0x2A
#define LIS3DH_OUT_Y_H     0x2B
#define LIS3DH_OUT_Z_L     0x2C
#define LIS3DH_OUT_Z_H     0x2D

//AUX REGISTER
#define LIS3DH_OUT_1_L     0x08
#define LIS3DH_OUT_1_H     0x09
#define LIS3DH_OUT_2_L     0x0A
#define LIS3DH_OUT_2_H     0x0B
#define LIS3DH_OUT_3_L     0x0C
#define LIS3DH_OUT_3_H     0x0D

//STATUS REGISTER bit mask
#define LIS3DH_STATUS_REG_ZYXOR                        0x80    // 1 : new data set has over written the previous one
// 0 : no overrun has occurred (default)
#define LIS3DH_STATUS_REG_ZOR                          0x40    // 0 : no overrun has occurred (default)
// 1 : new Z-axis data has over written the previous one
#define LIS3DH_STATUS_REG_YOR                          0x20    // 0 : no overrun has occurred (default)
// 1 : new Y-axis data has over written the previous one
#define LIS3DH_STATUS_REG_XOR                          0x10    // 0 : no overrun has occurred (default)
// 1 : new X-axis data has over written the previous one
#define LIS3DH_STATUS_REG_ZYXDA                        0x08    // 0 : a new set of data is not yet avvious one
// 1 : a new set of data is available
#define LIS3DH_STATUS_REG_ZDA                          0x04    // 0 : a new data for the Z-Axis is not availvious one
// 1 : a new data for the Z-Axis is available
#define LIS3DH_STATUS_REG_YDA                          0x02    // 0 : a new data for the Y-Axis is not available
// 1 : a new data for the Y-Axis is available
#define LIS3DH_STATUS_REG_XDA                          0x01    // 0 : a new data for the X-Axis is not available

#define LIS3DH_DATAREADY_BIT                           LIS3DH_STATUS_REG_ZYXDA


//STATUS AUX REGISTER bit mask
#define LIS3DH_STATUS_AUX_321OR                         0x80
#define LIS3DH_STATUS_AUX_3OR                           0x40
#define LIS3DH_STATUS_AUX_2OR                           0x20
#define LIS3DH_STATUS_AUX_1OR                           0x10
#define LIS3DH_STATUS_AUX_321DA                         0x08
#define LIS3DH_STATUS_AUX_3DA                           0x04
#define LIS3DH_STATUS_AUX_2DA                           0x02
#define LIS3DH_STATUS_AUX_1DA                           0x01

#define LIS3DH_MEMS_I2C_ADDRESS           0x33

//FIFO REGISTERS
#define LIS3DH_FIFO_CTRL_REG           0x2E
#define LIS3DH_FIFO_SRC_REG           0x2F


/* Exported macro ------------------------------------------------------------*/

#ifndef __SHARED__MACROS

#define __SHARED__MACROS
#define ValBit(VAR,Place)         (VAR & (1<<Place))
#define BIT(x) ( (x) )

#endif /*__SHARED__MACROS*/

/* Exported functions --------------------------------------------------------*/
//Sensor Configuration Functions
status_t LIS3DH_SetODR(LIS3DH_ODR_t ov);
status_t LIS3DH_SetMode(LIS3DH_Mode_t md);
status_t LIS3DH_SetAxis(LIS3DH_Axis_t axis);
status_t LIS3DH_SetFullScale(LIS3DH_Fullscale_t fs);
status_t LIS3DH_SetBDU(State_t bdu);
status_t LIS3DH_SetBLE(LIS3DH_Endianess_t ble);
status_t LIS3DH_SetSelfTest(LIS3DH_SelfTest_t st);
status_t LIS3DH_SetTemperature(State_t state);
status_t LIS3DH_SetADCAux(State_t state);

//Filtering Functions
status_t LIS3DH_HPFClickEnable(State_t hpfe);
status_t LIS3DH_HPFAOI1Enable(State_t hpfe);
status_t LIS3DH_HPFAOI2Enable(State_t hpfe);
status_t LIS3DH_SetHPFMode(LIS3DH_HPFMode_t hpf);
status_t LIS3DH_SetHPFCutOFF(LIS3DH_HPFCutOffFreq_t hpf);
status_t LIS3DH_SetFilterDataSel(State_t state);

//Interrupt Functions
status_t LIS3DH_SetInt1Pin(LIS3DH_IntPinConf_t pinConf);
status_t LIS3DH_SetInt2Pin(LIS3DH_IntPinConf_t pinConf);
status_t LIS3DH_Int1LatchEnable(State_t latch);
status_t LIS3DH_ResetInt1Latch(void);
status_t LIS3DH_SetIntConfiguration(LIS3DH_Int1Conf_t ic);
status_t LIS3DH_SetInt1Threshold(u8_t ths);
status_t LIS3DH_SetInt1Duration(LIS3DH_Int1Conf_t id);
status_t LIS3DH_SetIntMode(LIS3DH_Int1Mode_t ic);
status_t LIS3DH_SetClickCFG(u8_t status);
status_t LIS3DH_SetInt6D4DConfiguration(LIS3DH_INT_6D_4D_t ic);
status_t LIS3DH_GetInt1Src(u8_t* val);
status_t LIS3DH_GetInt1SrcBit(u8_t statusBIT, u8_t* val);

//FIFO Functions
status_t LIS3DH_FIFOModeEnable(LIS3DH_FifoMode_t fm);
status_t LIS3DH_SetWaterMark(u8_t wtm);
status_t LIS3DH_SetTriggerInt(LIS3DH_TrigInt_t tr);
status_t LIS3DH_GetFifoSourceReg(u8_t* val);
status_t LIS3DH_GetFifoSourceBit(u8_t statusBIT, u8_t* val);
status_t LIS3DH_GetFifoSourceFSS(u8_t* val);

//Other Reading Functions
status_t LIS3DH_GetStatusReg(u8_t* val);
status_t LIS3DH_GetStatusBit(u8_t statusBIT, u8_t* val);
status_t LIS3DH_GetStatusAUXBit(u8_t statusBIT, u8_t* val);
status_t LIS3DH_GetStatusAUX(u8_t* val);
status_t LIS3DH_GetAccAxesRaw(AxesRaw_t* buff);
status_t LIS3DH_GetAuxRaw(LIS3DH_Aux123Raw_t* buff);
status_t LIS3DH_GetClickResponse(u8_t* val);
status_t LIS3DH_GetTempRaw(i8_t* val);
status_t LIS3DH_GetWHO_AM_I(u8_t* val);
status_t LIS3DH_Get6DPosition(u8_t* val);

status_t LIS3DH_SetClickLIMIT(u8_t t_limit);
status_t LIS3DH_SetClickLATENCY(u8_t t_latency);
status_t LIS3DH_SetClickWINDOW(u8_t t_window);
status_t LIS3DH_SetClickTHS(u8_t ths);
status_t LIS3DH_RESET_MEM(void);

bool LIS3DH_test(void);
void printAccValue(i16_t accX,i16_t accY,i16_t accZ,int compose);

void printAccRaw(i16_t accX,i16_t accY,i16_t accZ);
bool LIS3DH_ReadReg(u8_t Reg, u8_t* Data);
u8_t LIS3DH_WriteReg(u8_t WriteAddr, u8_t Data);
//Generic
// i.e. u8_t LIS3DH_ReadReg(u8_t Reg, u8_t* Data);
// i.e. u8_t LIS3DH_WriteReg(u8_t Reg, u8_t Data);


#endif /* __LIS3DH_H */

/******************* (C) COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/



