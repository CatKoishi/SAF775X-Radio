#ifndef __SAF775X_H
#define __SAF775X_H

#include "stdint.h"
#include "main.h"

#define DSP_I2C_ADDR      0x38


// Band RF Mode
#define RFMODE_FM       0	//FM
#define RFMODE_AM       1	//AM

// Volume
#define MIN_VOL         0
#define MID_VOL         20
#define MAX_VOL         100

// Band Mode
#define NUM_BANDS       4
#define BAND_FM         0
#define BAND_LW         1
#define BAND_MW         2
#define BAND_SW         3

#define BAND_WX         4
#define BAND_DAB3       5
#define BAND_DABL       6


// Step
#define NUM_STEPS       4

// Filters
#define NUM_FILTERS  16  // Auto + 15 fixed bandwidth filters

// Channels in band
#define CHS_LW       20
#define CHS_MW       50
#define CHS_SW      100
#define CHS_FM      100

// Tuning Mode
#define Preset        1
#define Search        2
#define AF_Update     3
#define Jump          4
#define Check         5
#define End           7

// Read Mode
#define Read_Status        0
#define Read_Data          1


struct tunerRadio
{
	uint8_t nBandMode;	//FM, LW, MW, SW
	uint8_t nRFMode;    //FM, AM
	uint16_t nBandFreq[NUM_BANDS];
	uint8_t nFreqStep[NUM_BANDS];  //当前不同波段的步进序号
};

struct tunerStatus
{
	int8_t nRSSI;     // 接收信号强度指示,     [-8, 99]dBuv
	uint8_t nUSN;		  // 超声噪声检测,      FM:[0, 255]
	int8_t nACD;      // 邻信道检测,        AM:[-60, 60]dB
	uint8_t nWAM;			// FM噪声比例,        FM:[0, 255]
	int8_t nOffset;		// 与信号频率的频偏值 FM:[-127, 127]KHz    AM:[-12.7, 12.7]KHz
	int8_t nSNR;      // 信噪比, in dB
	bool bSTIN;       // 立体声指示
	uint8_t nQRS;			// 调谐信息指示
	bool bAFOK;
};

struct tunerAudio
{
	uint8_t nVolume[2];        // 音量控制, 0-31
	uint8_t index;  // 0->HP, 1->SPK
	bool bMuted;    // 静音指示
};

struct tunerSetting
{
	uint8_t bDemoMode;
	// Filters
	uint8_t nBandFilter[2];   // Current FIR filter index
	// AGC
	uint8_t nBandAGC[2];
	
	// Change in standby/idle state
	bool bFMAGCext;    // FM extern AGC 0=internal AGC only, 1=use int&ext AGC
	bool bAMANTtyp;    // AM Antenna select 0=HiZ ANT, 1=LoZ ANT
	
	uint8_t nFMANTsel;    // FM ANT selection ?=FM ANT?[0,1]
	bool bFMiPD;    // FM Improved phase diversity 0=off, 1=on
	bool bFMCEQ;    // FM channel equalizer, 0=off, 1=on
	bool bFMiMS;    // FM enhanced multipath suppression, 0=off, 1=on
	bool bFMCNS;    // FM click noise suppression 0=off, 1=on
	bool bFMSI;     // FM stereo improvement 0=off, 1=on
	
	uint8_t nNBSA[2];  // IF Noise blanker
	uint8_t nNBSB;  // Audio Noise blanker
	
	
	// FM立体声 0->单声道 [1~9]->立体声
	uint8_t nFMST;
	// FM去加重时间常数  1=50us(default), 2=75us
	uint8_t nDeemphasis;
	
	uint8_t nAMFixedLP;
	uint8_t nAMFixedHP;
	
	uint8_t nSoftMute[2];
	
	uint8_t nDynamicCut[2];
	
};

struct tunerATS
{
	// ATS搜索模式 0->慢速 1->快速 2->先进
	uint8_t nATSMode;
	// 小信号阈值
	uint8_t nSigThreshold;
	// FM ATS频率低值
	uint8_t nFMRegion;
	// 线路增益 [-48,+15]dB
	int8_t nMWStep;
};

struct Dirana3Radio
{
	struct tunerAudio Audio;
	struct tunerStatus Status;
	struct tunerATS ATS;
	struct tunerRadio Radio;
	struct tunerSetting Config;
};



extern const uint8_t nBandRFMode[NUM_BANDS];

extern uint16_t nBandStep[NUM_BANDS][NUM_STEPS];
extern const uint8_t nBandStepNum[NUM_BANDS];
extern uint8_t nBandStepATS[NUM_BANDS];
extern const uint16_t nFastATS[14][2];
extern const uint16_t nFMATSRegion[4][2];

extern const int16_t nBandFMin[NUM_BANDS];
extern const int16_t nBandFMax[NUM_BANDS];



void Set_REG(uint8_t addr, uint8_t* data, uint8_t size);
void Get_REG(uint8_t addr, uint8_t* data, uint8_t size);
void Set_REGFree(int len, ...);

void Set_ADSP(uint32_t subaddr, uint32_t data);
void Set_ADSP_multi(uint32_t subaddr, uint32_t *data, uint8_t size);
uint32_t Get_ADSP(uint32_t subaddr);


void GetStatus(void);
void GetQRS(void);
void GetRDS(struct RDSBuffer* rds);

void SetVolume(uint8_t percent);
void SetMute(bool mute);
void TuneFreq(uint16_t freq, uint8_t mode);

void SetTuner(void);
void SetFilter(uint8_t index);
void SetAGC(uint8_t index);

void SetTunerOPT(void);
void SetExtAGC(bool on);
void SetFMAntenna(uint8_t ant);
void SetFMiPhaseDiversity(bool on);
void SetAMAntenna(uint8_t ant_type);

void SetRadioDSP(void);
void SetFMiMultipathSuppression(bool on);
void SetFMClickNoiseSuppression(bool on);
void SetFMChannelEqualizer(bool on);
void SetNoiseBlanker(uint8_t rfmode, uint8_t sensIF, uint8_t sensAu);

void SetRadioSignal(void);
void SetFMStereoImprovement(bool on);
void SetFMDeemphasis(uint8_t tao);
void SetAMFixedHP(uint8_t hp);
void SetAMFixedLP(uint8_t lp);

void SetSoftMute(uint8_t level);


void SwitchBand(uint8_t band);

void TunerStructInit(struct Dirana3Radio* init, bool initPara);
void TunerInit(void);



#endif
