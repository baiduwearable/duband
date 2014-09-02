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
#include "board_config_pins.h"

#ifdef FEATURE_MOTOR_LRA

#include "twi_master.h"
#include "nrf_delay.h"
#include "drv2605.h"
#include "simple_uart.h"
#include "nrf_gpio.h"
static uint8_t drv2605_address;

static unsigned char drv260x_init_val[] =
    {
        RATED_VOLTAGE_REG, 0x56,
        OVERDRIVE_CLAMP_VOLTAGE_REG, 0x90,
        FEEDBACK_CONTROL_REG, 0xF6,
        Control1_REG, 0x93,
        Control3_REG, 0x80,
        //AUDIO_HAPTICS_MIN_INPUT_REG, 0,
        //AUDIO_HAPTICS_MAX_INPUT_REG, 0,
        //AUDIO_HAPTICS_MIN_OUTPUT_REG, 0,
        //AUDIO_HAPTICS_MAX_OUTPUT_REG, 0,
    };

/*******************************************************************************
* Function Name  : LIS3DH_WriteReg
* Description  : Generic Writing function. It must be fullfilled with either
*   : I2C or SPI writing function
* Input   : Register Address, Data to be written
* Output  : None
* Return  : None
*******************************************************************
u8_t DRV2605_WriteReg(u8_t WriteAddr, u8_t Data) {
  
 uint8_t w2_data[2];
 
 w2_data[0] = WriteAddr;
 w2_data[1] = Data;
  return twi_master_transfer(drv2605_address, w2_data, 2, TWI_ISSUE_STOP);  
}************/
/*******************************************************************************
* Function Name  : LIS3DH_ReadReg
* Description  : Generic Reading function. It must be fullfilled with either
*   : I2C or SPI reading functions     
* Input   : Register Address
* Output  : Data REad
* Return  : None
******************************************************************
bool DRV2605_ReadReg(u8_t Reg, u8_t* Data) {
  //uint8_t register_address, uint8_t *destination, uint8_t number_of_bytes
  bool transfer_succeeded;
  transfer_succeeded = twi_master_transfer(drv2605_address, &Reg, 1, TWI_DONT_ISSUE_STOP);
  transfer_succeeded &= twi_master_transfer(drv2605_address|TWI_READ_BIT, Data, 1, TWI_ISSUE_STOP);
  return transfer_succeeded;
}
*************/
/*
 parameter "data" format: addr, value, addr, value, ...
*/
static int drv260x_write_reg_val(unsigned char* data, int size)
{
    int i = 0;
    int err = 0;

    if (size % 2 != 0)
        return -EINVAL;

    while (i < size) {
        // TODO, i2c write
        twi_master_transfer(drv2605_address,data + i, 2, TWI_ISSUE_STOP);
        i+=2;
    }
    return err;
}

static void drv260x_set_go_bit(char val)
{
    unsigned char go[] =
        {
            GO_REG, val
        };
    drv260x_write_reg_val(go, 2);
}

static unsigned char drv260x_read_reg(unsigned char reg)
{
    // TODO, i2c read

    bool transfer_succeeded;
    static unsigned char Data;
    unsigned char mReg = reg;
    transfer_succeeded = twi_master_transfer(drv2605_address, &mReg, 1, TWI_DONT_ISSUE_STOP);
    transfer_succeeded &= twi_master_transfer(drv2605_address|TWI_READ_BIT, &Data, 1, TWI_ISSUE_STOP);
    return Data;
}

static unsigned char drv260x_setbit_reg(unsigned char reg, unsigned char mask, unsigned char value)
{
    unsigned char temp = 0;
    unsigned char buff[2];
    unsigned char regval = drv260x_read_reg(reg);

    temp = regval & ~mask;
    temp |= value & mask;

    if(temp != regval) {
        buff[0] = reg;
        buff[1] = temp;

        return drv260x_write_reg_val(buff, 2);
    } else
        return 2;
}

static void drv2605_poll_go_bit(void)
{
    while (drv260x_read_reg(GO_REG) == GO) {
        nrf_delay_ms(GO_BIT_POLL_INTERVAL);
    }
}

static void drv2605_select_library(char lib)
{
    unsigned char library[] =
        {
            LIBRARY_SELECTION_REG, lib
        };
    drv260x_write_reg_val(library, 2);
}

static void drv260x_set_rtp_val(char value)
{
    unsigned char rtp_val[] =
        {
            REAL_TIME_PLAYBACK_REG, value
        };
    drv260x_write_reg_val(rtp_val, 2);
}

static void drv2605_set_waveform_sequence(unsigned char* seq, char size)
{
    unsigned char data[WAVEFORM_SEQUENCER_MAX + 1];
    int i = 1;

    if (size > WAVEFORM_SEQUENCER_MAX) {
        size = WAVEFORM_SEQUENCER_MAX;
    }
    for(; i<(size+1); i++) {
        data[i] = seq[i-1];
    }
    for(; i<=WAVEFORM_SEQUENCER_MAX; i++) {
        data[i] = 0;
    }
    data[0] = WAVEFORM_SEQUENCER_REG;

    // //TODO: i2c write
    //i2c_write(addr_260x,  data, WAVEFORM_SEQUENCER_MAX+1);
    twi_master_transfer(drv2605_address,data, WAVEFORM_SEQUENCER_MAX+1, TWI_ISSUE_STOP);

}

static void drv260x_change_mode(char mode)
{
    unsigned char tmp[2] = {MODE_REG, mode};

    if((mode == MODE_PATTERN_RTP_ON)||(mode == MODE_SEQ_RTP_ON))
        tmp[1] = MODE_REAL_TIME_PLAYBACK;
    else if((mode == MODE_PATTERN_RTP_OFF)||(mode == MODE_SEQ_RTP_OFF))
        tmp[1] = MODE_INTERNAL_TRIGGER;

    drv260x_write_reg_val(tmp, 2);
}

static unsigned char autocal_sequence[] = {
            MODE_REG,                       AUTO_CALIBRATION,
            REAL_TIME_PLAYBACK_REG,         REAL_TIME_PLAYBACK_STRENGTH,
            GO_REG,                         GO,
        };


/* initialize */
void drv260x_init(void)
{
    uint8_t OTP;
    char status;
    nrf_gpio_pin_set(LRA_EN_PIN);
    drv2605_address = (uint8_t)(DRV2605_ADDR << 1);
    if (!twi_master_init()) {
        LOG(LEVEL_INFO,"drv260xInit fail \n");
        //  simple_uart_putstring("drv260xInit fail \r\n");
    }

    drv260x_change_mode(MODE_INTERNAL_TRIGGER);
    nrf_delay_ms(STANDBY_WAKE_DELAY);

    OTP = drv260x_read_reg(AUTOCAL_MEM_INTERFACE_REG) & AUTOCAL_MEM_INTERFACE_REG_OTP_MASK;

    drv260x_write_reg_val(drv260x_init_val, sizeof(drv260x_init_val));

#ifdef DEBUG_LOG1

    // char str[32];
    uint8_t regData;
    regData = drv260x_read_reg(RATED_VOLTAGE_REG);
    // sprintf(str,"0x56: 0x%x \t",regData);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"0x56: 0x%x \n",regData);

    regData = drv260x_read_reg(OVERDRIVE_CLAMP_VOLTAGE_REG);
    // sprintf(str,"0x90: 0x%x \t",regData);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"0x90: 0x%x \n",regData);

    regData = drv260x_read_reg(FEEDBACK_CONTROL_REG);
    // sprintf(str,"0xf6: 0x%x \t",regData);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"0xf6: 0x%x \n",regData);

    regData = drv260x_read_reg(Control1_REG);
    // sprintf(str,"0x93: 0x%x \t",regData);
    // simple_uart_putstring(str);
    LOG(LEVEL_INFO,"0x93: 0x%x \n",regData);
#endif

    if(OTP == 0) {
        // calib
        drv260x_write_reg_val(autocal_sequence, sizeof(autocal_sequence));
        drv2605_poll_go_bit();
        status = drv260x_read_reg(STATUS_REG);
        if ((status & DIAG_RESULT_MASK) == AUTO_CAL_FAILED) {
            // printk("drv260x auto-cal retry failed.\n");
            // return -ENODEV;
        }
    }

    drv2605_select_library(LIBRARY_F);

    // Put hardware in standby
    drv260x_change_mode(MODE_STANDBY);

    TWI_SDA_STANDARD0_DISCONNECT();
    TWI_SCL_STANDARD0_DISCONNECT();
    //NRF_TWI1->PSELSCL = 0xFFFFFFFF;
    //NRF_TWI1->PSELSDA = 0xFFFFFFFF;
    NRF_TWI1->ENABLE = TWI_ENABLE_ENABLE_Disabled<< TWI_ENABLE_ENABLE_Pos;
    NRF_GPIO->PIN_CNF[TWI_MASTER_CONFIG_DATA_PIN_NUMBER] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
    NRF_GPIO->PIN_CNF[TWI_MASTER_CONFIG_CLOCK_PIN_NUMBER] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

    nrf_gpio_pin_clear(LRA_EN_PIN);
}

static unsigned char vib0[] = {0x05, 0x00};
static unsigned char vib1[] = {0x24, 0x00};
static const struct LIB_Seq vibs[] =
    {
        {
            vib0, sizeof(vib0)
        },
        {vib1, sizeof(vib1)},
    };

/* start a viberate seq, wait: wait vib done */
void drv260x_vib(int idx, int wait)
{

    nrf_gpio_pin_set(LRA_EN_PIN);
    if (!twi_master_init()) {
        //simple_uart_putstring("drv260xInit fail \r\n");
    }

    drv260x_change_mode(MODE_INTERNAL_TRIGGER);
    drv2605_set_waveform_sequence(vibs[idx].seq, vibs[idx].cnt);
    drv260x_set_go_bit(GO);
    if(wait) {
        drv2605_poll_go_bit();
        drv260x_change_mode(MODE_STANDBY);
    }
    TWI_SDA_STANDARD0_DISCONNECT();
    TWI_SCL_STANDARD0_DISCONNECT();

    NRF_TWI1->ENABLE = TWI_ENABLE_ENABLE_Disabled<< TWI_ENABLE_ENABLE_Pos;
    NRF_GPIO->PIN_CNF[TWI_MASTER_CONFIG_DATA_PIN_NUMBER] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
    NRF_GPIO->PIN_CNF[TWI_MASTER_CONFIG_CLOCK_PIN_NUMBER] =  (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
    nrf_gpio_pin_clear(LRA_EN_PIN);
}

/* stop vib */
void drv260x_stop(void)
{
    drv260x_set_go_bit(STOP);
}

/* is viberating? */
int drv260x_running(void)
{
    return drv260x_read_reg(GO_REG);
}

/* enter low power mode */
int drv260x_suspend(void)
{
    drv260x_change_mode(MODE_STANDBY);
    return 1;
}

#endif //FEATURE_MOTOR_LRA
