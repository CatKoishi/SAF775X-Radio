/*!
    \file    main.h
    \brief   the header file of main 

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x 
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#ifndef MAIN_H
#define MAIN_H

/* led spark function */
#include "stdbool.h"
#include "gd32f30x.h"
#include "rds.h"

#define MENU_MAIN_INDEX    6
#define MENU_DISP_INDEX    6
#define MENU_AUDIO_INDEX    8
#define MENU_RADIO_INDEX    12
#define MENU_ATS_INDEX    6
#define MENU_DEVICE_INDEX    5

#define FLASH_DEVICE    0
#define FLASH_RADIO    1
#define FLASH_DISP    2
#define FLASH_AUDIO    3

//    PAGE    0       1       2     3-8      9          10          11      12
// CONTENT DEVICE | RADIO | DISP | AUDIO | CH_FM | CH_LW+CH_NUM | CH_MW | CH_SW


struct time
{
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint16_t year;
	uint32_t timestamp;
};

struct info
{
	uint32_t pid[4]; // flashID | master | subversion | date
	uint32_t uid[3]; // 96bit
	uint32_t tid;
};

struct device
{
	struct info sInfo;
	struct time sTime;
	uint16_t nBatVolt;
	uint8_t bAutoMono;
	uint8_t bSoftReboot;
};


void EXTI_Callback(uint8_t exti_line);
void TIM_Callback(uint8_t tim);
void ADC_Callback(uint8_t adc, char group);
void RTC_Callback(uint8_t line);

#endif /* MAIN_H */
