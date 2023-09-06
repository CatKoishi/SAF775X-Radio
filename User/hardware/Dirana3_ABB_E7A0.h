/******************************************************************************/
/*                                                                            */
/*  (c) 2014 NXP B.V., All rights reserved                                    */
/*                                                                            */
/*  This source code and any compilation or derivative thereof is the         */
/*  proprietary information of NXP B.V. and is confidential in nature.        */
/*  Under no circumstances is this software to be exposed to or placed        */
/*  under an Open Source License of any type without the expressed            */
/*  written permission of NXP B.V..                                           */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *  C-Header file for Dirana3 SRC Software 7.1                                *
 ******************************************************************************
 *
 *  $Revision: 4367 $
 *
 *  $Date: 2014-06-04 15:24:03 +0530 (Wed, 04 Jun 2014) $
 *
 *
 *  File name:     Dirana3_ABB_E7A0.h
 *
 *  Purpose:       This file contains defines for all handles that are
 *                 mentioned in the audio user manual version TBD
 *                 The addresses are only valid for Dirana3 SRC Software
 *                 version 7.1
 *
 ******************************************************************************/

#ifndef DIRANA3_ABB_E7A0_H


/* Memory offsets of SDSP */
#define SDSP_X_OFFSET                                0xF30000UL
#define SDSP_Y_OFFSET                                0xF34000UL


/* SDSP X memory */
#define SDSP_X_SRC_CtrlTS0                           0xF30382UL
#define SDSP_X_SRC_CtrlTS1                           0xF30385UL
#define SDSP_X_SRC_CtrlTS2                           0xF30386UL
#define SDSP_X_SRC_CtrlTS3                           0xF30387UL
#define SDSP_X_SRC_CtrlTS4                           0xF30388UL
#define SDSP_X_SRC_CtrlTS5                           0xF30389UL

#define SDSP_X_pSRCInputFlag8                        0xF30481UL
#define SDSP_X_InputFlagIIS_BBP_IISIN0               0xF304B3UL
#define SDSP_X_InputFlagIIS_BBP_IISIN1               0xF304B7UL
#define SDSP_X_InputFlagIIS_IN0                      0xF30483UL
#define SDSP_X_InputFlagIIS_IN1_SD0                  0xF30487UL
#define SDSP_X_InputFlagIIS_IN2                      0xF30493UL
#define SDSP_X_InputFlagIIS_IN3                      0xF30497UL
#define SDSP_X_InputFlagIIS_IN4                      0xF3049BUL
#define SDSP_X_InputFlagSPDIFIN0                     0xF304FBUL
#define SDSP_X_InputFlagSPDIFIN1                     0xF304FFUL
#define SDSP_X_InputFlagTDMIN0                       0xF304BFUL
#define SDSP_X_InputFlagTDMIN1                       0xF304C9UL
#define SDSP_X_InputFlagTDMIN2                       0xF304D3UL
#define SDSP_X_InputFlagTDMIN3                       0xF304DDUL
#define SDSP_X_InputFlagTDMIN4                       0xF304E7UL
#define SDSP_X_InputFlagTDMIN5                       0xF304F1UL
#define SDSP_X_InputFlagIIS_HOST0_IISIN0             0xF3049FUL
#define SDSP_X_InputFlagIIS_HOST0_IISIN1             0xF304A3UL
#define SDSP_X_InputFlagIIS_HOST0_IISIN2             0xF304A7UL
#define SDSP_X_InputFlagIIS_HOST1_IISIN0             0xF304ABUL
#define SDSP_X_InputFlagIIS_HOST1_IISIN1             0xF304AFUL
#define SDSP_X_pSRCInputData8                        0xF3050FUL

/* SDSP X memory relative addresses */
#define SDSP_X_SRC_CtrlTS0_REL                       0x000382UL
#define SDSP_X_SRC_CtrlTS1_REL                       0x000385UL
#define SDSP_X_SRC_CtrlTS2_REL                       0x000386UL
#define SDSP_X_SRC_CtrlTS3_REL                       0x000387UL
#define SDSP_X_SRC_CtrlTS4_REL                       0x000388UL
#define SDSP_X_SRC_CtrlTS5_REL                       0x000389UL

#define SDSP_X_pSRCInputFlag8_REL                    0x000481UL
#define SDSP_X_InputFlagIIS_BBP_IISIN0_REL           0x0004B3UL
#define SDSP_X_InputFlagIIS_BBP_IISIN1_REL           0x0004B7UL
#define SDSP_X_InputFlagIIS_IN0_REL                  0x000483UL
#define SDSP_X_InputFlagIIS_IN1_SD0_REL              0x000487UL
#define SDSP_X_InputFlagIIS_IN2_REL                  0x000493UL
#define SDSP_X_InputFlagIIS_IN3_REL                  0x000497UL
#define SDSP_X_InputFlagIIS_IN4_REL                  0x00049BUL
#define SDSP_X_InputFlagSPDIFIN0_REL                 0x0004FBUL
#define SDSP_X_InputFlagSPDIFIN1_REL                 0x0004FFUL
#define SDSP_X_InputFlagTDMIN0_REL                   0x0004BFUL
#define SDSP_X_InputFlagTDMIN1_REL                   0x0004C9UL
#define SDSP_X_InputFlagTDMIN2_REL                   0x0004D3UL
#define SDSP_X_InputFlagTDMIN3_REL                   0x0004DDUL
#define SDSP_X_InputFlagTDMIN4_REL                   0x0004E7UL
#define SDSP_X_InputFlagTDMIN5_REL                   0x0004F1UL
#define SDSP_X_InputFlagIIS_HOST0_IISIN0_REL         0x00049FUL
#define SDSP_X_InputFlagIIS_HOST0_IISIN1_REL         0x0004A3UL
#define SDSP_X_InputFlagIIS_HOST0_IISIN2_REL         0x0004A7UL
#define SDSP_X_InputFlagIIS_HOST1_IISIN0_REL         0x0004ABUL
#define SDSP_X_InputFlagIIS_HOST1_IISIN1_REL         0x0004AFUL
#define SDSP_X_pSRCInputData8_REL                    0x00050FUL

#define DIRANA3_ABB_E7A0_H
#endif
