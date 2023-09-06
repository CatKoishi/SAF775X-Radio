#ifndef __GUI_H
#define __GUI_H

#include "gd32f10x.h"
#include "stdbool.h"
#include "rds.h"

/******************************************************************************/

uint8_t num2str(uint16_t num, uint8_t * str);
uint8_t myround(uint16_t number);

void GUI_Main(uint16_t freq, uint8_t band, uint16_t step, uint8_t FMfilter, uint8_t AMfilter, int8_t RSSI, bool bias, uint8_t bat, uint8_t volume, bool stereo, bool mute, uint8_t channel, bool fsel, struct RDSData *rds, bool force);
void GUI_Menu(uint8_t sel, bool force);
void GUI_Menu_Display(uint8_t sel, uint8_t lumi, uint16_t BLtime, uint8_t contrast, uint16_t BLB, uint8_t bias, uint8_t fps, bool inv, bool force);
void GUI_Menu_Audio(uint8_t sel, bool mph, bool ceq, uint8_t deempahsis, uint8_t stereo, uint8_t output, int8_t gain, bool bass, bool force);
void GUI_Menu_Tune(uint8_t sel, uint8_t atsmode, uint8_t threshold, uint16_t fmin, uint16_t fmax, bool bias, int8_t offset, uint8_t step, bool force);
void GUI_Menu_Memory(uint16_t sel, uint8_t band, uint8_t ch_num, uint16_t * ch_freq, bool force);
void GUI_Menu_Device(uint8_t dsp_id,uint8_t dsp_hw1,uint8_t dsp_hw2,uint8_t dsp_sw1,uint8_t dsp_sw2,uint32_t *mcuid,uint16_t flash,uint16_t sram,uint16_t temperature,uint16_t vref,uint16_t irc40k,bool force);
void GUI_Menu_Advance(uint8_t sel, uint8_t amnb, uint8_t fmnb, uint8_t amsm, uint8_t fmsm, uint8_t amhc, uint8_t fmhc, uint8_t fmsc, uint8_t fmsb, bool force);
void GUI_Menu_APO(uint16_t time, bool state, bool force);
void GUI_Menu_About(uint8_t sel, bool force);
void GUI_Startup(void);
void GUI_SeekALL(uint16_t fmin, uint16_t fmax, uint16_t fnow, uint8_t number, bool force);
void GUI_SeekDIR(int8_t dir);
void GUI_WaveGen(uint8_t wsel, uint8_t asel, uint8_t multi, uint16_t mode,uint16_t ch1f, int16_t ch1a, uint16_t ch2f, int16_t ch2a, bool force);
// 需要构建主页
//GUI_Main(nBandFreq[nBandMode], nBandMode, nBandStep[nBandMode][nFreqStep[nBandMode]], nFMFilter, nAMFilter, nRSSI, bBIAS, BAT_Volt, nVolume, bSTIN, bMuted, nBandCh[nBandMode], bFilterSel, true);
// 需要刷新主页
//GUI_Main(nBandFreq[nBandMode], nBandMode, nBandStep[nBandMode][nFreqStep[nBandMode]], nFMFilter, nAMFilter, nRSSI, bBIAS, BAT_Volt, nVolume, bSTIN, bMuted, nBandCh[nBandMode], bFilterSel, false);

//主菜单
//GUI_Menu(sel, bool force);
//菜单“显示”
//GUI_Menu_Display(sel, nBL, nBLTime, nContrast, bool force);
//菜单“音频”
//GUI_Menu_Audio(sel, bFMENH, nDeemphasis, bFMST, bBASS, bool force);
//菜单“调谐”
//GUI_Menu_Tune(sel, bATSMODE, nLowSig, bBIAS, bool force);
//菜单“关于”
//GUI_Menu_About(sel, bool force);


#endif

