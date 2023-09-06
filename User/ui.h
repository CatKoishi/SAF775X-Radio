/**
  ************************************* Copyright ******************************   
  *                 (C) Copyright 2023,NyaKoishi,China,GCU.
  *                            All Rights Reserved
  *
  * @file      : ui.h
  * @author    : NyaKoishi
  * @version   : V1.0.0
  * @date      : 2023-05-30
  * @brief     : Header for ui.h file
  * @attention : 
  * 
  ******************************************************************************
 */



#ifndef __UI_H_
#define __UI_H_

#include "SAF775X.h"

void UI_Main(bool init);
void UI_Menu(int8_t index, bool init);
void UI_Display(int8_t index, bool init);
void UI_Audio(int8_t index, int8_t band, int8_t sel, bool init);
void UI_Radio(int8_t index, bool init);
void UI_Search(int8_t index, bool init);
void UI_Device(int8_t index, bool init);
void UI_About(int8_t index, bool init);

void GUI_SeekALL(uint16_t fmin, uint16_t fmax, uint16_t fnow, uint8_t number, bool force);

#endif

