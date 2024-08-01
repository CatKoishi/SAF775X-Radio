/**
  ************************************* Copyright ******************************   
  *                 (C) Copyright 2023,NyaKoishi,China,GCU.
  *                            All Rights Reserved
  *
  * @file      : func.h
  * @author    : NyaKoishi
  * @version   : V1.0.0
  * @date      : 2023-06-13
  * @brief     : Header for func.h file
  * @attention : 
  * 
  ******************************************************************************
 */



#ifndef __FUNC_H_
#define __FUNC_H_

#include "main.h"

// DAC_Channel
#define BLSIG_CH		1
#define RFSIG_CH		2



float inRangeFloat(float min, float max, float num);
int32_t inRangeInt(int32_t min, int32_t max, int32_t num);
int32_t inRangeLoop(int32_t min, int32_t max, int32_t num, int32_t dir);
bool checkRange(int min, int max, int num);

uint32_t myabs(int32_t number);
int myround(float num);

void DAC_Start(uint8_t ch);
void DAC_Stop(uint8_t ch);
void DAC_OutVal(uint8_t ch, uint16_t val);
uint16_t DAC_GetValue(uint8_t ch);


void GetMCUInfo(uint32_t *id, uint16_t *flash, uint16_t *sram);

#endif

