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
#include "bd_spi_master.h"
#include <nrf_assert.h>
#include "hal_acc.h"
#include "simple_uart.h"
#define FIFO_STREAM_MODE

#ifdef DEBUG_LOG1
void printAccRaw(i16_t accX,i16_t accY,i16_t accZ)
{
    // char data_array[20];

    // simple_uart_put(0x0d);
    // simple_uart_put(0x0a);

    // sprintf(data_array,"%d",accX);
    // simple_uart_putstring((const uint8_t *)data_array);
    // simple_uart_put(';');
    // sprintf(data_array,"%d",accY);
    // simple_uart_putstring((const uint8_t *)data_array);
    // simple_uart_put(';');
    // sprintf(data_array,"%d",accZ);
    // simple_uart_putstring((const uint8_t *)data_array);
    //  simple_uart_put(';');
    LOG(LEVEL_INFO,"%d;%d;%d;\n",accX,accY,accZ);
}

void printAccValue(i16_t accX,i16_t accY,i16_t accZ,int compose)
{
    // char data_array[20];

    // simple_uart_put(0x0d);
    // simple_uart_put(0x0a);
    /*
     sprintf(data_array,";%d",accX);
     simple_uart_putstring((const uint8_t *)data_array);
     sprintf(data_array,";%d",accY);
     simple_uart_putstring((const uint8_t *)data_array);
    */
    // sprintf(data_array,";%d",accZ);
    // simple_uart_putstring((const uint8_t *)data_array);
    // sprintf(data_array,";%d",compose);
    // simple_uart_putstring((const uint8_t *)data_array);
    LOG(LEVEL_INFO,"%d;%d;\n",accZ,compose);
}
#endif
static bool LIS3DH_ConfigClick(LIS3DH_ODR_t Freq, LIS3DH_Fullscale_t FullScale)
{
    // config click
    // uint8_t regValue;
    switch (Freq) {/*
                          case LIS3DH_ODR_25Hz:
                           //simple_uart_putstring("25Hz \t");
                           LIS3DH_SetClickCFG( LIS3DH_ZD_ENABLE | LIS3DH_ZS_DISABLE | LIS3DH_YD_ENABLE | 
                                                       LIS3DH_YS_DISABLE | LIS3DH_XD_ENABLE | LIS3DH_XS_DISABLE);
                         
                           LIS3DH_SetClickLIMIT(0x03);////(6);//127ms 127*0.05
                             LIS3DH_SetClickLATENCY(0x03);////637ms  637*0.05  (0x33);//
                           LIS3DH_SetClickWINDOW(0x05); // 637ms 637*0.05 (0xff);//
                           break;*/
        case LIS3DH_ODR_50Hz:
            //simple_uart_putstring("50Hz \t");
            LIS3DH_SetClickCFG( LIS3DH_ZD_ENABLE | LIS3DH_ZS_DISABLE | LIS3DH_YD_DISABLE |
                                LIS3DH_YS_DISABLE | LIS3DH_XD_DISABLE | LIS3DH_XS_DISABLE);
            LIS3DH_SetClickLIMIT(0x02);////(6);//127ms 127*0.05
            LIS3DH_SetClickLATENCY(0x04);////637ms  637*0.05  (0x33);//
            LIS3DH_SetClickWINDOW(0x05); // 637ms 637*0.05 (0xff);//
            /*
             LIS3DH_SetClickLIMIT(0x07);////(6);//127ms 127*0.05
               LIS3DH_SetClickLATENCY(0x06);////637ms  637*0.05  (0x33);//
             LIS3DH_SetClickWINDOW(0x06); // 637ms 637*0.05 (0xff);//
            */
            break;
        case LIS3DH_ODR_100Hz:
            //simple_uart_putstring("100Hz \t");
            LIS3DH_SetClickCFG( LIS3DH_ZD_ENABLE | LIS3DH_ZS_DISABLE | LIS3DH_YD_DISABLE |
                                LIS3DH_YS_DISABLE | LIS3DH_XD_DISABLE | LIS3DH_XS_DISABLE);

            LIS3DH_SetClickLIMIT(TAP_TIME_LIMIT);//(0x8);//(6);//127ms 127*0.05
            LIS3DH_SetClickLATENCY(TAP_LATENCY);//637ms  637*0.05  (0x33);//
            LIS3DH_SetClickWINDOW(TAP_WINDOW); // 637ms 637*0.05 (0xff);// 29ms
            break;
            /*
            case LIS3DH_ODR_200Hz:
             simple_uart_putstring("200Hz \t");
             LIS3DH_SetClickCFG( LIS3DH_ZD_ENABLE | LIS3DH_ZS_DISABLE | LIS3DH_YD_DISABLE | 
                    LIS3DH_YS_DISABLE | LIS3DH_XD_DISABLE | LIS3DH_XS_DISABLE);
             
             LIS3DH_SetClickLIMIT(0x10);//(6);//127ms 127*0.05
             LIS3DH_SetClickLATENCY(0x20);//637ms  637*0.05 (0x33);//
             LIS3DH_SetClickWINDOW(0x40); // 637ms 637*0.05 (0xff);//
             break;
            case LIS3DH_ODR_400Hz:
             simple_uart_putstring("400Hz \t");
             LIS3DH_SetClickCFG( LIS3DH_ZD_ENABLE | LIS3DH_ZS_ENABLE | LIS3DH_YD_DISABLE | 
                    LIS3DH_YS_DISABLE | LIS3DH_XD_DISABLE | LIS3DH_XS_DISABLE);
             
             LIS3DH_SetClickLIMIT(0x33);//(6);//127ms 127*0.05
             LIS3DH_SetClickLATENCY(0x40);//637ms  637*0.05 (0x33);//
             LIS3DH_SetClickWINDOW(0x90); // 637ms 637*0.05 (0xff);//
             break;*/
        default:
            break;
    }
    switch (FullScale) {
        case LIS3DH_FULLSCALE_4:
            LIS3DH_SetClickTHS(TAP_THRESHOLD);//255mg (0xff);// 2g 40  64 = 2g  80:2.5g 88:2.75g 96:3g  //1.5G
            break;

        default:
            break;
    }
#ifdef DEBUG_LOG1
    //  char str[32];
    uint8_t regValue;
    LIS3DH_ReadReg(LIS3DH_CLICK_CFG,&regValue);
    // sprintf(str,"CLICK_CFG: 0x%x\t",regValue);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"CLICK_CFG: 0x%x\n",regValue);

    LIS3DH_ReadReg(LIS3DH_TIME_LIMIT,&regValue);
    // sprintf(str,"TIME_LIMIT: 0x%x\t",regValue);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"TIME_LIMIT: 0x%x\n",regValue);

    LIS3DH_ReadReg(LIS3DH_TIME_LATENCY,&regValue);
    // sprintf(str,"LATENCY: 0x%x \t",regValue);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"LATENCY: 0x%x\n",regValue);

    LIS3DH_ReadReg(LIS3DH_TIME_WINDOW,&regValue);
    // sprintf(str,"WINDOW: 0x%x\t",regValue);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"WINDOW: 0x%x\n",regValue);

    LIS3DH_ReadReg(LIS3DH_CLICK_THS,&regValue);
    // sprintf(str,"CLICK_THS: %d\n\r",regValue);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"CLICK_THS: 0x%x\n",regValue);
#endif

    return true;
}
static uint8_t LIS3DH_Init(void)
{
    // bool init_succed;
    uint8_t whoami;

    /* Initialise SPI */
    /*!< Sample data at rising edge of clock and shift serial data at falling edge */
    /*!< MSb first */
    uint32_t *spi_base_address = spi_master_init(SPI0,SPI_MODE0,false);
    if (spi_base_address == 0) {
        return false;
    }
    spi_master_enable(SPI0);

    /* get device WHO_AM_I first to see if it exists! */
    ASSERT(LIS3DH_ReadReg(LIS3DH_WHO_AM_I,&whoami));
    ASSERT(whoami == 0x33);

    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG1, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG2, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG3, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG4, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG5, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG6, 0));
    ASSERT(LIS3DH_FIFOModeEnable(LIS3DH_FIFO_DISABLE));


    ASSERT(LIS3DH_SetAxis(LIS3DH_X_ENABLE|LIS3DH_Y_ENABLE|LIS3DH_Z_ENABLE));//
    ASSERT(LIS3DH_SetMode(LIS3DH_NORMAL));

    ASSERT(LIS3DH_SetODR(LIS3DH_ODR_FREQ));
    LIS3DH_SetFullScale(LIS3DH_FULLSCALE_4);
    ASSERT(LIS3DH_SetBDU(MEMS_ENABLE));

#ifdef FIFO_STREAM_MODE
    LIS3DH_FIFOModeEnable(LIS3DH_FIFO_STREAM_MODE);
#else

    LIS3DH_FIFOModeEnable(LIS3DH_FIFO_MODE);
#endif

    LIS3DH_SetClickCFG(0);
    LIS3DH_SetIntConfiguration(0);
    spi_master_disable(SPI0);
    return true;
}

/*******************************************************************************
* Function Name  : LIS3DH_GetFifoData
* Description    : Read ALL samples stored in FIFO
* Input          : Byte to empty by FIFO unread sample value
* Output         : None
* Return         : Status [value of FSS]
*******************************************************************************/
#define ACC_DATA_LEN 32
AxesRaw_t accData[ACC_DATA_LEN];

static void LIS3DH_GetFifoData(uint8_t * val)
{
    uint8_t index = 0;
    uint8_t fss = 0;
    uint8_t ovrun;
    LIS3DH_GetFifoSourceBit(LIS3DH_FIFO_SRC_OVRUN, &ovrun);
    /* Read transaction */
    LIS3DH_GetFifoSourceFSS(&fss);

    if(MEMS_SET == ovrun){
        fss = ACC_DATA_LEN;
    }
    fss = (fss >> 2) << 2;
#ifdef DEBUG_ACC
    LOG(LEVEL_INFO, "FSS len %d, ovrun:%d", fss, ovrun);
#endif
    for(index = 0; index < fss; index ++)
    {
        //ASSERT(LIS3DH_GetFifoSourceFSS(val));
        LIS3DH_GetAccAxesRaw(accData+index);        
    } 
#ifdef DEBUG_ACC
    //LOG(LEVEL_INFO, "index: %d  || fifo len %d \r\n", index, fss);
#endif
    
    *val = fss;
#ifdef FIFO_STREAM_MODE
    //ASSERT(LIS3DH_FIFOModeEnable(LIS3DH_FIFO_STREAM_MODE));
#else

    LIS3DH_FIFOModeEnable(LIS3DH_FIFO_BYPASS_MODE);
    LIS3DH_FIFOModeEnable(LIS3DH_FIFO_MODE);
#endif

}
void hal_acc_init(void)
{
    LIS3DH_Init();
}
void hal_acc_reset(void)
{
    spi_master_enable(SPI0);

    ASSERT(LIS3DH_RESET_MEM());
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG1, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG2, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG3, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG4, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG5, 0));
    ASSERT(LIS3DH_WriteReg(LIS3DH_CTRL_REG6, 0));
    ASSERT(LIS3DH_FIFOModeEnable(LIS3DH_FIFO_DISABLE));

    spi_master_disable(SPI0);
}
void hal_acc_config_Dtap(bool enable)
{
    spi_master_enable(SPI0);

    LIS3DH_Int1LatchEnable(MEMS_ENABLE);

    if(enable) {
        LIS3DH_SetInt2Pin(LIS3DH_CLICK_ON_PIN_INT2_ENABLE | LIS3DH_I2_INT1_ON_PIN_INT2_DISABLE |
                          LIS3DH_I2_INT2_ON_PIN_INT2_DISABLE | LIS3DH_I2_BOOT_ON_INT2_DISABLE |
                          LIS3DH_INT_ACTIVE_HIGH);
    } else {
        LIS3DH_SetInt2Pin(LIS3DH_CLICK_ON_PIN_INT2_DISABLE | LIS3DH_I2_INT1_ON_PIN_INT2_DISABLE |
                          LIS3DH_I2_INT2_ON_PIN_INT2_DISABLE | LIS3DH_I2_BOOT_ON_INT2_DISABLE |
                          LIS3DH_INT_ACTIVE_HIGH);
    }
    spi_master_disable(SPI0);

    hal_acc_ConfigClick(LIS3DH_ODR_FREQ,LIS3DH_FULLSCALE_4);
}
void hal_acc_config_wake_int(bool enable)
{
    spi_master_enable(SPI0);
#ifdef DEBUG_LOG1

    char index,value;
    for(index = 0x21; index <= 0x3D; index ++) {
        LIS3DH_ReadReg(index, &value);
        LOG(LEVEL_INFO,"--add:0x%x==v:0x%x \n",index,value);
        // sprintf(str,"--add:0x%x==v:0x%x \r\n",index,value);
        // simple_uart_putstring(str);
    }
#endif
    /*
     LIS3DH_WriteReg(LIS3DH_CTRL_REG1,0xA7);
     LIS3DH_WriteReg(LIS3DH_CTRL_REG2,0);
     LIS3DH_WriteReg(LIS3DH_CTRL_REG3,0x40);
     LIS3DH_WriteReg(LIS3DH_CTRL_REG4,00);
     LIS3DH_WriteReg(LIS3DH_CTRL_REG5,0x8);
     LIS3DH_WriteReg(LIS3DH_INT1_THS,0x10);
     LIS3DH_WriteReg(LIS3DH_INT1_DURATION,0);
     LIS3DH_WriteReg(LIS3DH_INT1_CFG,0x3F);
     ASSERT(LIS3DH_SetMode(LIS3DH_NORMAL));
    */

    if(enable) {
        LIS3DH_SetInt1Threshold(33); // 1100 mg        1100/() (35:1.085g)  (34:1.054g) (33:1.023)
        LIS3DH_SetInt1Duration(0);
        LIS3DH_SetIntMode(LIS3DH_INT_MODE_OR);
        LIS3DH_SetInt6D4DConfiguration(LIS3DH_INT1_6D_4D_DISABLE);
        LIS3DH_Int1LatchEnable(MEMS_ENABLE);
        LIS3DH_SetInt1Pin(LIS3DH_CLICK_ON_PIN_INT1_DISABLE | LIS3DH_I1_INT1_ON_PIN_INT1_ENABLE |
                          LIS3DH_I1_INT2_ON_PIN_INT1_DISABLE | LIS3DH_I1_DRDY1_ON_INT1_DISABLE | LIS3DH_I1_DRDY2_ON_INT1_DISABLE |
                          LIS3DH_WTM_ON_INT1_DISABLE | LIS3DH_INT1_OVERRUN_DISABLE   );
        LIS3DH_SetIntConfiguration(LIS3DH_INT1_OR |
                                   LIS3DH_INT1_ZHIE_ENABLE|
                                   LIS3DH_INT1_ZLIE_DISABLE|
                                   LIS3DH_INT1_YHIE_ENABLE|
                                   LIS3DH_INT1_YLIE_DISABLE|
                                   LIS3DH_INT1_XHIE_ENABLE|
                                   LIS3DH_INT1_XLIE_DISABLE);// z  Low and High int
    } else {
        LIS3DH_SetInt1Pin(LIS3DH_CLICK_ON_PIN_INT1_DISABLE | LIS3DH_I1_INT1_ON_PIN_INT1_DISABLE |
                          LIS3DH_I1_INT2_ON_PIN_INT1_DISABLE | LIS3DH_I1_DRDY1_ON_INT1_DISABLE | LIS3DH_I1_DRDY2_ON_INT1_DISABLE |
                          LIS3DH_WTM_ON_INT1_DISABLE | LIS3DH_INT1_OVERRUN_DISABLE);
        LIS3DH_SetIntConfiguration(LIS3DH_INT1_AND |
                                   LIS3DH_INT1_ZHIE_DISABLE|
                                   LIS3DH_INT1_ZLIE_DISABLE|
                                   LIS3DH_INT1_YHIE_DISABLE|
                                   LIS3DH_INT1_YLIE_DISABLE|
                                   LIS3DH_INT1_XHIE_DISABLE|
                                   LIS3DH_INT1_XLIE_DISABLE);// z  Low and High int
    }


#ifdef DEBUG_LOG1
    for(index = 0x20; index <= 0x3D; index ++) {
        LIS3DH_ReadReg(index, &value);
        // sprintf(str,"config add:0x%x==v:0x%x \r\n",index,value);
        // simple_uart_putstring(str);
        LOG(LEVEL_INFO,"config add:0x%x==v:0x%x \n",index,value);
    }

    index = 0x31;
    LIS3DH_ReadReg(index, &value);
    //    sprintf(str,"config add:0x%x==v:0x%x \r\n",index,value);
    //    simple_uart_putstring(str);
    LOG(LEVEL_INFO,"config add:0x%x==v:0x%x \n",index,value);
    index = 0x25;
    LIS3DH_ReadReg(index, &value);
    //  sprintf(str,"config add:0x%x==v:0x%x \r\n",index,value);
    //  simple_uart_putstring(str);
    LOG(LEVEL_INFO,"config add:0x%x==v:0x%x \n",index,value);
#endif

    spi_master_disable(SPI0);
}

void hal_acc_PowerDown(void)
{
    spi_master_enable(SPI0);
    LIS3DH_SetMode(LIS3DH_POWER_DOWN);
    spi_master_disable(SPI0);
}

void hal_acc_enable(void)
{
    spi_master_enable(SPI0);
    LIS3DH_SetMode(LIS3DH_NORMAL);
    spi_master_disable(SPI0);
}

bool hal_acc_ConfigClick(LIS3DH_ODR_t Freq, LIS3DH_Fullscale_t FullScale)
{
    spi_master_enable(SPI0);
    bool ret = LIS3DH_ConfigClick(Freq, FullScale);
    //ret &= LIS3DH_HPFClickEnable(MEMS_ENABLE);
    spi_master_disable(SPI0);
    return ret;
}
void hal_acc_GetFifoData(uint8_t * val)
{
    spi_master_enable(SPI0);
    LIS3DH_GetFifoData(val);
    spi_master_disable(SPI0);
}


