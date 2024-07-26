/**
  ************************************* Copyright ******************************   
  *                 (C) Copyright 2023,NyaKoishi,China,GCU.
  *                            All Rights Reserved
  *
  * @file      : Dirana3BasicDSP.h
  * @author    : NyaKoishi
  * @version   : V1.0.0
  * @date      : 2023-05-21
  * @brief     : Header for Dirana3BasicDSP.h file
  * @attention : 
  * 
  ******************************************************************************
 */



#ifndef __DIRANA3_BASIC_D_S_P_H_
#define __DIRANA3_BASIC_D_S_P_H_

#define ADSP_E  2.718281828
#define ADSP_FS 44100.0
#define ADSP_PI PI
//#define ADSP_PI 3.141592654
#define ADSP_SQRT2   1.414213562

#define ADSP_Channel_Primary       0x20
#define ADSP_Channel_Secondary1    0x28
#define ADSP_Channel_Secondary2    0x40
#define ADSP_Source_RadioP         0x00
#define ADSP_Source_RadioS         0x01
#define ADSP_Source_ADC01          0x08
#define ADSP_Source_ADC23          0x09
#define ADSP_Source_NoiseGen       0x1E
#define ADSP_Source_SinGen         0x1F

#define ADSP_Peripheral_ADC        0x28
#define ADSP_Peripheral_DAC01      0x32
#define ADSP_Peripheral_DAC23      0x33
#define ADSP_Peripheral_DAC45      0x34
#define ADSP_Peripheral_PowerON    0
#define ADSP_Peripheral_PowerOFF   1

#define ADSP_DACMode_LowNoise      0
#define ADSP_DACMode_HighGain      1

#define ADSP_MUTE_MAIN    0
#define ADSP_MUTE_FRONT   1
#define ADSP_MUTE_REAR    2


#define ADSP_MainVolMax    12.04
#define ADSP_MaxLoudBoost  19.0
#define ADSP_FixedBoost    48.16

#define ADSP_ToneMaxBoost    12.0
#define ADSP_GeqMaxBoost     12.0



#define ADSP_NoiseType_White 0
#define ADSP_NoiseType_Pink  1

struct basicControl
{
	float mainVol;
	float fader[2];
	float balance[2];
	float dcBlock;
};

struct toneControl
{
	uint16_t bassFc;
	float bassGain;
	
	uint16_t midFc;
	float midQ;
	float midGain;
	
	uint16_t trebleFc;
	float trebleGain;
};

//     50 100 200 400 800 1600 3200 6400 12800
// fc : 47 94 188 375 750 1500 3000 6000 12000
// gain -12~12dB
// shapeFactor 0.1~10
struct graphicEQband
{
	uint8_t band;
	uint16_t fc;
	float gain;
	float shapeFactor;
};

struct graphicEQ
{
	struct graphicEQband band[9];
};

struct graphicSA
{
	uint16_t fc[9];
	float q[9];
	float inputGain;
	float bandGain[9];
	float threshold[8];
	float ta;
	float tr;
	uint32_t audioSource;
};

struct quasiPD
{
	uint32_t InPntr1;
	uint32_t InPntr2;
	float timeAttack;    // 0.0001 ~ 2.0s  [0.035]
	float timeRelease;   // 0.0001 ~ 20.0s [1.5]
	float timeHold;      // 0 ~ 10s        [0]
};

struct filterCoeff
{
	float b0;
	float b1;
	float b2;
	float a1;
	float a2;
};

struct gpFilterCom
{
	uint8_t filterType;
	uint32_t InPntr;
	struct filterCoeff filter;
};

struct gpFilterSep
{
	uint8_t filterTypeL;
	uint8_t filterTypeR;
	uint32_t InPntrL;
	uint32_t InPntrR;
	struct filterCoeff filterL;
	struct filterCoeff filterR;
};

struct gpFilterCross
{
	uint8_t filterType1;
	uint8_t filterType2;
	uint32_t InPntr;
	struct filterCoeff filter1;
	struct filterCoeff filter2;
};

struct filterSystem
{
	struct gpFilterSep GPF1;
	struct gpFilterCom GPF2;
	struct gpFilterSep GPF3;
	struct gpFilterCom GPF4;
	struct gpFilterCom GPF5;
	struct gpFilterCross GPF6;
	struct gpFilterCross GPF7;
};

#define ADSP_Filter_Flat     0
#define ADSP_Filter_1stLP    1
#define ADSP_Filter_1stHP    2
#define ADSP_Filter_2ndLP    3
#define ADSP_Filter_2ndHP    4
#define ADSP_Filter_ShvLP    5
#define ADSP_Filter_ShvHP    6
#define ADSP_Filter_Peak     7



struct keyFunc
{
	// AUB
	bool OnOffAUB;
	float AUBGain;
	
	// ALE
	bool OnOffALE;
	
	bool OnOffLEV;
	float LEVSmoothTime;
	float LEVReleaseTime;
	float LEVExpanderThres;
	
	bool OnOffAEQ;
	float AEQReferSpectrum[3];
	float AEQSpectrumOffset[3];
};



void initBasicControl(struct basicControl* init, bool initPara);
void setMainVol(float VoldB);
int8_t setBalance(char channel, float att);
int8_t setFader(char channel, float att);
int8_t setMute(uint8_t channel, uint8_t mute);
int8_t setDCFilter(float fc);

void initSignalScaler(void);


#define ADSP_ToneBand_Bass   0
#define ADSP_ToneBand_Mid    1
#define ADSP_ToneBand_Treble 2

void initTone(struct toneControl* init, bool initPara);
void setTone(struct toneControl para, uint8_t band);

void initGraphicEQ(struct graphicEQ* init, bool initPara);
void setGraphicEQ(struct graphicEQband para);

int8_t setNoiseGeneratorType(uint8_t noiseType);
int8_t setNoiseGeneratorVol(float volL, float volR);
void initNoiseGenerator(void);
void deinitNoiseGenerator(void);

int8_t setSinGeneratorFreq(uint16_t frequency);
int8_t setSinGeneratorVol(float volL, float volR);
void initSinGenerator(void);
void deinitSinGenerator(void);

void initGraphicSApara(struct graphicSA* para);
void initGraphicSA(struct graphicSA para);
void setGraphicSAGain(float gain);

void initQPDpara(struct quasiPD* para);
void initQPD(struct quasiPD para);
uint32_t getQPDPeak(void);
float getQPDPeakdB(void);
uint32_t getQPDAverPeak(void);
float getQPDAverPeakdB(void);

void initKeyFuncPara(struct keyFunc* para);
void initUltraBass(bool onoff);
void setUltraBassGain(float gain);

#endif

