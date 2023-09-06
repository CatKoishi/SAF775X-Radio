
#include "SAF775X.h"
#include "Dirana3BasicDSP.h"

//Dirana3(SAF7751/4/5/8)
#include "Dirana3_ABB_E7A0.h"
#include "Dirana3_ABB_E7A1.h"

#include "gd32f30x.h"
#include "systick.h"

#include <arm_math.h>


/***************************************************************************/

const uint32_t gEQAddr[] = {
	ADSP_Y_GEq_b10L_REL, ADSP_Y_GEq_b20L_REL, ADSP_Y_EQ4_b10L_REL,
	ADSP_Y_GEq_b30L_REL, ADSP_Y_EQ4_b20L_REL, ADSP_Y_GEq_b40_REL , 
	ADSP_Y_GEq_b50_REL , ADSP_Y_EQ4_b30_REL , ADSP_Y_EQ4_b40_REL
};


const uint32_t gSAAddr[] = {
	ADSP_Y_Gsa_a11L_REL, ADSP_Y_Gsa_a12L_REL, ADSP_Y_Gsa_a13L_REL,
	ADSP_Y_Gsa_a14_REL , ADSP_Y_Gsa_a15_REL , ADSP_Y_Gsa_a16_REL , 
	ADSP_Y_Gsa_a17_REL , ADSP_Y_Gsa_a18_REL , ADSP_Y_Gsa_a19_REL
};


/*****************************Local Func************************************/


static int32_t min(int32_t num1, int32_t num2)
{
	if(num1 < num2)
		return num1;
	else
		return num2;
}

static float inRange(float min, float max, float num)
{
	float tmp;
	if(num > max)
		tmp = max;
	else if(num < min)
		tmp = min;
	else
		tmp = num;
	return tmp;
}

// YMEM 12bits -> $
// XMEM 24bits -> #
static uint32_t dataConv(char dataMode, float data)
{
	int32_t out;
	
	if(dataMode == 'X')  // 24 bits, 1 sign bit
	{
		out = min(roundf(data*8388608.0),8388607);
		if(out < 0)
			out &= 0x00FFFFFF;
	}
	else if(dataMode == 'Y')  // 12 bits, 1 sign bit
	{
		out = min(roundf(data*2048.0),2047);
		if(out < 0)
			out &= 0x0FFF;
	}
	else if(dataMode == 'Z')  // 12bits * 2 dual YMEM
	{
		out = min(roundf(data*8388608.0),8388607);
		if(out < 0)
			out &= 0x00FFFFFF;
		uint32_t tmp;
		tmp = (out >> 1) & 0x7FF;
		out = (out & 0x00FFF000) | tmp;
	}
	
	return (uint32_t)out;
}


/***************************************************************************/


void WaitEasyProgram(void)
{
	while(Get_ADSP(ADSP_X_EasyP_Index))
	{
		delay_us(150);
	}
}

int8_t EasyProgramUpdateCoeff(uint32_t* coeff, uint32_t memaddr, uint8_t number)
{
	WaitEasyProgram();
	Set_ADSP_multi(ADSP_Y_UpdatC_Coeff0, coeff, number);
	Set_ADSP(ADSP_X_SrcSw_OrDigSrcSelMask, memaddr);
	Set_ADSP(ADSP_X_EasyP_Index, number);
	
	return 0;
}

void inputSourceSelect(uint8_t channel, uint8_t source)
{
	Set_REGFree(2, channel, source);
}

void analogPower(uint8_t peripheral, uint8_t power)
{
	Set_REGFree(3, 0xA9, peripheral, power);
}


/* -------------------------------- begin ------------------------------ */
/**
  * @Name    : lineDriverMode
  * @brief   :
  * @param   : mode: [输入] ADSP_DACMode_LowNoise : low-noise mode 0.7Vrms
  *                         ADSP_DACMode_HighGain : common mode 1Vrms
 **/
/* -------------------------------- end -------------------------------- */
void lineDriverMode(uint8_t mode)
{
	Set_REGFree(3, 0xA9, 0x31, mode);
}

struct basicControl* sysBC;

void initBasicControl(struct basicControl* init, bool initPara)
{
	sysBC = init;
	
	if(initPara)
	{
		sysBC->mainVol = -3;
		sysBC->balance[0] = 0;
		sysBC->balance[1] = 0;
		sysBC->fader[0] = 0;
		sysBC->fader[1] = 0;
		sysBC->dcBlock = 8.5;
	}
	else
	{
		lineDriverMode(ADSP_DACMode_HighGain);
	}
}

void setMainVol(float VoldB)
{
	float volMain1P,volMain2P;
	uint32_t txvolm[2];
	
	//sysBC->mainVol = VoldB;
	
	if(VoldB >= 0)
	{
		volMain1P = 1;
		volMain2P = powf(10, (VoldB - ADSP_MainVolMax)/20.0);
	}
	else if(VoldB >= -ADSP_MaxLoudBoost)
	{
		volMain1P = powf(10, VoldB/20.0);
		volMain2P = powf(10, (-ADSP_MainVolMax)/20.0);
	}
	else if( VoldB >= (-ADSP_FixedBoost+ADSP_MainVolMax-ADSP_MaxLoudBoost) )
	{
		volMain1P = powf(10, (-ADSP_MaxLoudBoost)/20.0);
		volMain2P = powf(10, (VoldB+ADSP_MaxLoudBoost-ADSP_MainVolMax)/20.0);
	}
	else
	{
		volMain1P = powf(10, (ADSP_FixedBoost+VoldB-ADSP_MainVolMax)/20.0);
		volMain2P = powf(10, (-ADSP_FixedBoost)/20.0);
	}
	
	txvolm[0] = dataConv('Y', volMain1P);
	txvolm[1] = dataConv('Y', volMain2P);
	
	Set_ADSP_multi(ADSP_Y_Vol_Main1P, txvolm, 2);
}


int8_t setBalance(char channel, float att)
{
	float volBal;
	uint32_t txdata;
	
	if(att < -65.9)
		return 1;
	
	volBal = powf(10, att/20.0);
	txdata = dataConv('Y', volBal);
	
	if(channel == 'L')
	{
		//sysBC->balance[0] = att;
		Set_ADSP(ADSP_Y_Vol_BalPL, txdata);
	}
	else if(channel == 'R')
	{
		//sysBC->balance[1] = att;
		Set_ADSP(ADSP_Y_Vol_BalPR, txdata);
	}
	else
		return 1;
	
	return 0;
}

int8_t setFader(char channel, float att)
{
	float volFad;
	uint32_t txdata;
	
	if(att > 66.0)  // inf att
		txdata = 0;
	else
	{
		volFad = powf(10, (-att)/20.0);
		txdata = dataConv('Y', volFad);
	}
	
	if(channel == 'F')
	{
		//sysBC->fader[0] = att;
		Set_ADSP(ADSP_Y_Vol_FadF, txdata);
	}
	else if(channel == 'R')
	{
		//sysBC->fader[1] = att;
		Set_ADSP(ADSP_Y_Vol_FadR, txdata);
	}
	else
		return 1;
	
	return 0;
}

// 1:muted    0:release mute
int8_t setMute(uint8_t channel, uint8_t mute)
{
	uint32_t txdata[2];
	
	if(mute >= 1)
	{
		txdata[0] = 0;
		txdata[1] = 0;
	}
	else
	{
		txdata[0] = 0x7FF;
		txdata[1] = 0x7FF;
	}
	
	if(channel == ADSP_MUTE_MAIN)
	{
		Set_ADSP(ADSP_Y_Mute_P, txdata[0]);
	}
	else if(channel == ADSP_MUTE_FRONT)
	{
		Set_ADSP_multi(ADSP_Y_MuteSix_FL,txdata,2);
	}
	else if(channel == ADSP_MUTE_REAR)
	{
		Set_ADSP_multi(ADSP_Y_MuteSix_RL,txdata,2);
	}
	else
		return 1;
	
	return 0;
}


int8_t setDCFilter(float fc)
{
	float a1, b0;
	uint32_t txSeq[3];
	
	if(fc < 3.5 || fc > 100.0)
		return 1;
	
	//sysBC->dcBlock = fc;
	
	a1 = -tanf(ADSP_PI*(fc/ADSP_FS - 0.25));
	b0 = -0.5-0.5*a1;
	
	txSeq[0] = dataConv('Y', a1);
	txSeq[1] = dataConv('Y', b0);
	txSeq[2] = txSeq[1];
	
	EasyProgramUpdateCoeff(txSeq, ADSP_Y_DCfilt_a1A_REL, 3);
	
	return 0;
}



// Dynamic Compressor


void initSignalScaler(void)
{
	float boost, comp;
	uint32_t txdata;
	
	// pre scaler -> GEQ+Tone
	boost = powf(10, (-ADSP_GeqMaxBoost-ADSP_ToneMaxBoost)/20.0);
	txdata = dataConv('X', boost);
	Set_ADSP(ADSP_X_Vol_OneOverMaxBoostP, txdata);
	// pre scaler -> soft clipping
	// Set Y:Vol_ScalBstP = $C00 when SCLP enabled, default value:$800
	
	// post scaler
	Set_ADSP(ADSP_X_Vol_Boost12dB, 0x000000);  // Fixed Boost = 48.16dB
	comp = ADSP_MainVolMax + ADSP_GeqMaxBoost + ADSP_ToneMaxBoost;
	boost = powf(10, (comp - ADSP_FixedBoost)/20.0);
	txdata = dataConv('Y', boost);
	Set_ADSP(ADSP_Y_Vol_UpScalF, txdata);
	Set_ADSP(ADSP_Y_Vol_UpScalR, txdata);
}

struct toneControl* sysTone;

void setTonePreScal(void)
{
	uint8_t i;
	float maxGain = 0;
	float scal;
	uint32_t txdata;
	
	if(sysTone->bassGain > maxGain)
		maxGain = sysTone->bassGain;
	if(sysTone->midGain > maxGain)
		maxGain = sysTone->midGain;
	if(sysTone->trebleGain > maxGain)
		maxGain = sysTone->trebleGain;
	
	scal = powf(10, (-maxGain)/20.0);
	
	txdata = dataConv('Y', scal);
	Set_ADSP(ADSP_Y_Vol_DesScalBMTP, txdata);
	
}

void initTone(struct toneControl* init, bool initPara)
{
	sysTone = init;
	
	if(initPara)
	{
		sysTone->bassFc = 125;
		sysTone->bassGain = 0;
		sysTone->midFc = 800;
		sysTone->midGain = 0;
		sysTone->midQ = 1;
		sysTone->trebleFc = 5000;
		sysTone->trebleGain = 0;
	}
	else
	{
		// set Tone
		setTone(*sysTone, ADSP_ToneBand_Bass);
		setTone(*sysTone, ADSP_ToneBand_Mid);
		setTone(*sysTone, ADSP_ToneBand_Treble);
	}
}

void setTone(struct toneControl para, uint8_t band)
{
	
	float a1, a2, b0, b1, b2, G, D, t;
	uint32_t tmp;
	uint32_t txSeq[12];
	
	if(band == ADSP_ToneBand_Bass)
	{
		a1 = -0.5*tanf(ADSP_PI*(para.bassFc/ADSP_FS - 0.25));
		b0 = 0.25-0.5*a1;
		b1 = b0;
		if(para.bassGain >=0)
			G = (powf(10, para.bassGain/20.0)-1)/16.0;
		else
			G = -((powf(10, -para.bassGain/20.0)-1)/16.0);
		
		// a1, a2
		tmp = dataConv('Z', a1);
		txSeq[0] = (tmp>>12) & 0xFFF;
		txSeq[1] = tmp & 0xFFF;
		txSeq[2] = 0;
		txSeq[3] = 0;
		// b0, b1, b2
		tmp = dataConv('Z', b0);
		txSeq[4] = (tmp>>12) & 0xFFF;
		txSeq[5] = tmp & 0xFFF;
		txSeq[6] = txSeq[4];
		txSeq[7] = txSeq[5];
		txSeq[8] = 0;
		txSeq[9] = 0;
		// g
		tmp = dataConv('Y', G);
		txSeq[10] = tmp;
		
		EasyProgramUpdateCoeff(txSeq, ADSP_Y_BMT_a1bHP_REL, 11);
	}
	else if(band == ADSP_ToneBand_Mid)
	{
		G = (powf(10, para.midGain/20.0)-1)/16.0;
		if(para.midGain >= 0)
			D = 1;
		else
			D = 1.0/(16*G + 1);
		t = 2*ADSP_PI * para.midFc / ADSP_FS;
		a2 = -0.5*(1-D*t/(2*para.midQ))/(1+D*t/(2*para.midQ));
		a1 = (0.5-a2)*cosf(t);
		b0 = (0.5+a2)/2.0;
		
		tmp = dataConv('Z', a1);
		txSeq[0] = (tmp>>12) & 0xFFF;
		txSeq[1] = tmp & 0xFFF;
		tmp = dataConv('Z', a2);
		txSeq[2] = (tmp>>12) & 0xFFF;
		txSeq[3] = tmp & 0xFFF;
		tmp = dataConv('Z', b0);
		txSeq[4] = (tmp>>12) & 0xFFF;
		txSeq[5] = tmp & 0xFFF;
		tmp = dataConv('Y', G);
		txSeq[6] = tmp;
		
		EasyProgramUpdateCoeff(txSeq, ADSP_Y_BMT_a1mHP_REL, 7);
	}
	else if(band == ADSP_ToneBand_Treble)
	{
		if(para.trebleGain >= 0)
			a1 = -0.5*tanf(ADSP_PI*(para.trebleFc/ADSP_FS - 0.25));
		else
			a1 = -0.5*tanf(ADSP_PI*((para.trebleFc/ADSP_FS)*powf(10, para.trebleGain/20.0) - 0.25));
		b0 = 0.25+0.5*a1;
		b1 = -b0;
		G = (powf(10, para.trebleGain/20.0)-1)/16.0;
		
		txSeq[0] = dataConv('Y', a1);
		txSeq[1] = 0;
		txSeq[2] = dataConv('Y', b0);
		txSeq[3] = dataConv('Y', b1);
		txSeq[4] = 0;
		txSeq[5] = dataConv('Y', G);
		
		EasyProgramUpdateCoeff(txSeq, ADSP_Y_BMT_a1tP_REL, 6);
	}
	
	setTonePreScal();
	
}



// Static/Dynamic Loudness



struct graphicEQ* sysEQ;

void setGraphicEQPreScal(void)
{
	uint8_t i;
	float maxGain = 0;
	float scal;
	uint32_t txdata;
	
	for(i=0;i<9;i++)
	{
		if(sysEQ->band[i].gain > maxGain)
			maxGain = sysEQ->band[i].gain;
	}
	
	scal = powf(10, (-maxGain)/20.0);
	
	txdata = dataConv('Y', scal);
	Set_ADSP(ADSP_Y_Vol_DesScalGEq, txdata);
	
}

void initGraphicEQ(struct graphicEQ* init, bool initPara)
{
	uint8_t i;
	
	sysEQ = init;
	
	if(initPara)
	{
		for(i=0;i<9;i++)
		{
			sysEQ->band[i].band = i;
			sysEQ->band[i].gain = 0;
			sysEQ->band[i].shapeFactor = 1;
		}
		sysEQ->band[0].fc = 47;
		sysEQ->band[1].fc = 94;
		sysEQ->band[2].fc = 188;
		sysEQ->band[3].fc = 375;
		sysEQ->band[4].fc = 750;
		sysEQ->band[5].fc = 1500;
		sysEQ->band[6].fc = 3000;
		sysEQ->band[7].fc = 6000;
		sysEQ->band[8].fc = 12000;
	}
	else
	{
		Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_GraphEQ_Enable);
		delay_us(10);
		Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_GraphEQ_additional4Band);
		delay_us(10);
		
		for(i=0;i<9;i++)
		{
			setGraphicEQ(sysEQ->band[i]);
		}
	}
	
}

void setGraphicEQ(struct graphicEQband para)
{
	float G, t, D, a2, a1, b0;
	uint32_t txG, txa2_H, txa2_L, txa1_H, txa1_L, txb0_H, txb0_L;
	uint32_t txSeq[7];
	G = (powf(10, para.gain / 20.0) - 1) / 4.0;
	t = 2*ADSP_PI * para.fc / ADSP_FS;
	if(para.gain >= 0)
		D = 1;
	else
		D = 1.0/(4.0*G+1);
	a2 = -0.5*(1-D*t/(2*para.shapeFactor))/(1+D*t/(2*para.shapeFactor));
	a1 = (0.5-a2)*cosf(t);
	b0 = (0.5+a2)/2.0;
	
	txG = dataConv('Y', G);
	
	if(para.band > 4)
	{
		txa1_H = dataConv('Y', a1);
		txa2_H = dataConv('Y', a2);
		txb0_H = dataConv('Y', b0);
		
		txSeq[0] = txb0_H;
		txSeq[1] = txa2_H;
		txSeq[2] = txa1_H;
		txSeq[3] = txG;
		
		EasyProgramUpdateCoeff(txSeq, gEQAddr[para.band], 4);
	}
	else
	{
		txa1_H = dataConv('Z', a1);
		txa1_L = txa1_H & 0xFFF;
		txa1_H = (txa1_H>>12) & 0xFFF;
		txa2_H = dataConv('Z', a2);
		txa2_L = txa2_H & 0xFFF;
		txa2_H = (txa2_H>>12) & 0xFFF;
		txb0_H = dataConv('Z', b0);
		txb0_L = txb0_H & 0xFFF;
		txb0_H = (txb0_H>>12) & 0xFFF;
		
		txSeq[0] = txb0_L;
		txSeq[1] = txb0_H;
		txSeq[2] = txa2_L;
		txSeq[3] = txa2_H;
		txSeq[4] = txa1_L;
		txSeq[5] = txa1_H;
		txSeq[6] = txG;
		
		EasyProgramUpdateCoeff(txSeq, gEQAddr[para.band], 7);
	}
	
	setGraphicEQPreScal();
	
}






const uint32_t noiseGeneratorPara[2][9] = {
	{0x333, 0x7ff, 0x814, 0x4cd, 0x7ff, 0x99a, 0x000, 0x150, 0x000},
  {0x820, 0x7ff, 0x7f7, 0x954, 0x7ff, 0x795, 0xfea, 0x06d, 0x046}
};

int8_t setNoiseGeneratorType(uint8_t noiseType)
{
	uint32_t txSeq[9];
	if(noiseType != ADSP_NoiseType_White || noiseType != ADSP_NoiseType_Pink)
		return 1;
	
	for(uint8_t i=0;i<9;i++)
		txSeq[i] = noiseGeneratorPara[noiseType][i];
	
	EasyProgramUpdateCoeff(txSeq, ADSP_Y_NG_a11_REL, 9);
	
	return 0;
}

int8_t setNoiseGeneratorVol(float volL, float volR)
{
	float NGL, NGR;
	uint32_t txSeq[2];
	if(volL > 0 || volR > 0)
		return 1;
	NGL = powf(10, volL/20.0);
	NGR = powf(10, volR/20.0);
	
	txSeq[0] = dataConv('Y', NGL);
	txSeq[1] = dataConv('Y', NGR);
	
	Set_ADSP_multi(ADSP_Y_NG_GL, txSeq, 2);
	
	return 0;
}

void initNoiseGenerator(void)
{
	Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_NoiseGen_Enable);
	inputSourceSelect(ADSP_Channel_Primary, ADSP_Source_NoiseGen);
	setNoiseGeneratorVol(0,0);
	setNoiseGeneratorType(ADSP_NoiseType_Pink);
}

void deinitNoiseGenerator(void)
{
	Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_NoiseGen_SinGen_Disable);
	inputSourceSelect(ADSP_Channel_Primary, ADSP_Source_RadioP);
}



int8_t setSinGeneratorFreq(uint16_t frequency)
{
	float theta, abak;
	uint32_t tmp;
	uint32_t txSeq[3];
	
	theta = 2*ADSP_PI*frequency/ADSP_FS;
	abak = cosf(theta);
	
	tmp = dataConv('Z', abak);
	txSeq[0] = (tmp>>12) & 0xFFF;
	txSeq[1] = tmp & 0xFFF;
	txSeq[2] = 1;
	
	EasyProgramUpdateCoeff(txSeq, ADSP_Y_SinGen_aHbak_REL, 3);
	
	return 0;
}

int8_t setSinGeneratorVol(float volL, float volR)
{
	float SGL, SGR;
	uint32_t txSeq[2];
	if(volL > 0 || volR > 0)
		return 1;
	SGL = -powf(10, volL/20.0);
	SGR = -powf(10, volR/20.0);
	
	txSeq[0] = dataConv('Y', SGL);
	txSeq[1] = dataConv('Y', SGR);
	
	Set_ADSP_multi(ADSP_Y_SinGen_GL, txSeq, 2);
	
	return 0;
}

void initSinGenerator(void)
{
	Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_SinGen_Enable);
	inputSourceSelect(ADSP_Channel_Primary, ADSP_Source_SinGen);
	setSinGeneratorVol(0, 0);
	setSinGeneratorFreq(475);
}

void deinitSinGenerator(void)
{
	Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_NoiseGen_SinGen_Disable);
	inputSourceSelect(ADSP_Channel_Primary, ADSP_Source_RadioP);
}


void initGraphicSApara(struct graphicSA* para)
{
	para->fc[0] = 63;
	para->fc[1] = 125;
	para->fc[2] = 250;
	para->fc[3] = 500;
	para->fc[4] = 1000;
	para->fc[5] = 2000;
	para->fc[6] = 4000;
	para->fc[7] = 8000;
	para->fc[8] = 16000;
	
	para->q[0] = 3;
	para->q[1] = 3;
	para->q[2] = 3;
	para->q[3] = 3;
	para->q[4] = 3;
	para->q[5] = 3;
	para->q[6] = 3;
	para->q[7] = 3;
	para->q[8] = 2.85;
	
	para->inputGain = 3;
	
	para->bandGain[0] = 12;
	para->bandGain[1] = 12;
	para->bandGain[2] = 12;
	para->bandGain[3] = 12;
	para->bandGain[4] = 12;
	para->bandGain[5] = 12;
	para->bandGain[6] = 14;
	para->bandGain[7] = 16;
	para->bandGain[8] = 18;
	
	para->threshold[0] = -20;
	para->threshold[1] = -17;
	para->threshold[2] = -14;
	para->threshold[3] = -11;
	para->threshold[4] = -9;
	para->threshold[5] = -7;
	para->threshold[6] = -5;
	para->threshold[7] = -3;
	
	para->ta = 0.007;
	para->tr = 0.125;
	
  //ADSP_X_Vol_OutPL_REL;
	para->audioSource = ADSP_X_DCfilt_OutPL_REL;
}

void initGraphicSA(struct graphicSA para)
{
	
	float bpGain, t, a1, a2, b0, kp, km, ca, cr, thres;
	uint32_t txSeq[12];
	uint32_t i, tmp;
	
	Set_ADSP(ADSP_X_EasyP_Index, ADSP_EASYP_GSA_Enable);
	
	setGraphicSAGain(para.inputGain);
	
	for(i=0;i<9;i++)
	{
		bpGain = powf(10, para.bandGain[i]/20.0)/16.0;
		tmp = dataConv('Y', bpGain);
		txSeq[i] = tmp;
	}
	EasyProgramUpdateCoeff(txSeq, ADSP_Y_Gsa_Gp1_REL, 9);
	
	for(i=0;i<9;i++)
	{
		t = 2*ADSP_PI*para.fc[i]/ADSP_FS;
		a2 = -0.5*(1-t/(2*para.q[i]))/(1+t/(2*para.q[i]));
		a1 = (0.5-a2)*cosf(t);
		b0 = (0.5+a2)/2.0;
		
		if(i >= 3)
		{
			txSeq[0] = dataConv('Y', a1);
			txSeq[1] = dataConv('Y', a2);
			txSeq[2] = dataConv('Y', b0);
			EasyProgramUpdateCoeff(txSeq, gSAAddr[i], 3);
		}
		else
		{
			tmp = dataConv('Z', a1);
			txSeq[0] = tmp & 0xFFF;
			txSeq[3] = (tmp>>12) & 0xFFF;
			tmp = dataConv('Z', a2);
			txSeq[1] = tmp & 0xFFF;
			txSeq[4] = (tmp>>12) & 0xFFF;
			tmp = dataConv('Z', b0);
			txSeq[2] = tmp & 0xFFF;
			txSeq[5] = (tmp>>12) & 0xFFF;
			EasyProgramUpdateCoeff(txSeq, gSAAddr[i], 6);
		}
	}
	
	ca = powf(ADSP_E, -21.0 / (ADSP_FS * para.ta));
	cr = powf(ADSP_E, -21.0 / (ADSP_FS * para.tr));
	kp = -(cr + ca) / 2.0;
	km = cr - ca;
	
	tmp = dataConv('Z', kp);
	txSeq[0] = tmp & 0xFFF;
	txSeq[1] = (tmp>>12) & 0xFFF;
	tmp = dataConv('Z', km);
	txSeq[2] = tmp & 0xFFF;
	txSeq[3] = (tmp>>12) & 0xFFF;
	EasyProgramUpdateCoeff(txSeq, ADSP_Y_Gsa_KpL_REL, 4);
	
	for(i=0;i<8;i++)
	{
		thres = powf(10, para.threshold[i]/20.0);
		tmp = dataConv('X', thres);
		txSeq[i] = tmp;
	}
	Set_ADSP_multi(ADSP_X_Gsa_Thresh1, txSeq, 8);
	
	Set_ADSP(ADSP_X_Gsa_InPntr, para.audioSource);
}

void setGraphicSAGain(float gain)
{
	float inGain;
	uint32_t txSeq;
	if(gain > 18)
		return;
	
	inGain = powf(10, gain/20.0)/8.0;
	txSeq = dataConv('Y', inGain);
	
	Set_ADSP(ADSP_Y_Gsa_ga, txSeq);
	
}

/*
ADSP_X_DCfilt_OutPL_REL // after dc block
ADSP_X_ToneOutPL_REL // after effects
ADSP_X_Vol_OutPL_REL // after vol_main1p
ADSP_X_Vol_OutFL_REL // after vol system
ADSP_X_FrontOutL_REL
*/
void initQPDpara(struct quasiPD* para)
{
	para->InPntr1 = ADSP_X_DCfilt_OutPL_REL;
	para->InPntr2 = ADSP_X_FrontOutL_REL;
	para->timeAttack = 0.015;
	para->timeHold = 8;
	para->timeRelease = 1.5;
	
}

void initQPD(struct quasiPD para)
{
	float ca, cr, cm, kp, km;
	uint32_t tmp;
	uint32_t txSeq[4];
	
	ca = powf(ADSP_E, -4.0 / (ADSP_FS * para.timeAttack));
	cr = powf(ADSP_E, -4.0 / (ADSP_FS * para.timeRelease));
	kp = -(cr + ca) / 2.0;
	km = cr - ca;
	cm = para.timeHold*ADSP_FS/4.0;
	
	tmp = dataConv('Z', kp);
	txSeq[0] = tmp & 0xFFF;
	txSeq[1] = (tmp>>12) & 0xFFF;
	tmp = dataConv('Z', km);
	txSeq[2] = tmp & 0xFFF;
	txSeq[3] = (tmp>>12) & 0xFFF;
	EasyProgramUpdateCoeff(txSeq, ADSP_Y_QPD_kpL1_REL, 4);
	
	tmp = (uint32_t)cm;
	Set_ADSP(ADSP_X_QPD_COUNTMAX1, tmp);
	
	Set_ADSP(ADSP_X_QPD_InPntr11, para.InPntr1);
	Set_ADSP(ADSP_X_QPD_InPntr12, para.InPntr2);
	
}

uint32_t getQPDPeak(void)
{
	return Get_ADSP(ADSP_X_QPD_NonAverPeak1);
}

float getQPDPeakdB(void)
{
	return log10f(getQPDPeak()/8388608.0);
}

uint32_t getQPDAverPeak(void)
{
	return Get_ADSP(ADSP_X_QPD_Peak1);
}

float getQPDAverPeakdB(void)
{
	return log10f(getQPDAverPeak()/8388608.0);
}


void initFilterCoeff(struct filterCoeff* para)
{
	para->b0 = 0.5;
	para->b1 = 0;
	para->b2 = 0;
	para->a1 = 0;
	para->a2 = 0;
}


void calcFilterNormalPass(uint8_t type, float fc, float gain, float q, struct filterCoeff* para)
{
	float loFc, loGain, loQ;
	float g, a1, b0, b1, b2, a2, t0, t1;
	
	if(type == ADSP_Filter_1stLP)
	{
		loGain  = inRange(-12, 6, gain);
		loFc = inRange(20, 20000, fc);
		g = powf(10, loGain/20.0);
		a1 = -0.5*tanf(ADSP_PI*(loFc/ADSP_FS - 0.25));
		b0 = g*(0.25-0.5*a1);
		b1 = b0;
		b2 = 0;
		a2 = 0;
	}
	else if(type == ADSP_Filter_1stHP)
	{
		loGain  = inRange(-12, 6, gain);
		loFc = inRange(20, 20000, fc);
		g = powf(10, loGain/20.0);
		a1 = -0.5*tanf(ADSP_PI*(loFc/ADSP_FS - 0.25));
		b0 = g*(0.25+0.5*a1);
		b1 = -b0;
		b2 = 0;
		a2 = 0;
	}
	else if(type == ADSP_Filter_2ndLP)
	{
		loGain  = inRange(-12, 0, gain);
		loFc = inRange(20, 20000, fc);
		g = powf(10, loGain/20.0);
		t0 = tanf(ADSP_PI*loFc/ADSP_FS);
		t1 = 1 + ADSP_SQRT2*t0 + t0*t0;
		a1 = (1-t0*t0)/t1;
		a2 = (ADSP_SQRT2*t0-1-t0*t0)/(2*t1);
		b0 = g*t0*t0/(2*t1);
		b1 = 2*b0;
		b2 = b0;
	}
	else if(type == ADSP_Filter_2ndHP)
	{
		loGain  = inRange(-12, 0, gain);
		loFc = inRange(20, 20000, fc);
		g = powf(10, loGain/20.0);
		t0 = tanf(ADSP_PI*loFc/ADSP_FS);
		t1 = 1 + ADSP_SQRT2*t0 + t0*t0;
		a1 = (1-t0*t0)/t1;
		a2 = (ADSP_SQRT2*t0-1-t0*t0)/(2*t1);
		b0 = g/(2*t1);
		b1 = -2*b0;
		b2 = b0;
	}
	else if(type == ADSP_Filter_ShvLP)
	{
		loGain  = inRange(-6, 6, gain);
		loFc = inRange(25, 10000, fc);
		g = powf(10, loGain/20.0);
		if(loGain >= 0)
			a1 = -0.5*tanf(ADSP_PI*(loFc/ADSP_FS - 0.25));
		else
			a1 = -0.5*tanf(ADSP_PI*(loFc/(ADSP_FS*g) - 0.25));
		t0 = 0.25-0.5*a1;
		b2 = 0;
		a2 = 0;
		b0 = 0.5+t0*(g-1);
		b1 = -a1+t0*(g-1);
	}
	else if(type == ADSP_Filter_ShvHP)
	{
		loGain  = inRange(-6, 6, gain);
		loFc = inRange(25, 10000, fc);
		g = powf(10, loGain/20.0);
		if(loGain >= 0)
			a1 = -0.5*tanf(ADSP_PI*(loFc/ADSP_FS - 0.25));
		else
			a1 = -0.5*tanf(ADSP_PI*((loFc*g)/ADSP_FS - 0.25));
		t0 = 0.25+0.5*a1;
		b2 = 0;
		a2 = 0;
		b0 = 0.5+t0*(g-1);
		b1 = -a1-t0*(g-1);
	}
	else if(type == ADSP_Filter_Peak)
	{
		loGain  = inRange(-6, 6, gain);
		loFc = inRange(20, 20000, fc);
		loQ = inRange(0.1, 10, q);
		g = powf(10, loGain/20.0);
		t0 = 2*ADSP_PI*loFc/ADSP_FS;
		if(g >= 1)
			t1 = t0/(2*loQ);
		else
			t1 = t0/(2*g*loQ);
		a2 = -0.5*(1-t1)/(1+t1);
		a1 = (0.5-a2)*cosf(t0);
		b0 = (g-1)*(0.25+0.5*a2)+0.5;
		b1 = -a1;
		b2 = -(g-1)*(0.25+0.5*a2)-a2;
	}
	else
	{
		b0 = 0.5;
		b1 = 0;
		b2 = 0;
		a1 = 0;
		a2 = 0;
	}
	
	para->b0 = b0;
	para->b1 = b1;
	para->b2 = b2;
	para->a1 = a1;
	para->a2 = a2;
}

void initFilterSystemPara(struct filterSystem* para)
{
	// Config All Filter to Flat Mode
	para->GPF1.filterTypeL = ADSP_Filter_Flat;
	para->GPF1.filterTypeR = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF1.filterL);
	initFilterCoeff(&para->GPF1.filterR);
	
	para->GPF2.filterType = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF2.filter);
	
	para->GPF3.filterTypeL = ADSP_Filter_Flat;
	para->GPF3.filterTypeR = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF3.filterL);
	initFilterCoeff(&para->GPF3.filterR);
	
	para->GPF4.filterType = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF4.filter);
	
	para->GPF5.filterType = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF5.filter);
	
	para->GPF6.filterType1 = ADSP_Filter_Flat;
	para->GPF6.filterType2 = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF6.filter1);
	initFilterCoeff(&para->GPF6.filter2);
	
	para->GPF7.filterType1 = ADSP_Filter_Flat;
	para->GPF7.filterType2 = ADSP_Filter_Flat;
	initFilterCoeff(&para->GPF7.filter1);
	initFilterCoeff(&para->GPF7.filter2);
}

void initFilterSystem(struct filterSystem para)
{
	// Enable GPF1,2,3,4,5,6,7
	
	// Config All Filter Signal Path
	
	// Config All Filter Coefficient
	
}

void updateFilter(struct filterSystem para, uint8_t order)
{
	
}












