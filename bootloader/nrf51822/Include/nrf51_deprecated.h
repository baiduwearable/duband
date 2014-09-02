/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
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


#ifndef NRF51_DEPRECATED_H
#define NRF51_DEPRECATED_H

/*lint ++flb "Enter library region */

/* This file is given to prevent your SW from not compiling with the updates made to nrf51.h and 
 * nrf51_bitfields.h. The macros defined in this file were available previously. Do not use these
 * macros on purpose. Use the ones defined in nrf51.h and nrf51_bitfields.h instead.
 */


/* FICR */
#define SIZERAMBLOCK0     SIZERAMBLOCK[0]                 /*!< Size of RAM block 0 in bytes.                                         */
#define SIZERAMBLOCK1     SIZERAMBLOCK[1]                 /*!< Size of RAM block 1 in bytes.                                         */
#define SIZERAMBLOCK2     SIZERAMBLOCK[2]                 /*!< Size of RAM block 2 in bytes.                                         */
#define SIZERAMBLOCK3     SIZERAMBLOCK[3]                 /*!< Size of RAM block 3 in bytes.                                         */
#define DEVICEID0         DEVICEID[0]                     /*!< Device identifier, bits 31 to 0.                                      */
#define DEVICEID1         DEVICEID[1]                     /*!< Device identifier, bits 63 to 32.                                     */
#define ER0               ER[0]                           /*!< Encryption root, bits 31..0.                                          */
#define ER1               ER[1]                           /*!< Encryption root, bits 63..32.                                         */
#define ER2               ER[2]                           /*!< Encryption root, bits 95..64.                                         */
#define ER3               ER[3]                           /*!< Encryption root, bits 127..96.                                        */
#define IR0               IR[0]                           /*!< Identity root, bits 31..0.                                            */
#define IR1               IR[1]                           /*!< Identity root, bits 63..32.                                           */
#define IR2               IR[2]                           /*!< Identity root, bits 95..64.                                           */
#define IR3               IR[3]                           /*!< Identity root, bits 127..96.                                          */
#define DEVICEADDR0       DEVICEADDR[0]                   /*!< Device address, bits 31..0.                                           */
#define DEVICEADDR1       DEVICEADDR[1]                   /*!< Device address, bits 63..32.                                          */


/* PPI */
#define TASKS_CHG0EN     TASKS_CHG[0].EN                  /*!< Enable channel group 0.                                               */
#define TASKS_CHG0DIS    TASKS_CHG[0].DIS                 /*!< Disable channel group 0.                                              */
#define TASKS_CHG1EN     TASKS_CHG[1].EN                  /*!< Enable channel group 1.                                               */
#define TASKS_CHG1DIS    TASKS_CHG[1].DIS                 /*!< Disable channel group 1.                                              */
#define TASKS_CHG2EN     TASKS_CHG[2].EN                  /*!< Enable channel group 2.                                               */
#define TASKS_CHG2DIS    TASKS_CHG[2].DIS                 /*!< Disable channel group 2.                                              */
#define TASKS_CHG3EN     TASKS_CHG[3].EN                  /*!< Enable channel group 3.                                               */
#define TASKS_CHG3DIS    TASKS_CHG[3].DIS                 /*!< Disable channel group 3.                                              */
#define CH0_EEP          CH[0].EEP                        /*!< Channel 0 event end-point.                                            */
#define CH0_TEP          CH[0].TEP                        /*!< Channel 0 task end-point.                                             */
#define CH1_EEP          CH[1].EEP                        /*!< Channel 1 event end-point.                                            */
#define CH1_TEP          CH[1].TEP                        /*!< Channel 1 task end-point.                                             */
#define CH2_EEP          CH[2].EEP                        /*!< Channel 2 event end-point.                                            */
#define CH2_TEP          CH[2].TEP                        /*!< Channel 2 task end-point.                                             */
#define CH3_EEP          CH[3].EEP                        /*!< Channel 3 event end-point.                                            */
#define CH3_TEP          CH[3].TEP                        /*!< Channel 3 task end-point.                                             */
#define CH4_EEP          CH[4].EEP                        /*!< Channel 4 event end-point.                                            */
#define CH4_TEP          CH[4].TEP                        /*!< Channel 4 task end-point.                                             */
#define CH5_EEP          CH[5].EEP                        /*!< Channel 5 event end-point.                                            */
#define CH5_TEP          CH[5].TEP                        /*!< Channel 5 task end-point.                                             */
#define CH6_EEP          CH[6].EEP                        /*!< Channel 6 event end-point.                                            */
#define CH6_TEP          CH[6].TEP                        /*!< Channel 6 task end-point.                                             */
#define CH7_EEP          CH[7].EEP                        /*!< Channel 7 event end-point.                                            */
#define CH7_TEP          CH[7].TEP                        /*!< Channel 7 task end-point.                                             */
#define CH8_EEP          CH[8].EEP                        /*!< Channel 8 event end-point.                                            */
#define CH8_TEP          CH[8].TEP                        /*!< Channel 8 task end-point.                                             */
#define CH9_EEP          CH[9].EEP                        /*!< Channel 9 event end-point.                                            */
#define CH9_TEP          CH[9].TEP                        /*!< Channel 9 task end-point.                                             */
#define CH10_EEP         CH[10].EEP                       /*!< Channel 10 event end-point.                                           */
#define CH10_TEP         CH[10].TEP                       /*!< Channel 10 task end-point.                                            */
#define CH11_EEP         CH[11].EEP                       /*!< Channel 11 event end-point.                                           */
#define CH11_TEP         CH[11].TEP                       /*!< Channel 11 task end-point.                                            */
#define CH12_EEP         CH[12].EEP                       /*!< Channel 12 event end-point.                                           */
#define CH12_TEP         CH[12].TEP                       /*!< Channel 12 task end-point.                                            */
#define CH13_EEP         CH[13].EEP                       /*!< Channel 13 event end-point.                                           */
#define CH13_TEP         CH[13].TEP                       /*!< Channel 13 task end-point.                                            */
#define CH14_EEP         CH[14].EEP                       /*!< Channel 14 event end-point.                                           */
#define CH14_TEP         CH[14].TEP                       /*!< Channel 14 task end-point.                                            */
#define CH15_EEP         CH[15].EEP                       /*!< Channel 15 event end-point.                                           */
#define CH15_TEP         CH[15].TEP                       /*!< Channel 15 task end-point.                                            */
#define CHG0             CHG[0]                           /*!< Channel group 0.                                                      */
#define CHG1             CHG[1]                           /*!< Channel group 1.                                                      */
#define CHG2             CHG[2]                           /*!< Channel group 2.                                                      */
#define CHG3             CHG[3]                           /*!< Channel group 3.                                                      */



#define PPI_CHG0_CH15_Pos       PPI_CHG_CH15_Pos            /*!< Position of CH15 field. */
#define PPI_CHG0_CH15_Msk       PPI_CHG_CH15_Msk            /*!< Bit mask of CH15 field. */
#define PPI_CHG0_CH15_Excluded  PPI_CHG_CH15_Excluded       /*!< Channel excluded. */
#define PPI_CHG0_CH15_Included  PPI_CHG_CH15_Included       /*!< Channel included. */
#define PPI_CHG0_CH14_Pos       PPI_CHG_CH14_Pos            /*!< Position of CH14 field. */
#define PPI_CHG0_CH14_Msk       PPI_CHG_CH14_Msk            /*!< Bit mask of CH14 field. */
#define PPI_CHG0_CH14_Excluded  PPI_CHG_CH14_Excluded       /*!< Channel excluded. */
#define PPI_CHG0_CH14_Included  PPI_CHG_CH14_Included       /*!< Channel included. */
#define PPI_CHG0_CH13_Pos       PPI_CHG_CH13_Pos            /*!< Position of CH13 field. */
#define PPI_CHG0_CH13_Msk       PPI_CHG_CH13_Msk            /*!< Bit mask of CH13 field. */
#define PPI_CHG0_CH13_Excluded  PPI_CHG_CH13_Excluded       /*!< Channel excluded. */
#define PPI_CHG0_CH13_Included  PPI_CHG_CH13_Included       /*!< Channel included. */
#define PPI_CHG0_CH12_Pos       PPI_CHG_CH12_Pos            /*!< Position of CH12 field. */
#define PPI_CHG0_CH12_Msk       PPI_CHG_CH12_Msk            /*!< Bit mask of CH12 field. */
#define PPI_CHG0_CH12_Excluded  PPI_CHG_CH12_Excluded       /*!< Channel excluded. */
#define PPI_CHG0_CH12_Included  PPI_CHG_CH12_Included       /*!< Channel included. */
#define PPI_CHG0_CH11_Pos       PPI_CHG_CH11_Pos            /*!< Position of CH11 field. */
#define PPI_CHG0_CH11_Msk       PPI_CHG_CH11_Msk            /*!< Bit mask of CH11 field. */
#define PPI_CHG0_CH11_Excluded  PPI_CHG_CH11_Excluded       /*!< Channel excluded. */
#define PPI_CHG0_CH11_Included  PPI_CHG_CH11_Included       /*!< Channel included. */
#define PPI_CHG0_CH10_Pos       PPI_CHG_CH10_Pos            /*!< Position of CH10 field. */
#define PPI_CHG0_CH10_Msk       PPI_CHG_CH10_Msk            /*!< Bit mask of CH10 field. */
#define PPI_CHG0_CH10_Excluded  PPI_CHG_CH10_Excluded       /*!< Channel excluded. */
#define PPI_CHG0_CH10_Included  PPI_CHG_CH10_Included       /*!< Channel included. */
#define PPI_CHG0_CH9_Pos        PPI_CHG_CH9_Pos             /*!< Position of CH9 field. */
#define PPI_CHG0_CH9_Msk        PPI_CHG_CH9_Msk             /*!< Bit mask of CH9 field. */
#define PPI_CHG0_CH9_Excluded   PPI_CHG_CH9_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH9_Included   PPI_CHG_CH9_Included        /*!< Channel included. */
#define PPI_CHG0_CH8_Pos        PPI_CHG_CH8_Pos             /*!< Position of CH8 field. */
#define PPI_CHG0_CH8_Msk        PPI_CHG_CH8_Msk             /*!< Bit mask of CH8 field. */
#define PPI_CHG0_CH8_Excluded   PPI_CHG_CH8_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH8_Included   PPI_CHG_CH8_Included        /*!< Channel included. */
#define PPI_CHG0_CH7_Pos        PPI_CHG_CH7_Pos             /*!< Position of CH7 field. */
#define PPI_CHG0_CH7_Msk        PPI_CHG_CH7_Msk             /*!< Bit mask of CH7 field. */
#define PPI_CHG0_CH7_Excluded   PPI_CHG_CH7_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH7_Included   PPI_CHG_CH7_Included        /*!< Channel included. */
#define PPI_CHG0_CH6_Pos        PPI_CHG_CH6_Pos             /*!< Position of CH6 field. */
#define PPI_CHG0_CH6_Msk        PPI_CHG_CH6_Msk             /*!< Bit mask of CH6 field. */
#define PPI_CHG0_CH6_Excluded   PPI_CHG_CH6_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH6_Included   PPI_CHG_CH6_Included        /*!< Channel included. */
#define PPI_CHG0_CH5_Pos        PPI_CHG_CH5_Pos             /*!< Position of CH5 field. */
#define PPI_CHG0_CH5_Msk        PPI_CHG_CH5_Msk             /*!< Bit mask of CH5 field. */
#define PPI_CHG0_CH5_Excluded   PPI_CHG_CH5_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH5_Included   PPI_CHG_CH5_Included        /*!< Channel included. */
#define PPI_CHG0_CH4_Pos        PPI_CHG_CH4_Pos             /*!< Position of CH4 field. */
#define PPI_CHG0_CH4_Msk        PPI_CHG_CH4_Msk             /*!< Bit mask of CH4 field. */
#define PPI_CHG0_CH4_Excluded   PPI_CHG_CH4_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH4_Included   PPI_CHG_CH4_Included        /*!< Channel included. */
#define PPI_CHG0_CH3_Pos        PPI_CHG_CH3_Pos             /*!< Position of CH3 field. */
#define PPI_CHG0_CH3_Msk        PPI_CHG_CH3_Msk             /*!< Bit mask of CH3 field. */
#define PPI_CHG0_CH3_Excluded   PPI_CHG_CH3_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH3_Included   PPI_CHG_CH3_Included        /*!< Channel included. */
#define PPI_CHG0_CH2_Pos        PPI_CHG_CH2_Pos             /*!< Position of CH2 field. */
#define PPI_CHG0_CH2_Msk        PPI_CHG_CH2_Msk             /*!< Bit mask of CH2 field. */
#define PPI_CHG0_CH2_Excluded   PPI_CHG_CH2_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH2_Included   PPI_CHG_CH2_Included        /*!< Channel included. */
#define PPI_CHG0_CH1_Pos        PPI_CHG_CH1_Pos             /*!< Position of CH1 field. */
#define PPI_CHG0_CH1_Msk        PPI_CHG_CH1_Msk             /*!< Bit mask of CH1 field. */
#define PPI_CHG0_CH1_Excluded   PPI_CHG_CH1_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH1_Included   PPI_CHG_CH1_Included        /*!< Channel included. */
#define PPI_CHG0_CH0_Pos        PPI_CHG_CH0_Pos             /*!< Position of CH0 field. */
#define PPI_CHG0_CH0_Msk        PPI_CHG_CH0_Msk             /*!< Bit mask of CH0 field. */
#define PPI_CHG0_CH0_Excluded   PPI_CHG_CH0_Excluded        /*!< Channel excluded. */
#define PPI_CHG0_CH0_Included   PPI_CHG_CH0_Included        /*!< Channel included. */ 

#define PPI_CHG1_CH15_Pos       PPI_CHG_CH15_Pos            /*!< Position of CH15 field. */
#define PPI_CHG1_CH15_Msk       PPI_CHG_CH15_Msk            /*!< Bit mask of CH15 field. */
#define PPI_CHG1_CH15_Excluded  PPI_CHG_CH15_Excluded       /*!< Channel excluded. */
#define PPI_CHG1_CH15_Included  PPI_CHG_CH15_Included       /*!< Channel included. */
#define PPI_CHG1_CH14_Pos       PPI_CHG_CH14_Pos            /*!< Position of CH14 field. */
#define PPI_CHG1_CH14_Msk       PPI_CHG_CH14_Msk            /*!< Bit mask of CH14 field. */
#define PPI_CHG1_CH14_Excluded  PPI_CHG_CH14_Excluded       /*!< Channel excluded. */
#define PPI_CHG1_CH14_Included  PPI_CHG_CH14_Included       /*!< Channel included. */
#define PPI_CHG1_CH13_Pos       PPI_CHG_CH13_Pos            /*!< Position of CH13 field. */
#define PPI_CHG1_CH13_Msk       PPI_CHG_CH13_Msk            /*!< Bit mask of CH13 field. */
#define PPI_CHG1_CH13_Excluded  PPI_CHG_CH13_Excluded       /*!< Channel excluded. */
#define PPI_CHG1_CH13_Included  PPI_CHG_CH13_Included       /*!< Channel included. */
#define PPI_CHG1_CH12_Pos       PPI_CHG_CH12_Pos            /*!< Position of CH12 field. */
#define PPI_CHG1_CH12_Msk       PPI_CHG_CH12_Msk            /*!< Bit mask of CH12 field. */
#define PPI_CHG1_CH12_Excluded  PPI_CHG_CH12_Excluded       /*!< Channel excluded. */
#define PPI_CHG1_CH12_Included  PPI_CHG_CH12_Included       /*!< Channel included. */
#define PPI_CHG1_CH11_Pos       PPI_CHG_CH11_Pos            /*!< Position of CH11 field. */
#define PPI_CHG1_CH11_Msk       PPI_CHG_CH11_Msk            /*!< Bit mask of CH11 field. */
#define PPI_CHG1_CH11_Excluded  PPI_CHG_CH11_Excluded       /*!< Channel excluded. */
#define PPI_CHG1_CH11_Included  PPI_CHG_CH11_Included       /*!< Channel included. */
#define PPI_CHG1_CH10_Pos       PPI_CHG_CH10_Pos            /*!< Position of CH10 field. */
#define PPI_CHG1_CH10_Msk       PPI_CHG_CH10_Msk            /*!< Bit mask of CH10 field. */
#define PPI_CHG1_CH10_Excluded  PPI_CHG_CH10_Excluded       /*!< Channel excluded. */
#define PPI_CHG1_CH10_Included  PPI_CHG_CH10_Included       /*!< Channel included. */
#define PPI_CHG1_CH9_Pos        PPI_CHG_CH9_Pos             /*!< Position of CH9 field. */
#define PPI_CHG1_CH9_Msk        PPI_CHG_CH9_Msk             /*!< Bit mask of CH9 field. */
#define PPI_CHG1_CH9_Excluded   PPI_CHG_CH9_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH9_Included   PPI_CHG_CH9_Included        /*!< Channel included. */
#define PPI_CHG1_CH8_Pos        PPI_CHG_CH8_Pos             /*!< Position of CH8 field. */
#define PPI_CHG1_CH8_Msk        PPI_CHG_CH8_Msk             /*!< Bit mask of CH8 field. */
#define PPI_CHG1_CH8_Excluded   PPI_CHG_CH8_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH8_Included   PPI_CHG_CH8_Included        /*!< Channel included. */
#define PPI_CHG1_CH7_Pos        PPI_CHG_CH7_Pos             /*!< Position of CH7 field. */
#define PPI_CHG1_CH7_Msk        PPI_CHG_CH7_Msk             /*!< Bit mask of CH7 field. */
#define PPI_CHG1_CH7_Excluded   PPI_CHG_CH7_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH7_Included   PPI_CHG_CH7_Included        /*!< Channel included. */
#define PPI_CHG1_CH6_Pos        PPI_CHG_CH6_Pos             /*!< Position of CH6 field. */
#define PPI_CHG1_CH6_Msk        PPI_CHG_CH6_Msk             /*!< Bit mask of CH6 field. */
#define PPI_CHG1_CH6_Excluded   PPI_CHG_CH6_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH6_Included   PPI_CHG_CH6_Included        /*!< Channel included. */
#define PPI_CHG1_CH5_Pos        PPI_CHG_CH5_Pos             /*!< Position of CH5 field. */
#define PPI_CHG1_CH5_Msk        PPI_CHG_CH5_Msk             /*!< Bit mask of CH5 field. */
#define PPI_CHG1_CH5_Excluded   PPI_CHG_CH5_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH5_Included   PPI_CHG_CH5_Included        /*!< Channel included. */
#define PPI_CHG1_CH4_Pos        PPI_CHG_CH4_Pos             /*!< Position of CH4 field. */
#define PPI_CHG1_CH4_Msk        PPI_CHG_CH4_Msk             /*!< Bit mask of CH4 field. */
#define PPI_CHG1_CH4_Excluded   PPI_CHG_CH4_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH4_Included   PPI_CHG_CH4_Included        /*!< Channel included. */
#define PPI_CHG1_CH3_Pos        PPI_CHG_CH3_Pos             /*!< Position of CH3 field. */
#define PPI_CHG1_CH3_Msk        PPI_CHG_CH3_Msk             /*!< Bit mask of CH3 field. */
#define PPI_CHG1_CH3_Excluded   PPI_CHG_CH3_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH3_Included   PPI_CHG_CH3_Included        /*!< Channel included. */
#define PPI_CHG1_CH2_Pos        PPI_CHG_CH2_Pos             /*!< Position of CH2 field. */
#define PPI_CHG1_CH2_Msk        PPI_CHG_CH2_Msk             /*!< Bit mask of CH2 field. */
#define PPI_CHG1_CH2_Excluded   PPI_CHG_CH2_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH2_Included   PPI_CHG_CH2_Included        /*!< Channel included. */
#define PPI_CHG1_CH1_Pos        PPI_CHG_CH1_Pos             /*!< Position of CH1 field. */
#define PPI_CHG1_CH1_Msk        PPI_CHG_CH1_Msk             /*!< Bit mask of CH1 field. */
#define PPI_CHG1_CH1_Excluded   PPI_CHG_CH1_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH1_Included   PPI_CHG_CH1_Included        /*!< Channel included. */
#define PPI_CHG1_CH0_Pos        PPI_CHG_CH0_Pos             /*!< Position of CH0 field. */
#define PPI_CHG1_CH0_Msk        PPI_CHG_CH0_Msk             /*!< Bit mask of CH0 field. */
#define PPI_CHG1_CH0_Excluded   PPI_CHG_CH0_Excluded        /*!< Channel excluded. */
#define PPI_CHG1_CH0_Included   PPI_CHG_CH0_Included        /*!< Channel included. */

#define PPI_CHG2_CH15_Pos       PPI_CHG_CH15_Pos            /*!< Position of CH15 field. */
#define PPI_CHG2_CH15_Msk       PPI_CHG_CH15_Msk            /*!< Bit mask of CH15 field. */
#define PPI_CHG2_CH15_Excluded  PPI_CHG_CH15_Excluded       /*!< Channel excluded. */
#define PPI_CHG2_CH15_Included  PPI_CHG_CH15_Included       /*!< Channel included. */
#define PPI_CHG2_CH14_Pos       PPI_CHG_CH14_Pos            /*!< Position of CH14 field. */
#define PPI_CHG2_CH14_Msk       PPI_CHG_CH14_Msk            /*!< Bit mask of CH14 field. */
#define PPI_CHG2_CH14_Excluded  PPI_CHG_CH14_Excluded       /*!< Channel excluded. */
#define PPI_CHG2_CH14_Included  PPI_CHG_CH14_Included       /*!< Channel included. */
#define PPI_CHG2_CH13_Pos       PPI_CHG_CH13_Pos            /*!< Position of CH13 field. */
#define PPI_CHG2_CH13_Msk       PPI_CHG_CH13_Msk            /*!< Bit mask of CH13 field. */
#define PPI_CHG2_CH13_Excluded  PPI_CHG_CH13_Excluded       /*!< Channel excluded. */
#define PPI_CHG2_CH13_Included  PPI_CHG_CH13_Included       /*!< Channel included. */
#define PPI_CHG2_CH12_Pos       PPI_CHG_CH12_Pos            /*!< Position of CH12 field. */
#define PPI_CHG2_CH12_Msk       PPI_CHG_CH12_Msk            /*!< Bit mask of CH12 field. */
#define PPI_CHG2_CH12_Excluded  PPI_CHG_CH12_Excluded       /*!< Channel excluded. */
#define PPI_CHG2_CH12_Included  PPI_CHG_CH12_Included       /*!< Channel included. */
#define PPI_CHG2_CH11_Pos       PPI_CHG_CH11_Pos            /*!< Position of CH11 field. */
#define PPI_CHG2_CH11_Msk       PPI_CHG_CH11_Msk            /*!< Bit mask of CH11 field. */
#define PPI_CHG2_CH11_Excluded  PPI_CHG_CH11_Excluded       /*!< Channel excluded. */
#define PPI_CHG2_CH11_Included  PPI_CHG_CH11_Included       /*!< Channel included. */
#define PPI_CHG2_CH10_Pos       PPI_CHG_CH10_Pos            /*!< Position of CH10 field. */
#define PPI_CHG2_CH10_Msk       PPI_CHG_CH10_Msk            /*!< Bit mask of CH10 field. */
#define PPI_CHG2_CH10_Excluded  PPI_CHG_CH10_Excluded       /*!< Channel excluded. */
#define PPI_CHG2_CH10_Included  PPI_CHG_CH10_Included       /*!< Channel included. */
#define PPI_CHG2_CH9_Pos        PPI_CHG_CH9_Pos             /*!< Position of CH9 field. */
#define PPI_CHG2_CH9_Msk        PPI_CHG_CH9_Msk             /*!< Bit mask of CH9 field. */
#define PPI_CHG2_CH9_Excluded   PPI_CHG_CH9_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH9_Included   PPI_CHG_CH9_Included        /*!< Channel included. */
#define PPI_CHG2_CH8_Pos        PPI_CHG_CH8_Pos             /*!< Position of CH8 field. */
#define PPI_CHG2_CH8_Msk        PPI_CHG_CH8_Msk             /*!< Bit mask of CH8 field. */
#define PPI_CHG2_CH8_Excluded   PPI_CHG_CH8_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH8_Included   PPI_CHG_CH8_Included        /*!< Channel included. */
#define PPI_CHG2_CH7_Pos        PPI_CHG_CH7_Pos             /*!< Position of CH7 field. */
#define PPI_CHG2_CH7_Msk        PPI_CHG_CH7_Msk             /*!< Bit mask of CH7 field. */
#define PPI_CHG2_CH7_Excluded   PPI_CHG_CH7_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH7_Included   PPI_CHG_CH7_Included        /*!< Channel included. */
#define PPI_CHG2_CH6_Pos        PPI_CHG_CH6_Pos             /*!< Position of CH6 field. */
#define PPI_CHG2_CH6_Msk        PPI_CHG_CH6_Msk             /*!< Bit mask of CH6 field. */
#define PPI_CHG2_CH6_Excluded   PPI_CHG_CH6_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH6_Included   PPI_CHG_CH6_Included        /*!< Channel included. */
#define PPI_CHG2_CH5_Pos        PPI_CHG_CH5_Pos             /*!< Position of CH5 field. */
#define PPI_CHG2_CH5_Msk        PPI_CHG_CH5_Msk             /*!< Bit mask of CH5 field. */
#define PPI_CHG2_CH5_Excluded   PPI_CHG_CH5_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH5_Included   PPI_CHG_CH5_Included        /*!< Channel included. */
#define PPI_CHG2_CH4_Pos        PPI_CHG_CH4_Pos             /*!< Position of CH4 field. */
#define PPI_CHG2_CH4_Msk        PPI_CHG_CH4_Msk             /*!< Bit mask of CH4 field. */
#define PPI_CHG2_CH4_Excluded   PPI_CHG_CH4_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH4_Included   PPI_CHG_CH4_Included        /*!< Channel included. */
#define PPI_CHG2_CH3_Pos        PPI_CHG_CH3_Pos             /*!< Position of CH3 field. */
#define PPI_CHG2_CH3_Msk        PPI_CHG_CH3_Msk             /*!< Bit mask of CH3 field. */
#define PPI_CHG2_CH3_Excluded   PPI_CHG_CH3_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH3_Included   PPI_CHG_CH3_Included        /*!< Channel included. */
#define PPI_CHG2_CH2_Pos        PPI_CHG_CH2_Pos             /*!< Position of CH2 field. */
#define PPI_CHG2_CH2_Msk        PPI_CHG_CH2_Msk             /*!< Bit mask of CH2 field. */
#define PPI_CHG2_CH2_Excluded   PPI_CHG_CH2_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH2_Included   PPI_CHG_CH2_Included        /*!< Channel included. */
#define PPI_CHG2_CH1_Pos        PPI_CHG_CH1_Pos             /*!< Position of CH1 field. */
#define PPI_CHG2_CH1_Msk        PPI_CHG_CH1_Msk             /*!< Bit mask of CH1 field. */
#define PPI_CHG2_CH1_Excluded   PPI_CHG_CH1_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH1_Included   PPI_CHG_CH1_Included        /*!< Channel included. */
#define PPI_CHG2_CH0_Pos        PPI_CHG_CH0_Pos             /*!< Position of CH0 field. */
#define PPI_CHG2_CH0_Msk        PPI_CHG_CH0_Msk             /*!< Bit mask of CH0 field. */
#define PPI_CHG2_CH0_Excluded   PPI_CHG_CH0_Excluded        /*!< Channel excluded. */
#define PPI_CHG2_CH0_Included   PPI_CHG_CH0_Included        /*!< Channel included. */

#define PPI_CHG3_CH15_Pos       PPI_CHG_CH15_Pos            /*!< Position of CH15 field. */
#define PPI_CHG3_CH15_Msk       PPI_CHG_CH15_Msk            /*!< Bit mask of CH15 field. */
#define PPI_CHG3_CH15_Excluded  PPI_CHG_CH15_Excluded       /*!< Channel excluded. */
#define PPI_CHG3_CH15_Included  PPI_CHG_CH15_Included       /*!< Channel included. */
#define PPI_CHG3_CH14_Pos       PPI_CHG_CH14_Pos            /*!< Position of CH14 field. */
#define PPI_CHG3_CH14_Msk       PPI_CHG_CH14_Msk            /*!< Bit mask of CH14 field. */
#define PPI_CHG3_CH14_Excluded  PPI_CHG_CH14_Excluded       /*!< Channel excluded. */
#define PPI_CHG3_CH14_Included  PPI_CHG_CH14_Included       /*!< Channel included. */
#define PPI_CHG3_CH13_Pos       PPI_CHG_CH13_Pos            /*!< Position of CH13 field. */
#define PPI_CHG3_CH13_Msk       PPI_CHG_CH13_Msk            /*!< Bit mask of CH13 field. */
#define PPI_CHG3_CH13_Excluded  PPI_CHG_CH13_Excluded       /*!< Channel excluded. */
#define PPI_CHG3_CH13_Included  PPI_CHG_CH13_Included       /*!< Channel included. */
#define PPI_CHG3_CH12_Pos       PPI_CHG_CH12_Pos            /*!< Position of CH12 field. */
#define PPI_CHG3_CH12_Msk       PPI_CHG_CH12_Msk            /*!< Bit mask of CH12 field. */
#define PPI_CHG3_CH12_Excluded  PPI_CHG_CH12_Excluded       /*!< Channel excluded. */
#define PPI_CHG3_CH12_Included  PPI_CHG_CH12_Included       /*!< Channel included. */
#define PPI_CHG3_CH11_Pos       PPI_CHG_CH11_Pos            /*!< Position of CH11 field. */
#define PPI_CHG3_CH11_Msk       PPI_CHG_CH11_Msk            /*!< Bit mask of CH11 field. */
#define PPI_CHG3_CH11_Excluded  PPI_CHG_CH11_Excluded       /*!< Channel excluded. */
#define PPI_CHG3_CH11_Included  PPI_CHG_CH11_Included       /*!< Channel included. */
#define PPI_CHG3_CH10_Pos       PPI_CHG_CH10_Pos            /*!< Position of CH10 field. */
#define PPI_CHG3_CH10_Msk       PPI_CHG_CH10_Msk            /*!< Bit mask of CH10 field. */
#define PPI_CHG3_CH10_Excluded  PPI_CHG_CH10_Excluded       /*!< Channel excluded. */
#define PPI_CHG3_CH10_Included  PPI_CHG_CH10_Included       /*!< Channel included. */
#define PPI_CHG3_CH9_Pos        PPI_CHG_CH9_Pos             /*!< Position of CH9 field. */
#define PPI_CHG3_CH9_Msk        PPI_CHG_CH9_Msk             /*!< Bit mask of CH9 field. */
#define PPI_CHG3_CH9_Excluded   PPI_CHG_CH9_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH9_Included   PPI_CHG_CH9_Included        /*!< Channel included. */
#define PPI_CHG3_CH8_Pos        PPI_CHG_CH8_Pos             /*!< Position of CH8 field. */
#define PPI_CHG3_CH8_Msk        PPI_CHG_CH8_Msk             /*!< Bit mask of CH8 field. */
#define PPI_CHG3_CH8_Excluded   PPI_CHG_CH8_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH8_Included   PPI_CHG_CH8_Included        /*!< Channel included. */
#define PPI_CHG3_CH7_Pos        PPI_CHG_CH7_Pos             /*!< Position of CH7 field. */
#define PPI_CHG3_CH7_Msk        PPI_CHG_CH7_Msk             /*!< Bit mask of CH7 field. */
#define PPI_CHG3_CH7_Excluded   PPI_CHG_CH7_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH7_Included   PPI_CHG_CH7_Included        /*!< Channel included. */
#define PPI_CHG3_CH6_Pos        PPI_CHG_CH6_Pos             /*!< Position of CH6 field. */
#define PPI_CHG3_CH6_Msk        PPI_CHG_CH6_Msk             /*!< Bit mask of CH6 field. */
#define PPI_CHG3_CH6_Excluded   PPI_CHG_CH6_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH6_Included   PPI_CHG_CH6_Included        /*!< Channel included. */
#define PPI_CHG3_CH5_Pos        PPI_CHG_CH5_Pos             /*!< Position of CH5 field. */
#define PPI_CHG3_CH5_Msk        PPI_CHG_CH5_Msk             /*!< Bit mask of CH5 field. */
#define PPI_CHG3_CH5_Excluded   PPI_CHG_CH5_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH5_Included   PPI_CHG_CH5_Included        /*!< Channel included. */
#define PPI_CHG3_CH4_Pos        PPI_CHG_CH4_Pos             /*!< Position of CH4 field. */
#define PPI_CHG3_CH4_Msk        PPI_CHG_CH4_Msk             /*!< Bit mask of CH4 field. */
#define PPI_CHG3_CH4_Excluded   PPI_CHG_CH4_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH4_Included   PPI_CHG_CH4_Included        /*!< Channel included. */
#define PPI_CHG3_CH3_Pos        PPI_CHG_CH3_Pos             /*!< Position of CH3 field. */
#define PPI_CHG3_CH3_Msk        PPI_CHG_CH3_Msk             /*!< Bit mask of CH3 field. */
#define PPI_CHG3_CH3_Excluded   PPI_CHG_CH3_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH3_Included   PPI_CHG_CH3_Included        /*!< Channel included. */
#define PPI_CHG3_CH2_Pos        PPI_CHG_CH2_Pos             /*!< Position of CH2 field. */
#define PPI_CHG3_CH2_Msk        PPI_CHG_CH2_Msk             /*!< Bit mask of CH2 field. */
#define PPI_CHG3_CH2_Excluded   PPI_CHG_CH2_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH2_Included   PPI_CHG_CH2_Included        /*!< Channel included. */
#define PPI_CHG3_CH1_Pos        PPI_CHG_CH1_Pos             /*!< Position of CH1 field. */
#define PPI_CHG3_CH1_Msk        PPI_CHG_CH1_Msk             /*!< Bit mask of CH1 field. */
#define PPI_CHG3_CH1_Excluded   PPI_CHG_CH1_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH1_Included   PPI_CHG_CH1_Included        /*!< Channel included. */
#define PPI_CHG3_CH0_Pos        PPI_CHG_CH0_Pos             /*!< Position of CH0 field. */
#define PPI_CHG3_CH0_Msk        PPI_CHG_CH0_Msk             /*!< Bit mask of CH0 field. */
#define PPI_CHG3_CH0_Excluded   PPI_CHG_CH0_Excluded        /*!< Channel excluded. */
#define PPI_CHG3_CH0_Included   PPI_CHG_CH0_Included        /*!< Channel included. */

#define RADIO_TXPOWER_TXPOWER_Neg40dBm  RADIO_TXPOWER_TXPOWER_Neg30dBm  /*!< -30dBm. */

/*lint --flb "Leave library region" */

#endif /* NRF51_DEPRECATED_H */

