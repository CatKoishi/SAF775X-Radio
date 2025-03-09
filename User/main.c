#include "main.h"
#include "gd32f30x.h"
#include "systick.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "init.h"
#include "SAF775X.h"
#include "lcd.h"
#include "ui.h"
#include "flash/flash.h"
#include "func.h"
#include "rtc.h"

#include "Dirana3BasicDSP.h"

#include "flashdb.h"


#define KEY_MENU    0
#define KEY_UP      1
#define KEY_DOWN    2
#define KEY_OK      3
#define KEY_LENC    4
#define KEY_RENC    5

#define KEY_NUM     6

#define KEY_FREE    0
#define KEY_PRESS   1
#define KEY_LP      2

/**********************************Key & ENC***********************************/

volatile uint8_t lastA1 = 1;
volatile uint8_t lastA2 = 1;
volatile uint8_t nowA1 = 1;
volatile uint8_t nowA2 = 1;

volatile uint8_t lastDET = 0;
volatile uint8_t nowDET = 0;

volatile int8_t lcode = 0;
volatile int8_t rcode = 0;

volatile uint8_t keyValue[KEY_NUM];

/*************************************Configs**********************************/

uint32_t bootCount = 0;     // 开机次数
uint32_t accBootTime = 0;   // 累计运行时间(s)

struct Dirana3Radio sTuner;

struct basicControl sAudioBasic;
struct toneControl sAudioTone;
struct graphicEQ sAudioEQ;
struct filterSystem sAudioFilter;
struct keyFunc sAudioKeyFunc;

struct device sDevice = {
  .bAutoMono=true,
  .bSoftReboot=true,
  .sInfo.pid[0]=8,  // flash arrange
  .sInfo.pid[1]=2,  // version
  .sInfo.pid[2]=0,  // subversion
  .sInfo.pid[3]=20250308,
};

#define FDB_LOG_TAG "[kvdb]"

/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
  {"codeName", "NYK7751", 0}, /* string KV */
  {"Author", "NyaKoishi", 0}, /* string KV */
  {"bootCount", &bootCount, sizeof(bootCount)}, // Int Type
  {"bootTime", &accBootTime, sizeof(accBootTime)}
};

/* KVDB object */
static struct fdb_kvdb kvdb = { 0 };

/************************************Var***************************************/

// DAV,GSA[0,8],VU
volatile uint8_t nGsaVal[10] = {0};
volatile uint32_t nQPeakDet;

struct RDSBuffer sRDSBuffer;
struct RDSData sRDSData;

/************************************Indicator*********************************/

volatile bool bIIC_BUSY = false;
volatile bool bMenu = false;
volatile bool bMenuLCD = false;
volatile bool bFlagRFS = false;
volatile bool bFlagGSA = false;
volatile bool bFlagRDS = false;

volatile bool bFlagReBoot = false;

volatile bool bSeek = false;

volatile bool bFilterSel = false;  //是否设置带宽

volatile int nSyncTimer = 600;
volatile bool bFlagSync = false;

/************************************Channel***********************************/

CHANNEL nChannel[NUM_BANDS] = {
  { CHS_FM, 0, 0, {0} },
  { CHS_LW, 0, 0, {0} },
  { CHS_MW, 0, 0, {0} },
  { CHS_SW, 0, 0, {0} },
};

/************************************ Flash **********************************/

char bandDbName[NUM_BANDS][7] = {"bandFm", "bandLw", "bandMw", "bandSw"};

void saveSettings(uint8_t cfgID)
{
  struct fdb_blob blob;
  fdb_err_t err;
  if(cfgID == CFG_DEVICE || cfgID == CFG_ALL) {
    err = fdb_kv_set_blob(&kvdb, "version", fdb_blob_make(&blob, &sDevice, sizeof(sDevice)));
  }
  
  if(cfgID == CFG_RADIO || cfgID == CFG_ALL) {
    err = fdb_kv_set_blob(&kvdb, "cfgTuner", fdb_blob_make(&blob, &sTuner, sizeof(sTuner)));
  }
  
  if(cfgID == CFG_DISP || cfgID == CFG_ALL) {
    err = fdb_kv_set_blob(&kvdb, "cfgDisplay", fdb_blob_make(&blob, &sDisplay, sizeof(sDisplay)));
  }
  
  if(cfgID == CFG_AUDIO || cfgID == CFG_ALL) {
    err = fdb_kv_set_blob(&kvdb, "cfgAudio", fdb_blob_make(&blob, &sAudioBasic, sizeof(sAudioBasic)));
    err = fdb_kv_set_blob(&kvdb, "cfgTone", fdb_blob_make(&blob, &sAudioTone, sizeof(sAudioTone)));
    err = fdb_kv_set_blob(&kvdb, "cfgEq", fdb_blob_make(&blob, &sAudioEQ, sizeof(sAudioEQ)));
    err = fdb_kv_set_blob(&kvdb, "cfgFilter", fdb_blob_make(&blob, &sAudioFilter, sizeof(sAudioFilter)));
    err = fdb_kv_set_blob(&kvdb, "cfgAuKey", fdb_blob_make(&blob, &sAudioKeyFunc, sizeof(sAudioKeyFunc)));
  }
  
}

void readSettings(uint8_t cfgID)
{
  struct fdb_blob blob;
  int needUpdate = 0;
  
  if(cfgID == CFG_DEVICE || cfgID == CFG_ALL) {
    fdb_kv_get_blob(&kvdb, "version", fdb_blob_make(&blob, &sDevice, sizeof(sDevice)));
    needUpdate += blob.saved.len;
  }
  
  if(cfgID == CFG_RADIO || cfgID == CFG_ALL) {
    fdb_kv_get_blob(&kvdb, "cfgTuner", fdb_blob_make(&blob, &sTuner, sizeof(sTuner)));
    needUpdate += blob.saved.len;
  }
  
  if(cfgID == CFG_DISP || cfgID == CFG_ALL) {
    fdb_kv_get_blob(&kvdb, "cfgDisplay", fdb_blob_make(&blob, &sDisplay, sizeof(sDisplay)));
    needUpdate += blob.saved.len;
  }
  
  if(cfgID == CFG_AUDIO || cfgID == CFG_ALL) {
    fdb_kv_get_blob(&kvdb, "cfgAudio", fdb_blob_make(&blob, &sAudioBasic, sizeof(sAudioBasic)));
    needUpdate += blob.saved.len;
    fdb_kv_get_blob(&kvdb, "cfgTone", fdb_blob_make(&blob, &sAudioTone, sizeof(sAudioTone)));
    needUpdate += blob.saved.len;
    fdb_kv_get_blob(&kvdb, "cfgEq", fdb_blob_make(&blob, &sAudioEQ, sizeof(sAudioEQ)));
    needUpdate += blob.saved.len;
    fdb_kv_get_blob(&kvdb, "cfgFilter", fdb_blob_make(&blob, &sAudioFilter, sizeof(sAudioFilter)));
    needUpdate += blob.saved.len;
    fdb_kv_get_blob(&kvdb, "cfgAuKey", fdb_blob_make(&blob, &sAudioKeyFunc, sizeof(sAudioKeyFunc)));
    needUpdate += blob.saved.len;
  }
  
  
}

void saveChannels(uint8_t band)
{
  if(band >= NUM_BANDS)
    return;
  struct fdb_blob blob;
  
  fdb_kv_set_blob(&kvdb, bandDbName[band], fdb_blob_make(&blob, &nChannel[band], sizeof(CHANNEL)));
}

void readChannels(uint8_t band)
{
  if(band >= NUM_BANDS)
    return;
  struct fdb_blob blob;
  CHANNEL tmp = {0};
  
  fdb_kv_get_blob(&kvdb, bandDbName[band], fdb_blob_make(&blob, &tmp, sizeof(CHANNEL)));
  if(blob.saved.len > 0) {
    if(tmp.chanNum > 0 && tmp.chanNum <= nChannel[band].chanMax) {
      memcpy(&nChannel[band], &tmp, sizeof(CHANNEL));
    }
  }
  
}

// curFreq*4, curBand, curFilter*2, curVolumn*2, curMute
void saveBasicPara(void) {
  struct fdb_blob blob;
  uint16_t bSet[10] = {0};
  
  bSet[0] = sTuner.Radio.nBandMode;
  bSet[1] = sTuner.Radio.nBandFreq[0];
  bSet[2] = sTuner.Radio.nBandFreq[1];
  bSet[3] = sTuner.Radio.nBandFreq[2];
  bSet[4] = sTuner.Radio.nBandFreq[3];
  bSet[5] = sTuner.Config.nBandFilter[0];
  bSet[6] = sTuner.Config.nBandFilter[1];
  bSet[7] = sTuner.Audio.nVolume[0];
  bSet[8] = sTuner.Audio.nVolume[1];
  bSet[9] = sTuner.Audio.bMuted;
  
  fdb_kv_set_blob(&kvdb, "basicPara", fdb_blob_make(&blob, &bSet, sizeof(bSet)));
}

void readBasicPara(void) {
  struct fdb_blob blob;
  uint16_t bSet[10] = {0};
  
  fdb_kv_get_blob(&kvdb, "basicPara", fdb_blob_make(&blob, &bSet, sizeof(bSet)));
  if(blob.saved.len > 0) {
    sTuner.Radio.nBandMode       = bSet[0];
    sTuner.Radio.nBandFreq[0]    = bSet[1];
    sTuner.Radio.nBandFreq[1]    = bSet[2];
    sTuner.Radio.nBandFreq[2]    = bSet[3];
    sTuner.Radio.nBandFreq[3]    = bSet[4];
    sTuner.Config.nBandFilter[0] = bSet[5];
    sTuner.Config.nBandFilter[1] = bSet[6];
    sTuner.Audio.nVolume[0]      = bSet[7];
    sTuner.Audio.nVolume[1]      = bSet[8];
    sTuner.Audio.bMuted          = bSet[9];
  }
}

/************************************Function*********************************/

void flushKey(void)
{
  for(uint8_t i = 0; i < KEY_NUM; i++)
    keyValue[i] = 0;
}

void AdjustVolume(int8_t dir)
{
  int8_t vol = sTuner.Audio.nVolume[sTuner.Audio.index];
  
  vol = inRangeInt(MIN_VOL, MAX_VOL, vol+dir*2);
  
  SetVolume(vol);
}

void AdjustFreq(int8_t dir)
{
  uint16_t f;
  f = sTuner.Radio.nBandFreq[sTuner.Radio.nBandMode] + dir * nBandStep[sTuner.Radio.nBandMode][sTuner.Radio.nFreqStep[sTuner.Radio.nBandMode]];
  
  f = inRangeInt(nBandFMin[sTuner.Radio.nBandMode], nBandFMax[sTuner.Radio.nBandMode], f);
  
  if(sTuner.Radio.nRFMode == RFMODE_FM) {
    TuneFreq(f, Jump);
  } else {
    TuneFreq(f, Preset);
  }
  
  
  RDS_Refresh();
}

void AdjustFilter(int8_t dir)
{
  int8_t index = sTuner.Config.nBandFilter[sTuner.Radio.nRFMode];
  
  index += dir;
  index = inRangeInt(0,NUM_FILTERS-1,index);
  
  SetFilter(index);
}

void AdjustAGC(int8_t dir)
{
  int8_t index = sTuner.Config.nBandAGC[sTuner.Radio.nRFMode];
  
  index += dir;
  index = inRangeInt(0,3,index);
  
  SetAGC(index);
}

void VolumeHandler(void)
{
  // 右编码器旋转
  if(rcode != 0)
  {
    AdjustVolume(rcode);
    rcode = 0;
  }
  
  // 右编码器按下
  if(keyValue[KEY_RENC] != 0)
  {
    if(keyValue[KEY_RENC] == KEY_PRESS)
    {
      SetMute(!(sTuner.Audio.bMuted));
    }
    else
    {
      if(bMenu == false) // change screen
      {
        
      }
    }
    keyValue[KEY_RENC] = 0;
  }
}

void reFillBackLightTimer(void)
{
  sDisplay.backTimeCounter = sDisplay.backTime*10;
  nSyncTimer = 600;
}

bool isSigOK(void)
{
  if(sTuner.Radio.nRFMode == RFMODE_FM)
  {
    if(sTuner.ATS.nATSMode != 0)  // Fast FM Judge
    {
      delay_ms(3);  // reliable rssi, first usn, wam, offset
      GetStatus();
      if(sTuner.Status.nQRS <= 1)
      {
        delay_ms(2);
        GetStatus();
      }
      if(sTuner.Status.nRSSI < sTuner.ATS.nSigThreshold || sTuner.Status.nUSN > 80 || sTuner.Status.nWAM > 80)
        return false;
      
      delay_ms(32);
      GetStatus();
      if(sTuner.Status.bSTIN == true && sTuner.Status.nRSSI > sTuner.ATS.nSigThreshold-8)
        return true;
      else if(sTuner.Status.nRSSI > sTuner.ATS.nSigThreshold && sTuner.Status.nUSN < 40 && sTuner.Status.nWAM < 40 && (sTuner.Status.nOffset > -8 && sTuner.Status.nOffset < 8))
        return true;
    }
    else  // Slow FM Judge
    {
      delay_ms(35);
      GetStatus();
      if(sTuner.Status.bSTIN == true && sTuner.Status.nRSSI > sTuner.ATS.nSigThreshold-12)
        return true;
      else if(sTuner.Status.nRSSI > sTuner.ATS.nSigThreshold && sTuner.Status.nUSN < 40 && sTuner.Status.nWAM < 40 && (sTuner.Status.nOffset > -8 && sTuner.Status.nOffset < 8))
        return true;
    }
  }
  else  // AM
  {
    if(sTuner.ATS.nATSMode != 0)  // Fast AM Judge
    {
      delay_ms(12);  // reliable offset, first rssi, usn
      GetStatus();
      if(sTuner.Status.nQRS <= 2)
      {
        delay_ms(8);
        GetStatus();
      }
      if((sTuner.Status.nOffset < -20 || sTuner.Status.nOffset > 20))
        return false;
      
      delay_ms(32);
      GetStatus();
      if(sTuner.Status.nRSSI > sTuner.ATS.nSigThreshold+10 && sTuner.Status.nACD < 0 && (sTuner.Status.nOffset > -10 && sTuner.Status.nOffset < 10))
        return true;
    }
    else  // Slow AM Judge
    {
      delay_ms(35);
      GetStatus();
      if(sTuner.Status.nRSSI > sTuner.ATS.nSigThreshold+10 && sTuner.Status.nACD < 0 && (sTuner.Status.nOffset > -15 && sTuner.Status.nOffset < 15))
        return true;
    }
  }
  return false;
}


void SearchCH(int8_t dir)
{
  bSeek = true;
  bool CHQ = false;
  uint16_t initFreq = sTuner.Radio.nBandFreq[sTuner.Radio.nBandMode];
  uint16_t cursorFreq = initFreq;
  uint8_t initFilt = sTuner.Config.nBandFilter[sTuner.Radio.nRFMode];
  
  RDS_Refresh();
  
  SetMute(true);
  
  if (sTuner.Radio.nRFMode == RFMODE_FM)
    SetFilter(3);
  else
    SetFilter(6);
  
  
  while(!CHQ)
  {
    //show animation
    //GUI_SeekDIR(dir);
    
    //Rotate to abort action
    if(lcode!=0)
    {
      lcode = 0;
      SetFilter(initFilt);
      TuneFreq(cursorFreq, Preset);
      UI_Main(true);
      SetMute(false);
      bSeek = false;
      return;
    }
    cursorFreq = cursorFreq + (dir * nBandStepATS[sTuner.Radio.nBandMode]);
    
    // out of range
    if(cursorFreq>nBandFMax[sTuner.Radio.nBandMode] || cursorFreq<nBandFMin[sTuner.Radio.nBandMode])
    {
      SetFilter(initFilt);
      TuneFreq(initFreq, Preset);
      UI_Main(true);
      SetMute(false);
      bSeek = false;
      return;
    }
    
    TuneFreq(cursorFreq, Search);
    CHQ = isSigOK();
  }
  
  SetFilter(initFilt);
  TuneFreq(cursorFreq, Preset);
  UI_Main(true);
  SetMute(false);
  bSeek = false;
}


void SearchAllCH()
{
  uint8_t ch[5] = {0};
  
  uint8_t j = 0;
  uint16_t i = 0;
  uint16_t fmin = 0;
  uint16_t fmax = 0;
  uint16_t nBandMode = sTuner.Radio.nBandMode;
  uint16_t initFreq = sTuner.Radio.nBandFreq[nBandMode];
  uint8_t initFilt = sTuner.Config.nBandFilter[sTuner.Radio.nRFMode];
  uint16_t cursorFreq = 0;
  bool CHQ = false;
  bSeek = true;
  SetMute(true);
  nChannel[nBandMode].chanNum = 0;
  
  if (sTuner.Radio.nRFMode == RFMODE_FM)
    SetFilter(3);
  else
    SetFilter(6);
  
  GUI_SeekALL(nBandFMin[nBandMode],nBandFMax[nBandMode],nBandFMin[nBandMode],nChannel[nBandMode].chanNum,true);
  
  if(sTuner.Radio.nBandMode == BAND_SW && sTuner.ATS.nATSMode != 0)
  {
    for(j = 0;j<14;j++)
    {
      cursorFreq = nFastATS[j][0];
      for(i = 0;i<(nFastATS[j][1]-nFastATS[j][0])/nBandStepATS[nBandMode];i++)
      {
        TuneFreq(cursorFreq, Search);
        CHQ = isSigOK();
        if(CHQ)
        {
          nChannel[nBandMode].chanFreq[nChannel[nBandMode].chanNum] = cursorFreq;
          nChannel[nBandMode].chanNum++;
          CHQ = false;
        }
        GUI_SeekALL(nFastATS[0][0],nFastATS[13][1],cursorFreq,nChannel[nBandMode].chanNum,false);
        cursorFreq = cursorFreq + nBandStepATS[nBandMode];
        
        if(keyValue[KEY_MENU] != 0 || nChannel[nBandMode].chanNum == nChannel[nBandMode].chanMax)
          break;
      }
      
      if(keyValue[KEY_MENU] != 0 || nChannel[nBandMode].chanNum == nChannel[nBandMode].chanMax)
      {
        break;
      }
    }
  }
  else
  {
    if(nBandMode == BAND_FM)
    {
      fmin = nFMATSRegion[sTuner.ATS.nFMRegion][0], fmax = nFMATSRegion[sTuner.ATS.nFMRegion][1];
    }
    else if(nBandMode == BAND_MW && nBandStepATS[BAND_MW] == 10)
    {
      fmin = 530,fmax = 1710;
    }
    else
    {
      fmin = nBandFMin[nBandMode],fmax = nBandFMax[nBandMode];
    }
    cursorFreq = fmin;
    for(i = 0;i<(fmax-fmin)/nBandStepATS[nBandMode];i++)
    {
      TuneFreq(cursorFreq, Search);
      CHQ = isSigOK();
      if(CHQ)
      {
        nChannel[nBandMode].chanFreq[nChannel[nBandMode].chanNum] = cursorFreq;
        nChannel[nBandMode].chanNum++;
        CHQ = false;
      }
      GUI_SeekALL(fmin,fmax,cursorFreq,nChannel[nBandMode].chanNum,false);
      cursorFreq = cursorFreq + nBandStepATS[nBandMode];
      
      if(keyValue[KEY_MENU] != 0 || nChannel[nBandMode].chanNum == nChannel[nBandMode].chanMax)  //ʖ¶¯͋³ö»򴦂ú
      {
        keyValue[KEY_MENU] = 0;
        break;
      }
    }
  }
  nChannel[nBandMode].nowIndex = 0;
  
  SetFilter(initFilt);
  TuneFreq(initFreq, Preset);
  SetMute(false);
  bSeek = false;
  if(keyValue[KEY_MENU] != 0)
    GUI_Text(98,54,-1,-1,"STOP",&Font12,COLOR_WHITE,COLOR_BLACK);
  else
    GUI_Text(98,54,-1,-1,"DONE",&Font12,COLOR_WHITE,COLOR_BLACK);
  
  saveChannels(nBandMode);
  flushKey();
}

void GoChannel(int8_t dir)
{
  int8_t index = 0;
  uint8_t flag = 0;
  uint16_t nBandMode = sTuner.Radio.nBandMode;
  
  RDS_Refresh();
  
  if(nChannel[nBandMode].chanNum != 0)  //Ӑ̨¼ǂ¼
  {
    if(dir>0)
    {
      for(index = 0; index<nChannel[nBandMode].chanNum; index++)
      {
        if(nChannel[nBandMode].chanFreq[index] > sTuner.Radio.nBandFreq[nBandMode])
        {
          nChannel[nBandMode].nowIndex = index;
          flag = 1;
          break;
        }
      }
      if(flag == 0)
      {
        nChannel[nBandMode].nowIndex = 0;
      }
    }
    else
    {
      for(index = nChannel[nBandMode].chanNum-1; index>=0; index--)
      {
        if(nChannel[nBandMode].chanFreq[index] < sTuner.Radio.nBandFreq[nBandMode])
        {
          nChannel[nBandMode].nowIndex = index;
          flag = 1;
          break;
        }
      }
      if(flag == 0)
      {
        nChannel[nBandMode].nowIndex = nChannel[nBandMode].chanNum-1;
      }
    }
    
    sTuner.Radio.nBandFreq[nBandMode] = nChannel[nBandMode].chanFreq[nChannel[nBandMode].nowIndex];
    if(nBandMode == BAND_FM)
      TuneFreq(sTuner.Radio.nBandFreq[nBandMode], Jump);
    else
      TuneFreq(sTuner.Radio.nBandFreq[nBandMode], Preset);
    
  }
}


bool AddChannel(void)
{
  uint16_t nBandMode = sTuner.Radio.nBandMode;
  if(nChannel[nBandMode].chanNum >= nChannel[nBandMode].chanMax)
    return false;  //full
  if(nChannel[nBandMode].chanNum == 0)
  {
    nChannel[nBandMode].chanFreq[0] = sTuner.Radio.nBandFreq[nBandMode];
    nChannel[nBandMode].chanNum++;
    nChannel[nBandMode].nowIndex = 0;
    return true;
  }
  
  uint8_t i = 0;
  for(i = nChannel[nBandMode].chanNum - 1;i >= 0;i--)
  {
    
    if(sTuner.Radio.nBandFreq[nBandMode]==nChannel[nBandMode].chanFreq[i])
    {
      nChannel[nBandMode].nowIndex = i;
      return true;  //same
    }
    else if(sTuner.Radio.nBandFreq[nBandMode]<nChannel[nBandMode].chanFreq[i])
      nChannel[nBandMode].chanFreq[i+1] = nChannel[nBandMode].chanFreq[i];  //º󒆍
    else if(sTuner.Radio.nBandFreq[nBandMode]>nChannel[nBandMode].chanFreq[i])
    {
      nChannel[nBandMode].chanFreq[i+1] = sTuner.Radio.nBandFreq[nBandMode];
      nChannel[nBandMode].nowIndex = i+1;
      break;
    }
    if(i == 0)
    {
      nChannel[nBandMode].chanFreq[i] = sTuner.Radio.nBandFreq[nBandMode];
      nChannel[nBandMode].nowIndex = i;
      break;
    }
  }
  nChannel[nBandMode].chanNum++;
  saveChannels(nBandMode);
  return true;
}

// index 1--nChannel[nBandMode].chanNum
bool DeleteChannel(uint8_t index)
{
  uint16_t nBandMode = sTuner.Radio.nBandMode;
  if(nChannel[nBandMode].chanNum == 0)  //empty memory
    return false;
  if(index > nChannel[nBandMode].chanNum)  //index out of range
    return false;
  
  nChannel[nBandMode].chanFreq[index-1] = 0;
  for(uint8_t i = index-1;i<nChannel[nBandMode].chanNum-1;i++)
  {
    nChannel[nBandMode].chanFreq[i] = nChannel[nBandMode].chanFreq[i+1];
  }
  nChannel[nBandMode].chanNum--;
  saveChannels(nBandMode);
  return true;
}


/************************************Menu*********************************/
void MenuDisplay(void)
{
  bMenuLCD = true;
  int8_t index = 0;
  
  UI_Display(index,true);
  lcd_update();
  while(1)
  {
    if(keyValue[KEY_UP] != 0)
    {
      keyValue[KEY_UP] = 0;
      if(index == 0)
        continue;
      index--;
      UI_Display(index,false);
      lcd_update();
    }
    if(keyValue[KEY_DOWN] != 0)
    {
      keyValue[KEY_DOWN] = 0;
      if(index == MENU_DISP_INDEX-1)
        continue;
      index++;
      UI_Display(index,false);
      lcd_update();
    }
    
    // 左编码器旋转
    if(lcode != 0)
    {
      switch(index)
      {
        case 0:{
          sDisplay.brightness = inRangeInt(0,100,(int8_t)sDisplay.brightness + lcode);
          DAC_OutVal(BLSIG_CH, sDisplay.brightness);
        };break;
        case 1:{
          sDisplay.backTime = inRangeInt(0,60,(int16_t)sDisplay.backTime+lcode*5);
          sDisplay.backTimeCounter = sDisplay.backTime*10;
        };break;
        case 2:{
          sDisplay.contrast = inRangeInt(85,360,sDisplay.contrast+lcode);
          DispContrast(sDisplay.contrast);
        };break;
        case 3:{
          //do nothing
        };break;
        case 4:{
          //do nothing
        };break;
        case 5:{
          sDisplay.greyLevel[0] = inRangeInt(0,sDisplay.greyLevel[1],(int8_t)sDisplay.greyLevel[0] + lcode);
          DispGreyLevel(sDisplay.greyLevel[0], sDisplay.greyLevel[1]);
        };break;
        case 6:{
          sDisplay.greyLevel[1] = inRangeInt(sDisplay.greyLevel[0],31,(int8_t)sDisplay.greyLevel[1] + lcode);
          DispGreyLevel(sDisplay.greyLevel[0], sDisplay.greyLevel[1]);
        };break;
        default : index = 0;break;
      }
      UI_Display(index,false);
      lcd_update();
      lcode = 0;
    }
    
    if(keyValue[KEY_OK] != 0)
    {
      if(index == 3)
      {
        sDisplay.invDisp = !sDisplay.invDisp;
        DispInverse(sDisplay.invDisp);
        UI_Display(index,false);
        lcd_update();
      }
      if(index == 4)
      {
        sDisplay.emiFree = !sDisplay.emiFree;
        UI_Display(index,false);
        lcd_update();
      }
      keyValue[KEY_OK] = 0;
    }
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      bMenuLCD = false;
      break;
    }
    
    VolumeHandler();
  }
  
  
}

void MenuAudio(void)
{
  // vol, dc, eq, tone
  const int8_t paraNum[MENU_AUDIO_INDEX] = {0,0,2,0};
  const int8_t bandNum[MENU_AUDIO_INDEX] = {3,1,9,3};
  int8_t index = 0;
  int8_t paraSel = 0;
  int8_t bandSel = 0;
  
  UI_Audio(index,bandSel,paraSel,true);
  
  while(1)
  {
    if(keyValue[KEY_UP] != 0)
    {
      keyValue[KEY_UP] = 0;
      if(index == 0)
        continue;
      index--;
      paraSel = 0, bandSel = 0;
      UI_Audio(index,bandSel,paraSel,false);
    }
    if(keyValue[KEY_DOWN] != 0)
    {
      keyValue[KEY_DOWN] = 0;
      if(index == MENU_AUDIO_INDEX-1)
        continue;
      index++;
      paraSel = 0, bandSel = 0;
      UI_Audio(index,bandSel,paraSel,false);
    }
    
    // 左编码器按下
    if(keyValue[KEY_LENC] != 0)
    {
      keyValue[KEY_LENC] = 0;
      paraSel = inRangeLoop(0,paraNum[index]-1,paraSel,1);
      UI_Audio(index,bandSel,paraSel,false);
    }
    
    // 左编码器旋转
    if(lcode != 0)
    {
      switch(index)
      {
        case 0:{ // main vol / audio gain & balance
          switch(bandSel)
          {
            case 0:{
              sAudioBasic.mainVol = inRangeInt(-48,24,sAudioBasic.mainVol+lcode);
              setMainVol(sAudioBasic.mainVol);
            };break;
            case 1:{
              sAudioBasic.balance[0] = inRangeInt(-48,0,sAudioBasic.balance[0]+lcode);
              setBalance('L', sAudioBasic.balance[0]);
            };break;
            case 2:{
              sAudioBasic.balance[1] = inRangeInt(-48,0,sAudioBasic.balance[1]+lcode);
              setBalance('R', sAudioBasic.balance[1]);
            };break;
          }
        };break;
        
        case 1:{ // dc block
          sAudioBasic.dcBlock = inRangeFloat(3.5,100,sAudioBasic.dcBlock+lcode*0.5);
          setDCFilter(sAudioBasic.dcBlock);
        };break;
        
        case 2:{ // eq
          switch(paraSel)
          {
            case 0:{
              sAudioEQ.band[bandSel].gain = inRangeInt(-12,12,sAudioEQ.band[bandSel].gain+lcode);
            };break;
            case 1:{
              sAudioEQ.band[bandSel].shapeFactor = inRangeFloat(0.1,10,sAudioEQ.band[bandSel].shapeFactor+lcode*0.1);
            };break;
          }
          setGraphicEQ(sAudioEQ.band[bandSel]);
        };break;
        
        case 3:{ // tone
          switch(bandSel)
          {
            case 0:{
              sAudioTone.bassGain = inRangeInt(-12,12,sAudioTone.bassGain+lcode);
            };break;
            case 1:{
              sAudioTone.midGain = inRangeInt(-12,12,sAudioTone.midGain+lcode);
            };break;
            case 2:{
              sAudioTone.trebleGain = inRangeInt(-12,12,sAudioTone.trebleGain+lcode);
            };break;
          }
          setTone(sAudioTone, bandSel);
        };break;
        
        case 4:{ // wave gen
          
        };break;
        
        case 5:{ // filter
          
        };break;
        
        case 6:{ // ALE
          
        };break;
        
        case 7:{ // UltraBass
          sAudioKeyFunc.AUBGain = inRangeInt(0,24,sAudioKeyFunc.AUBGain+lcode);
          setUltraBassGain(sAudioKeyFunc.AUBGain);
        };break;
      }
      UI_Audio(index,bandSel,paraSel,false);
      lcode = 0;
    }
    
    if(keyValue[KEY_OK] != 0)
    {
      keyValue[KEY_OK] = 0;
      bandSel = inRangeLoop(0,bandNum[index]-1,bandSel,1);
      UI_Audio(index,bandSel,paraSel,false);
    }
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      break;
    }
    
    VolumeHandler();
  }
}


void MenuRadio(void)
{
  int8_t index = 0;
  uint8_t tmp;
  bool demo = sTuner.Config.bDemoMode;
  
  UI_Radio(index,true);
  
  while(1)
  {
    if(keyValue[KEY_UP] != 0)
    {
      keyValue[KEY_UP] = 0;
      if(index == 0)
        continue;
      index--;
      UI_Radio(index,false);
    }
    if(keyValue[KEY_DOWN] != 0)
    {
      keyValue[KEY_DOWN] = 0;
      if(index == MENU_RADIO_INDEX-1)
        continue;
      index++;
      UI_Radio(index,false);
    }
    
    // 确认键
    if(keyValue[KEY_OK] != 0)
    {
      switch(index)
      {
        case 0:{
        };break;
        case 1:{  // iPD
          if(demo)
          {
            SetFMiPhaseDiversity(!sTuner.Config.bFMiPD);
            if(sTuner.Config.bFMiPD == true)
              gpio_bit_set(GPIOC,GPIO_PIN_11);
            else if(sTuner.Config.nFMANTsel == 0 && sTuner.Config.bFMiPD == false)
              gpio_bit_reset(GPIOC,GPIO_PIN_11);
          }
        };break;
        case 2:{
        };break;
        case 3:{
        };break;
        case 4:{
          if(demo)
            SetFMStereoImprovement(!sTuner.Config.bFMSI);
        };break;
        case 5:{
          if(demo)
            SetFMChannelEqualizer(!sTuner.Config.bFMCEQ);
        };break;
        case 6:{
          SetFMiMultipathSuppression(!sTuner.Config.bFMiMS);
        };break;
        case 7:{
          SetFMClickNoiseSuppression(!sTuner.Config.bFMCNS);
        };break;
        case 8:{
        };break;
        case 9:{
        };break;
        case 10:{
        };break;
        case 11:{
        };break;
        default : index = 0;break;
      }
      UI_Radio(index,false);
      keyValue[KEY_OK] = 0;
    }
    
    // 左编码器旋转
    if(lcode != 0)
    {
      switch(index)
      {
        case 0:{
          sTuner.Config.nFMANTsel = inRangeInt(0,1,sTuner.Config.nFMANTsel+lcode);
          if(sTuner.Config.nFMANTsel == 1)
            gpio_bit_set(GPIOC,GPIO_PIN_11);
          else if(sTuner.Config.nFMANTsel == 0 && sTuner.Config.bFMiPD == false)
            gpio_bit_reset(GPIOC,GPIO_PIN_11);
          SetFMAntenna(sTuner.Config.nFMANTsel);
        };break;
        case 1:{
        };break;
        case 2:{
          sTuner.Config.nAMFixedLP = inRangeInt(0,3,sTuner.Config.nAMFixedLP+lcode);
          sTuner.Config.nAMFixedHP = sTuner.Config.nAMFixedLP;
          SetRadioSignal();
        };break;
        case 3:{
          sTuner.Config.nDeemphasis = inRangeInt(1,2,sTuner.Config.nDeemphasis+lcode);
          SetFMDeemphasis(sTuner.Config.nDeemphasis);
        };break;
        case 4:{
        };break;
        case 5:{
        };break;
        case 6:{
        };break;
        case 7:{
        };break;
        case 8:{  // noise blanker
          sTuner.Config.nNBSA[sTuner.Radio.nRFMode] = inRangeInt(0,3,sTuner.Config.nNBSA[sTuner.Radio.nRFMode]+lcode);
          SetNoiseBlanker(sTuner.Radio.nRFMode, sTuner.Config.nNBSA[sTuner.Radio.nRFMode], sTuner.Config.nNBSA[sTuner.Radio.nRFMode]);
        };break;
        case 9:{  // soft mute
          sTuner.Config.nSoftMute[sTuner.Radio.nRFMode] = inRangeInt(0,4,sTuner.Config.nSoftMute[sTuner.Radio.nRFMode]+lcode);
          SetSoftMute(sTuner.Config.nSoftMute[sTuner.Radio.nRFMode]);
        };break;
        case 10:{ // stereo blend
        };break;
        case 11:{ //dynamic cut
        };break;
        default : index = 0;break;
      }
      UI_Radio(index,false);
      lcode = 0;
    }
    
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      break;
    }
    
    VolumeHandler();
  }
  
  
}

void MenuSearch(void)
{
  int8_t index = 0;
  uint16_t tmp;
  
  UI_Search(index,true);
  
  while(1)
  {
    if(keyValue[KEY_UP] != 0)
    {
      keyValue[KEY_UP] = 0;
      if(index == 0)
        continue;
      index--;
      UI_Search(index,false);
    }
    if(keyValue[KEY_DOWN] != 0)
    {
      keyValue[KEY_DOWN] = 0;
      if(index == MENU_ATS_INDEX-1)
        continue;
      index++;
      UI_Search(index,false);
    }
    
    // 确认键
    if(keyValue[KEY_OK] != 0)
    {
      switch(index)
      {
        case 0:{
        };break;
        case 1:{
        };break;
        case 2:{
        };break;
        case 3:{
        };break;
        case 4:{  // manage
          
        };break;
        case 5:{  // start
          SearchAllCH();
        };break;
        default : index = 0;break;
      }
      UI_Search(index,false);
      keyValue[KEY_OK] = 0;
    }
    
    // 左编码器旋转
    if(lcode != 0)
    {
      switch(index)
      {
        case 0:{
          sTuner.ATS.nFMRegion = inRangeInt(0,3,sTuner.ATS.nFMRegion+lcode);
        };break;
        case 1:{
          sTuner.ATS.nATSMode = inRangeInt(0,1,sTuner.ATS.nATSMode+lcode);
        };break;
        case 2:{
          sTuner.ATS.nSigThreshold = inRangeInt(5,60,sTuner.ATS.nSigThreshold+lcode);
        };break;
        case 3:{
          sTuner.ATS.nMWStep = inRangeInt(9,10,sTuner.ATS.nMWStep+lcode);
          nBandStepATS[BAND_MW] = sTuner.ATS.nMWStep;
        };break;
        case 4:{
        };break;
        case 5:{
        };break;
        default : index = 0;break;
      }
      UI_Search(index,false);
      lcode = 0;
    }
    
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      break;
    }
    
    VolumeHandler();
  }
  
  
}

void MenuDevice(void)
{
  int8_t index = 0;
  uint16_t mcuInfo[2];
  uint8_t ver[19];
  //int strLen = 0;
  //char str[128];
  
  GetMCUInfo(sDevice.sInfo.uid,mcuInfo,mcuInfo+1);
  Get_REG(0xE2,ver,19);
  sDevice.sInfo.tid = ((uint32_t)ver[0]<<24) | ((uint32_t)ver[1]<<16) | ((uint32_t)ver[2]<<8) | ((uint32_t)ver[3]);
  
  
  //strLen = npf_snprintf(str, 128, "/**************************************************************/\n\n");
  
  printf("/**************************************************************/\n\n");
  
  printf("MCU Info: GD32F303RCT6\n");
  printf("UniqueID: %x%x%x\n",sDevice.sInfo.uid[0],sDevice.sInfo.uid[1],sDevice.sInfo.uid[2]);
  printf("Flash: %d KByte  SRAM: %d KByte\n",mcuInfo[0],mcuInfo[1]);
  
  printf("Firmware Info:\n");
  printf("Version: %d.%d        Date: %d\n",sDevice.sInfo.pid[1],sDevice.sInfo.pid[2],sDevice.sInfo.pid[3]);
  
  printf("Tuner Info: SAF775X Dirana3\n");
  printf("ARM ID: %d.%d(Build:%d)\n",ver[0],ver[1],((uint16_t)ver[2]<<8)+ver[3]);
  printf("Radio DSP0 ID: %d%d.%d%d%d\n",(ver[4])&0x0F,(ver[5]>>4)&0x0F,(ver[5])&0x0F,(ver[6]>>4)&0x0F,(ver[6])&0x0F);
  printf("Radio DSP1 ID: %d%d.%d%d%d\n",(ver[7])&0x0F,(ver[8]>>4)&0x0F,(ver[8])&0x0F,(ver[9]>>4)&0x0F,(ver[9])&0x0F);
  printf("Radio DSP2 ID: %d%d.%d%d%d\n",(ver[10])&0x0F,(ver[11]>>4)&0x0F,(ver[11])&0x0F,(ver[12]>>4)&0x0F,(ver[12])&0x0F);
  printf("Audio DSP0 ID: %d%d.%d\n",(ver[14]>>4)&0x0F,(ver[14])&0x0F,(ver[15]>>4)&0x0F);
  printf("Audio DSP1 ID: %d%d.%d\n",(ver[17]>>4)&0x0F,(ver[17])&0x0F,(ver[18]>>4)&0x0F);
  
  Get_REG(0xE6,ver,15);
  printf("Radio DSP0 Patch Status: %d    Version: %d (DSP ID=%d, APP ID=%d)\n",ver[0],(ver[1]>>4)&0x0F,(ver[1])&0x0F,ver[2]);
  printf("Radio DSP1 Patch Status: %d    Version: %d (DSP ID=%d, APP ID=%d)\n",ver[3],(ver[4]>>4)&0x0F,(ver[4])&0x0F,ver[5]);
  printf("Radio DSP2 Patch Status: %d    Version: %d (DSP ID=%d, APP ID=%d)\n",ver[6],(ver[7]>>4)&0x0F,(ver[7])&0x0F,ver[8]);
  printf("Audio DSP0 Patch Status: %d    Version: %d (DSP ID=%d, APP ID=%d)\n",ver[9],(ver[10]>>4)&0x0F,(ver[10])&0x0F,ver[11]);
  printf("Audio DSP1 Patch Status: %d    Version: %d (DSP ID=%d, APP ID=%d)\n",ver[12],(ver[13]>>4)&0x0F,(ver[13])&0x0F,ver[14]);
  
  printf("\n/**************************************************************/");
  
  UI_Device(index,true);
  
  while(1)
  {
    if(keyValue[KEY_UP] != 0)
    {
      keyValue[KEY_UP] = 0;
      if(index == 0)
        continue;
      index--;
      UI_Device(index,false);
    }
    if(keyValue[KEY_DOWN] != 0)
    {
      keyValue[KEY_DOWN] = 0;
      if(index == MENU_DEVICE_INDEX-1)
        continue;
      index++;
      UI_Device(index,false);
    }
    
    // 左编码器旋转
    if(keyValue[KEY_OK] != 0)
    {
      switch(index)
      {
        case 0:{
          sDevice.bAutoMono = !sDevice.bAutoMono;
          //set mono
        };break;
        case 1:{
          sTuner.Config.bDemoMode = !sTuner.Config.bDemoMode;
          // apply in next start
        };break;
        case 2:{
          sDevice.bSoftReboot = !sDevice.bSoftReboot;
        };break;
        case 3:{
          // enter time set
        };break;
        case 4:{
          // enter firmware update
          keyValue[KEY_OK] = 0;
          GUI_Text(8,24,248,96,"Hold OK to confirm",&Font24,COLOR_BLACK,COLOR_WHITE);
          // show confirm
          while(1)
          {
            if(keyValue[KEY_OK] == 2) // long press
            {
              gpio_bit_set(GPIOB,GPIO_PIN_8);
              delay_ms(1000);
              delay_ms(1000);
              delay_ms(1000);
              delay_ms(1000);
              delay_ms(1000);
              __disable_irq();
              __set_FAULTMASK(1);
              NVIC_SystemReset();
            }
            
            if(keyValue[KEY_MENU] != 0)
            {
              flushKey();
              break;
            }
          }
          UI_Device(index,true);
        };break;
        default : index = 0;break;
      }
      UI_Device(index,false);
      keyValue[KEY_OK] = 0;
    }
    
    if(lcode != 0)
    {
      flushKey();
    }
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      break;
    }
    
    VolumeHandler();
  }
  
  
}

void MenuAbout(void)
{
  UI_About(0,true);
  
  while(1)
  {
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      break;
    }
    
    VolumeHandler();
  }
}

void MenuMain(void)
{
  int8_t index = 0;
  
  UI_Menu(index,true);
  
  while(1)
  {
    
    if(keyValue[KEY_UP] != 0)
    {
      keyValue[KEY_UP] = 0;
      if(index == 0)
        continue;
      index--;
      UI_Menu(index,false);
    }
    if(keyValue[KEY_DOWN] != 0)
    {
      keyValue[KEY_DOWN] = 0;
      if(index == MENU_MAIN_INDEX-1)
        continue;
      index++;
      UI_Menu(index,false);
    }
    if(keyValue[KEY_OK] != 0)
    {
      flushKey();
      switch(index)
      {
        case 0 :MenuDisplay(),saveSettings(CFG_DISP);break;
        case 1 :MenuAudio(),saveSettings(CFG_AUDIO);break;
        case 2 :MenuRadio(),saveSettings(CFG_RADIO);break;//Radio
        case 3 :MenuSearch(),saveSettings(CFG_RADIO);break;//ATS
        case 4 :MenuDevice(),saveSettings(CFG_DEVICE);break;
        case 5 :MenuAbout();break;
        default : index = 0;break;
      }
      UI_Menu(index,true);
    }
    
    if(keyValue[KEY_MENU] != 0)
    {
      flushKey();
      //EEPROM_Save_Setting();    //SYNC TO EEPROM
      UI_Main(true);
      break;
    }
    
    VolumeHandler();
  }
  
}




/*                                                                            */

// main func

int main(void)
{
  uint32_t verCheck[4];
  
  SYS_Init();
  GPIO_Init();
  
  gpio_bit_set(GPIOC, GPIO_PIN_6); // PC6 PWR_CTR -> 3V3/1V2
  gpio_bit_set(GPIOC, GPIO_PIN_8); // PC8 PWR_LCD
  delay_ms(200);
  
  Timer_Init();
  if(bkp_read_data(BKP_DATA_0) != 0xA5A5) {
    /* backup data register value is not correct or not yet programmed
    (when the first time the program is executed) */
    RTC_Init();
    bkp_write_data(BKP_DATA_0, 0xA5A5);
  } else {
    /* allow access to BKP domain */
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    
    rtc_register_sync_wait();
    rtc_interrupt_enable(RTC_INT_SECOND);
    rtc_set_alarm(170*60);
  }
  
  ADC_Init();
  DAC_Init();
  USART_Init();
  flash_init();
  delay_ms(10);
  
  //fdb_kv_set_default(&kvdb);
  
  fdb_err_t result;
  int discardDB = 0;
  struct fdb_default_kv default_kv;
  
  default_kv.kvs = default_kv_table;
  default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
  result = fdb_kvdb_init(&kvdb, "koishi", "RadioSet", &default_kv, NULL);
  
  if (result != FDB_NO_ERR) {
    // Database init fail !!! Discard Database, Use default settings
    discardDB = 1;
  }
  
  
  
  // Read Configs from database
  struct fdb_blob blob;
  if(!discardDB) {
    /* get the "boot_count" KV value */
    fdb_kv_get_blob(&kvdb, "bootCount", fdb_blob_make(&blob, &bootCount, sizeof(bootCount)));
    bootCount++;
    fdb_kv_set_blob(&kvdb, "bootCount", fdb_blob_make(&blob, &bootCount, sizeof(bootCount)));
    FDB_INFO("Boot count: %d -> %d\n", bootCount-1, bootCount);
    
    int needUpdate = 0;
    struct device checkSetting = {0};
    fdb_kv_get_blob(&kvdb, "version", fdb_blob_make(&blob, &checkSetting, sizeof(checkSetting)));
    if(blob.saved.len > 0) {
      if(checkSetting.sInfo.pid[0] != sDevice.sInfo.pid[0])
        needUpdate = 1;
    } else {
      needUpdate = 1;
    }
    
    if(needUpdate) { // Write Configs
      LCD_StructInit(true);
      TunerStructInit(&sTuner, true);
      initBasicControl(&sAudioBasic, true);
      initTone(&sAudioTone, true);
      initGraphicEQ(&sAudioEQ, true);
      initKeyFuncPara(&sAudioKeyFunc);
      sDevice.sTime.year = 2024;
      sDevice.sTime.month = 7;
      sDevice.sTime.day = 23;
      sDevice.sTime.hour = 0;
      sDevice.sTime.minute = 0;
      sDevice.sTime.second = 0;
      sDevice.sTime.timestamp = 0;
      saveSettings(CFG_ALL);
    } else { // Read Configs
      readSettings(CFG_ALL);
      readChannels(BAND_FM);
      readChannels(BAND_LW);
      readChannels(BAND_MW);
      readChannels(BAND_SW);
    }
  } else {
    LCD_StructInit(true);
    TunerStructInit(&sTuner, true);
    initBasicControl(&sAudioBasic, true);
    initTone(&sAudioTone, true);
    initGraphicEQ(&sAudioEQ, true);
    initKeyFuncPara(&sAudioKeyFunc);
    sDevice.sTime.year = 2024;
    sDevice.sTime.month = 7;
    sDevice.sTime.day = 23;
    sDevice.sTime.hour = 0;
    sDevice.sTime.minute = 0;
    sDevice.sTime.second = 0;
    sDevice.sTime.timestamp = 0;
  }
  readBasicPara();
  
  delay_ms(5);
  
  // lcd config
  lcd_dma_init();
  LCD_StructInit(false);
  LCD_Init();
  
  if(sDisplay.backTime == 0)
    DAC_OutVal(BLSIG_CH, sDisplay.brightness);
  
  I2C_Init();
  TunerStructInit(&sTuner, false);
  TunerInit();
  
  // special func
  SetFMStereoImprovement(sTuner.Config.bFMSI);
  if(sTuner.Config.bFMiPD == true || sTuner.Config.nFMANTsel == 1)
    gpio_bit_set(GPIOC,GPIO_PIN_11);
  
  // audio config
  initBasicControl(&sAudioBasic, false);
  setDCFilter(sAudioBasic.dcBlock);
  setMainVol(sAudioBasic.mainVol);
  setBalance('L', sAudioBasic.balance[0]);
  setBalance('R', sAudioBasic.balance[1]);
  sTuner.Audio.index = 0;
  SetVolume(sTuner.Audio.nVolume[sTuner.Audio.index]);
  sTuner.Audio.index = 1;
  SetVolume(sTuner.Audio.nVolume[sTuner.Audio.index]);
  
  initSignalScaler();
  
  initTone(&sAudioTone, false);
  initGraphicEQ(&sAudioEQ, false);
  
  if(sTuner.Config.bDemoMode == true)
  {
    sAudioKeyFunc.OnOffAUB = true;
    initUltraBass(sAudioKeyFunc.OnOffAUB);
    setUltraBassGain(sAudioKeyFunc.AUBGain);
  }
  
  // Visual Effects
  struct graphicSA sAudioGSA;
  initGraphicSApara(&sAudioGSA);
  initGraphicSA(sAudioGSA);
  
  struct quasiPD sAudioQPD;
  initQPDpara(&sAudioQPD);
  initQPD(sAudioQPD);
  
  RDS_Init(&sRDSData);
  
  nowDET = gpio_input_bit_get(GPIOA,GPIO_PIN_7);
  if(nowDET == 1)  // headphone plugin
  {
    sTuner.Audio.index = 0;
    gpio_bit_reset(GPIOC, GPIO_PIN_4);
    gpio_bit_set(GPIOA, GPIO_PIN_6);
  }
  else
  {
    sTuner.Audio.index = 1;
    gpio_bit_reset(GPIOA, GPIO_PIN_6);
    gpio_bit_set(GPIOC, GPIO_PIN_4);
  }
  lastDET = nowDET;
  
  setMute(ADSP_MUTE_MAIN, 0);
  
  UI_Main(true);
  
  timer_enable(TIMER5);
  timer_enable(TIMER6);
  
  while(1)
  {
    
    // 左编码器旋转
    if(lcode != 0)
    {
      if(bFilterSel == true)
        AdjustFilter(lcode);
      else
        AdjustFreq(lcode);
      lcode = 0;
    }
    
    // 目录键按下
    if(keyValue[KEY_MENU] != 0)
    {
      if(keyValue[KEY_MENU] == KEY_PRESS)
      {
        flushKey();
        MenuMain();
      }
      else
      {
        // shut down
      }
      keyValue[KEY_MENU] = 0;
    }
    
    // 上键按下
    if(keyValue[KEY_UP] != 0)
    {
      if(keyValue[KEY_UP] == KEY_PRESS)
      {
        GoChannel(1);
      }
      else
      {
        SearchCH(1);
      }
      keyValue[KEY_UP] = 0;
    }
    
    // 下键按下
    if(keyValue[KEY_DOWN] != 0)
    {
      if(keyValue[KEY_DOWN] == KEY_PRESS)
      {
        GoChannel(-1);
      }
      else
      {
        SearchCH(-1);
      }
      keyValue[KEY_DOWN] = 0;
    }
    
    // 确认键按下
    if(keyValue[KEY_OK] != 0)
    {
      if(keyValue[KEY_OK] == KEY_PRESS)
      {
        bFilterSel = !bFilterSel;
      }
      else
      {
        AddChannel();
      }
      keyValue[KEY_OK] = 0;
    }
    
    // 左编码器按下
    if(keyValue[KEY_LENC] != 0)
    {
      if(keyValue[KEY_LENC] == KEY_PRESS)
      {
        sTuner.Radio.nFreqStep[sTuner.Radio.nBandMode]++;
        if(sTuner.Radio.nFreqStep[sTuner.Radio.nBandMode] == nBandStepNum[sTuner.Radio.nBandMode])
          sTuner.Radio.nFreqStep[sTuner.Radio.nBandMode] = 0;
      }
      else
      {
        if(sTuner.Radio.nBandMode == BAND_SW)
          SwitchBand(BAND_FM);
        else
          SwitchBand(sTuner.Radio.nBandMode+1);
      }
      keyValue[KEY_LENC] = 0;
    }
    
    VolumeHandler();
    
    /***************************/
    
    if(bFlagGSA == true && !sDisplay.emiFree)
    {
      bFlagGSA = 0;
      Get_REG(0x1A, (uint8_t *)nGsaVal+1, 9);
      nGsaVal[0] = 1;
      nQPeakDet = getQPDAverPeak();
    }
    if(bFlagRFS == true)
    {
      bFlagRFS = 0;
      GetStatus();
    }
    if(bFlagRDS == true)
    {
      bFlagRDS = 0;
      GetRDS(&sRDSBuffer);
    }
    if(bFlagSync == true) {
      bFlagSync = 0;
      saveBasicPara();
    }
    if(bFlagReBoot == true)
    {
      bFlagReBoot = false;
      if(sDevice.bSoftReboot == true)
      {
        for(int8_t i = sAudioBasic.mainVol; i > -66; i--)
        {
          setMainVol(i);
          delay_ms(20);
        }
        setMute(ADSP_MUTE_MAIN, 1);
        delay_ms(10);
        gpio_bit_reset(GPIOC, GPIO_PIN_4);
        gpio_bit_reset(GPIOA, GPIO_PIN_6);
      }
      
      TunerInit();
      SetFMStereoImprovement(sTuner.Config.bFMSI);
      
      setDCFilter(sAudioBasic.dcBlock);
      setMainVol(sAudioBasic.mainVol);
      setBalance('L', sAudioBasic.balance[0]);
      setBalance('R', sAudioBasic.balance[1]);
      
      sTuner.Audio.index = 0;
      SetVolume(sTuner.Audio.nVolume[sTuner.Audio.index]);
      sTuner.Audio.index = 1;
      SetVolume(sTuner.Audio.nVolume[sTuner.Audio.index]);
      nowDET = gpio_input_bit_get(GPIOA,GPIO_PIN_7);
      if(nowDET == 1)  // headphone plugin
      {
        sTuner.Audio.index = 0;
        gpio_bit_reset(GPIOC, GPIO_PIN_4);
        gpio_bit_set(GPIOA, GPIO_PIN_6);
      }
      else
      {
        sTuner.Audio.index = 1;
        gpio_bit_reset(GPIOA, GPIO_PIN_6);
        gpio_bit_set(GPIOC, GPIO_PIN_4);
      }
      lastDET = nowDET;

      initSignalScaler();

      initTone(&sAudioTone, false);
      initGraphicEQ(&sAudioEQ, false);
      
      if(sTuner.Config.bDemoMode == true)
      {
        sAudioKeyFunc.OnOffAUB = true;
        initUltraBass(sAudioKeyFunc.OnOffAUB);
        setUltraBassGain(sAudioKeyFunc.AUBGain);
      }
      
      initGraphicSApara(&sAudioGSA);
      initGraphicSA(sAudioGSA);

      initQPDpara(&sAudioQPD);
      initQPD(sAudioQPD);
      
      SetMute(sTuner.Audio.bMuted);
      
      RDS_Init(&sRDSData);
      
    }
    
    UI_Main(false);
  }
}

// 1mS*INT
#define TIM_INT_RDS    80
#define TIM_INT_KEY    10
#define TIM_INT_LCD    55
#define TIM_INT_GSA    80
// 100mS*INT
#define TIM_INT_RFS    5
#define TIM_INT_ONE    10
#define TIM_INT_BAT    48

void TIM_Callback(uint8_t tim)
{
  static uint16_t timer_RDS = TIM_INT_RDS;
  static uint16_t timer_KEY = TIM_INT_KEY;
  static uint16_t timer_LCD = TIM_INT_LCD;
  static uint16_t timer_GSA = TIM_INT_GSA-10;
  static uint16_t timer_RFS = TIM_INT_RFS;
  static uint16_t timer_ONE = TIM_INT_ONE;
  static uint16_t timer_BAT = TIM_INT_BAT;
  
  if(tim == 5)  // 1khz
  {
    /*********************TICK*********************/
    
    sDevice.sTime.timestamp++;
    
    /**********************ENC*********************/
    
    nowA1 = gpio_input_bit_get(GPIOB,GPIO_PIN_0);
    if(nowA1 > lastA1)
    {
      if(nowA1 == gpio_input_bit_get(GPIOB,GPIO_PIN_1))
        lcode = -1;
      else
        lcode = 1;
      reFillBackLightTimer();
    }
    lastA1 = nowA1;
    
    nowA2 = gpio_input_bit_get(GPIOB,GPIO_PIN_10);
    if(nowA2 > lastA2)
    {
      if(nowA2 == gpio_input_bit_get(GPIOB,GPIO_PIN_11))
        rcode = -1;
      else
        rcode = 1;
      reFillBackLightTimer();
    }
    lastA2 = nowA2;
    
    /*********************SW TIM*******************/
    
    timer_KEY--;
    if(timer_KEY == 0)
    {
      timer_KEY = TIM_INT_KEY;
      adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
      
      nowDET = gpio_input_bit_get(GPIOA,GPIO_PIN_7);
      if(nowDET != lastDET)
      {
        if(nowDET == 1)  // headphone plugin
        { 
          sTuner.Audio.index = 0;
          gpio_bit_reset(GPIOC, GPIO_PIN_4);
          gpio_bit_set(GPIOA, GPIO_PIN_6);
        }
        else
        {
          sTuner.Audio.index = 1;
          gpio_bit_reset(GPIOA, GPIO_PIN_6);
          gpio_bit_set(GPIOC, GPIO_PIN_4);
        }
        lastDET = nowDET;
      }
    }
    
    timer_LCD--;
    if(timer_LCD == 5 && !sDisplay.emiFree)
    {
      bFlagGSA = 1;
    }
    if(timer_LCD == 0)
    {
      timer_LCD = TIM_INT_LCD;
      if(bMenuLCD == false)
        lcd_update();
    }
    
    if(sTuner.Status.nRSSI > 20 && sTuner.Radio.nBandMode == BAND_FM)
      timer_RDS--;
    if(timer_RDS == 0)
    {
      timer_RDS = TIM_INT_RDS;
      bFlagRDS = 1;
    }
    
//    timer_GSA--;
//    if(timer_GSA == 0)
//    {
//      timer_GSA = TIM_INT_GSA;
//      bFlagGSA = 1;
//    }
    
    timer_interrupt_flag_clear(TIMER5, TIMER_INT_FLAG_UP);
  }
  else if(tim == 6)  //10hz
  {
    
    if(sDisplay.backTimeCounter != 0) {
      if(DAC_GetValue(BLSIG_CH) == 0 && sDisplay.brightness != 0)
        DAC_OutVal(BLSIG_CH, sDisplay.brightness);
      sDisplay.backTimeCounter--;
      if(sDisplay.backTimeCounter == 0)
        DAC_OutVal(BLSIG_CH, 0);
    }
    
    timer_RFS--;
    if(timer_RFS == 0) {
      timer_RFS = TIM_INT_RFS;
      bFlagRFS = 1;
    }
    
    timer_ONE--;
    if(timer_ONE == 0) {
      timer_ONE = TIM_INT_ONE;
      
    }
    
    timer_BAT--;
    if(timer_BAT == 0) {
      if(gpio_input_bit_get(GPIOC, GPIO_PIN_3) == RESET) {
        timer_BAT = 2;
        gpio_bit_set(GPIOC, GPIO_PIN_3);
      } else {
        timer_BAT = TIM_INT_BAT;
        adc_software_trigger_enable(ADC0, ADC_INSERTED_CHANNEL);
      }
    }
    
    if(nSyncTimer > 0) {
      nSyncTimer--;
      if(nSyncTimer <= 0) {
        bFlagSync = true;
      }
    }
    
    
    timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
  }
  
}
void KEY_Handler(uint8_t key, uint8_t state)
{
  static uint8_t key_count[KEY_NUM];
  static uint8_t key_state[KEY_NUM];
  
  switch(key_state[key])
  {
    case 0:{  // release
      if(key_count[key] >= 2)
      {
        key_count[key] = 0;
        keyValue[key] = 1;
      }
      if(state == 0)
        key_state[key] = 1;
    };break;
    
    case 1:{  // confirm
      // refresh back light timer
      reFillBackLightTimer();
      
      if(state == 0)
        key_count[key]++;
      else
        key_state[key] = 0;
      
      if(key_count[key] >= 100)
      {
        key_state[key] = 2;
        key_count[key] = 0;
        keyValue[key] = 2;
      }
    };break;
    
    case 2:{  // long press
      if(state == 1)
        key_state[key] = 0;
    };break;
    
    default :break;
  }
  
}

void ADC_Callback(uint8_t adc, char group)
{
  uint16_t valRegular = 0;
  uint16_t valInsert = 0;
  
  if(adc == 0)
  {
    if(group == 'R')
    {
      valRegular = adc_regular_data_read(ADC0);
      if(valRegular < 3448 && valRegular > 3039)  //0.782, 3202
      {
        // KEY 1
        KEY_Handler(KEY_MENU, 0);
      }
      else if(valRegular < 2772 && valRegular > 2363)  //0.619, 2535
      {
        // KEY 2
        KEY_Handler(KEY_UP, 0);
      }
      else if(valRegular < 1863 && valRegular > 1454)  //0.386, 1581
      {
        // KEY 3
        KEY_Handler(KEY_DOWN, 0);
      }
      else if(valRegular < 921 && valRegular > 512)  //0.164, 672
      {
        // KEY 4
        KEY_Handler(KEY_OK, 0);
      }
      else if(valRegular < 205)  //0, 0
      {
        // ENC2 KEY
        KEY_Handler(KEY_RENC, 0);
      }
      else if(valRegular > 3890)
      {
        // Release
        KEY_Handler(KEY_MENU, 1);
        KEY_Handler(KEY_UP, 1);
        KEY_Handler(KEY_DOWN, 1);
        KEY_Handler(KEY_OK, 1);
        KEY_Handler(KEY_RENC, 1);
      }
      
      KEY_Handler(KEY_LENC, gpio_input_bit_get(GPIOA, GPIO_PIN_0));
      
      adc_interrupt_flag_clear(ADC0,ADC_INT_FLAG_EOC);
    }
    else if(group == 'I')
    {
      valInsert = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
      sDevice.nBatVolt = (float)valInsert*4389.0/40960.0;
      gpio_bit_reset(GPIOC, GPIO_PIN_3);
      
      valInsert = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_1);
      sDevice.nIntTemp = myround( (1.45-(float)valInsert*3.3/4096.0)/(0.0041) + 25 );
      adc_interrupt_flag_clear(ADC0,ADC_INT_FLAG_EOIC);
    }
  }
}

void RTC_Callback(uint8_t line)
{
  if(line == 0)  // second
  {
    sDevice.sTime.second++;
    if(sDevice.sTime.second == 60)
    {
      sDevice.sTime.second = 0;
      sDevice.sTime.minute++;
    }
    if(sDevice.sTime.minute == 60)
    {
      sDevice.sTime.minute = 0;
      sDevice.sTime.hour++;
    }
    if(sDevice.sTime.hour == 24)
    {
      sDevice.sTime.hour = 0;
      sDevice.sTime.day++;
    }
  }
  if(line == 1)  // alarm
  {
    if(sTuner.Config.bDemoMode == true)
      bFlagReBoot = true;
  }
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
