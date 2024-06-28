#include "ui.h"
#include "gd32f30x.h"
#include "lcd.h"
#include "pic.h"
#include "SAF775X.h"
#include "Dirana3BasicDSP.h"
#include "font.h"
#include <math.h>

/*************************************EXTERN***********************************/

extern struct Dirana3Radio sTuner;
extern uint8_t nGsaVal[10];
extern uint32_t nQPeakDet;

extern struct displayConfig sDisplay;

extern struct basicControl sAudioBasic;
extern struct toneControl sAudioTone;
extern struct graphicEQ sAudioEQ;
extern struct filterSystem sAudioFilter;
extern struct keyFunc sAudioKeyFunc;

extern struct device sDevice;

extern volatile bool bFilterSel;	//是否设置带宽

extern uint8_t nBandCh[NUM_BANDS];

/*************************************CONST************************************/

const char menuName[6][11] = {
	{"Display"},
	{"Audio"},
	{"Radio"},
	{"Search"},
	{"Device"},
	{"About"},
};

//  AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA <- EOF
const char menuDetail[6][41] = {
	{"Backlight, Contrast, Grey Level, etc."},
	{"Audio system. EQ, Tone, Filter, etc."},
	{"Radio receive, signal handling, etc."},
	{"Auto Tuning Save(ATS) configs."},
	{"Device settings, update, alarm, etc."},
	{"Developers & etc."},
};

// Audio
const char audioTitle[8][11] = {
	{"Vol & Bal"},
	{"DC Block"},
	{"Equalizer"},
	{"Tone"},
	{"Wave Gen"},
	{"Filter"},
	{"AutoLE"},
	{"UltraBass"},
};

const char EQTitle[9][4] = {
	{"4 7"},
	{"9 4"},
	{"188"},
	{"375"},
	{"750"},
	{"1K5"},
	{"3 K"},
	{"6 K"},
	{"12K"},
};

// Radio
const char antTitle[2][5] = {
	{"ANT1"},
	{"ANT2"},
};

const char bwTitle[4][5] = {
	{"Wide"},	
	{"MidW"},
	{"MidL"},
	{"Narr"},
};

const char deemTitle[2][5] = {
	{"50us"},
	{"75us"},
};

const char levTitle[10][5] = {
	{"Lev0"},
	{"Lev1"},
	{"Lev2"},
	{"Lev3"},
	{"Lev4"},
	{"Lev5"},
	{"Lev6"},
	{"Lev7"},
	{"Lev8"},
	{"Lev9"},
};

const char regionTitle[4][5] = {
	{"CN"},	// 87~108
	{"OIRT"}, //65~73
	{"JP"}, //76~90
	{"US"}, //87.5~108
};

const char atsTitle[2][5] = {
	{"SLOW"},
	{"FAST"},
};

/******************************************************************************/

uint8_t Judge_length(uint16_t v) {
	if (v < 10)
		return 1;
	else if (v < 100) 
		return 2;
	else if (v < 1000) 
		return 3;
	else if (v < 10000) 
		return 4;
	else 
		return 5;
}


// input 12345, 123
// output str[] = {1,2,3,4,5,0,0,...}, {1,2,3,0,0,0...};
// return len = 5, 3
uint8_t num2str(uint16_t num, uint8_t* str)
{
	uint16_t tmp = num;
	uint8_t len = Judge_length(num);
	int8_t i;
	
	for(i=len-1; i>=0; i--)
	{
		str[i] = tmp%10;
		tmp /= 10;
	}
	return len;
}

void stroffset(uint8_t* str, uint8_t length)
{
	uint8_t i;
	for(i=0; i<length; i++)
	{
		str[i] += 48;
	}
}

uint8_t myround(uint16_t number)
{
	uint8_t mantissa, temp;
	mantissa = number%10;
	temp = number/10;
	if(mantissa>=5)
		return temp + 1;
	else
		return temp;
}




void UI_DrawScrollBar(uint8_t totalNum, uint8_t nowPos)
{
	static uint8_t lastPos = 0;
	char tmp = nowPos;
	if(lastPos != nowPos) // clear last pos
		GUI_Line_VH(251, 8+80*lastPos/totalNum, DIR_V, 8, 4, COLOR_WHITE);
	// draw scroll bar
	GUI_Line_VH(252, 4, DIR_V, 80, 2, COLOR_LIGHT);
	GUI_Line_VH(251, 8+80*nowPos/totalNum, DIR_V, 8, 4, COLOR_BLACK);
	if (tmp < 9)
		tmp = tmp + '1';
	else
		tmp = tmp - 9 + 'A';
	GUI_Char(250,86,tmp,&Font8,COLOR_BLACK,COLOR_WHITE);
	lastPos = nowPos;
}


void UI_DrawSelectBar(uint8_t pos)
{
	static uint8_t lastPos = 0;
	// normallized pos
	uint8_t nowPos = pos%4;
	if(lastPos != nowPos) // clear last pos
		GUI_FillBuff(0,2+24*lastPos,4,20,COLOR_WHITE);
	
	GUI_FillBuff(0,2+24*nowPos,4,20,COLOR_BLACK);
	
	lastPos = nowPos;
}

const uint8_t slot_xs[4] = {8,8,8,8};
const uint8_t slot_ys[4] = {2,26,50,74};

void UI_DrawText(uint8_t slot, const unsigned char* icon, const char* name, bool isSelect)
{
	// draw icon
	GUI_DrawBuff_Dot(slot_xs[slot], slot_ys[slot], 20, 20, MODE_GREY, 0,0, icon);
	// draw box name
	GUI_Text(slot_xs[slot]+20+4,slot_ys[slot],slot_xs[slot]+20+4+160,slot_ys[slot]+20,name,&Font20,COLOR_BLACK,isSelect ? COLOR_LIGHT:COLOR_WHITE);
}

void UI_DrawValue(uint8_t ys, int32_t number, const char* unit)
{
	uint8_t unitL = 0;
	const char* ptr = unit;
	
	// get unit length
	while(*ptr != 0)
	{
		unitL+=10;
		ptr++;
	}
	// clear number aera
	GUI_FillBuff(256-6-4-unitL-6*10,ys+2,6*10,16,COLOR_WHITE);
	// draw number right aligment
	GUI_Number(256-6-4-unitL, ys+2, number, ALIGNMENT_RIGHT, &Font20, COLOR_BLACK, COLOR_WHITE);
	// draw unit
	GUI_Text(256-6-4-unitL, ys+2, 246, ys+2+20, unit, &Font20, COLOR_BLACK, COLOR_WHITE);
}

void UI_DrawFloat(uint8_t ys, float number, uint8_t numFrac, const char* unit)
{
	uint8_t unitL = 0;
	const char* ptr = unit;
	
	// get unit length
	while(*ptr != 0)
	{
		unitL+=10;
		ptr++;
	}
	// clear number aera
	GUI_FillBuff(256-6-4-unitL-6*10,ys+2,6*10,16,COLOR_WHITE);
	// draw number right aligment
	GUI_Float(256-6-4-unitL, ys+2, number, numFrac, ALIGNMENT_RIGHT, &Font20, COLOR_BLACK, COLOR_WHITE);
	// draw unit
	GUI_Text(256-6-4-unitL, ys+2, 246, ys+2+20, unit, &Font20, COLOR_BLACK, COLOR_WHITE);
}

// name less than 16 char
void UI_DrawCheckBox(uint8_t slot, const unsigned char* icon, const char* name, bool isSelect, bool boxState)
{
	// draw icon
	GUI_DrawBuff_Dot(slot_xs[slot], slot_ys[slot], 20, 20, MODE_GREY, 0,0, icon);
	// draw box name
	GUI_Text(slot_xs[slot]+20+4,slot_ys[slot],slot_xs[slot]+20+4+160,slot_ys[slot]+20,name,&Font20,isSelect ? COLOR_BLACK:COLOR_LIGHT,COLOR_WHITE);
	// draw box outline
	GUI_Rectangle(230,slot_ys[slot],16,16,1,COLOR_BLACK);
	// draw check box
	GUI_FillBuff(230+3,slot_ys[slot]+3,11,11,boxState ? COLOR_BLACK:COLOR_WHITE);
}

// name less than 10 char
void UI_DrawValueBox(uint8_t slot, const unsigned char* icon, const char* name, bool isSelect, int32_t number, const char* unit)
{
	uint8_t unitL = 0;
	const char* ptr = unit;
	
	// get unit length
	while(*ptr != 0)
	{
		unitL++;
		ptr++;
	}
	
	// draw icon
	GUI_DrawBuff_Dot(slot_xs[slot], slot_ys[slot], 20, 20, MODE_GREY, 0,0, icon);
	// draw box name
	GUI_Text(slot_xs[slot]+20+4,slot_ys[slot],slot_xs[slot]+20+4+120,slot_ys[slot]+20,name,&Font20,isSelect ? COLOR_BLACK:COLOR_LIGHT,COLOR_WHITE);
	// clear number aera
	GUI_FillBuff(256-6-4-unitL*8-6*8,slot_ys[slot],6*8,16,COLOR_WHITE);
	// draw number right aligment
	GUI_Number(256-6-4-unitL*8, slot_ys[slot]+2, number, ALIGNMENT_RIGHT, &Font16, COLOR_BLACK, COLOR_WHITE);
	// draw unit
	GUI_Text(256-6-4-unitL*8, slot_ys[slot]+2, 246, slot_ys[slot]+2+16, unit, &Font16, COLOR_BLACK, COLOR_WHITE);
}

// name less than 16 char  text less than 6 char
void UI_DrawTextBox(uint8_t slot, const unsigned char* icon, const char* name, bool isSelect, const char* text)
{
	uint8_t unitL = 0;
	const char* ptr = text;
	
	// get unit length
	while(*ptr != 0)
	{
		unitL+=8;
		ptr++;
	}
	
	// draw icon
	GUI_DrawBuff_Dot(slot_xs[slot], slot_ys[slot], 20, 20, MODE_GREY, 0,0, icon);
	// draw box name
	GUI_Text(slot_xs[slot]+20+4,slot_ys[slot],slot_xs[slot]+20+4+160,slot_ys[slot]+20,name,&Font20,isSelect ? COLOR_BLACK:COLOR_LIGHT,COLOR_WHITE);
	// clear number aera
	GUI_FillBuff(256-6-4-48,slot_ys[slot],48,16,COLOR_WHITE);
	// draw unit
	GUI_Text(256-6-4-unitL, slot_ys[slot]+2, 246, slot_ys[slot]+2+16, text, &Font16, COLOR_BLACK, COLOR_WHITE);
}

/***********************************SPEC FUNC**********************************/

void UI_DrawOneGSA(uint16_t band, uint8_t h)
{
	uint8_t high = 0;
	
	while(h)
	{
		h=h>>1;
		high+=4;
	}
	
	GUI_FillBuff(74+band*12, 64,      9, 32-high, COLOR_WHITE);
	GUI_FillBuff(74+band*12, 96-high, 9,    high, COLOR_BLACK);
}

void UI_DrawVU(uint32_t val)
{
	// 50段 40段浅色 10段深色
	static int32_t max_vol = 0;
	if(max_vol < val)
		max_vol = val;
	
	int32_t percent = (val*50.0/max_vol);
	int32_t overflow = percent-40;
	
	//GUI_Rectangle(196+2,80+2,54,14,1,COLOR_BLACK);
	
	if(percent >= 40)
	{
		percent = 40;
		GUI_FillBuff(196+2+2,80+2+2,percent,10,COLOR_DARK);
		GUI_FillBuff(196+2+2+40,80+2+2,overflow,10,COLOR_BLACK);
		GUI_FillBuff(196+2+2+40+overflow,80+2+2,10-overflow,10,COLOR_WHITE);
	}
	else
	{
		GUI_FillBuff(196+2+2,80+2+2,percent,10,COLOR_DARK);
		GUI_FillBuff(196+2+2+percent,80+2+2,50-percent,10,COLOR_WHITE);
	}
	if(max_vol > 0)
		max_vol = max_vol - 1;
}

void UI_DrawEqBand(int8_t band, int8_t sel)
{
	uint16_t xs = 12;
	GUI_Line_VH(xs+12+24*band, 16+3, DIR_V, 50, 4, COLOR_LIGHT);
	GUI_Line_VH(xs+12+24*band, 43-2*sAudioEQ.band[band].gain, DIR_V, 3, 4, COLOR_BLACK);
	
	GUI_Text(xs+6+24*band,72,256,96,EQTitle[band],&Font8,COLOR_BLACK,COLOR_WHITE);
	GUI_FillBuff(xs+6+24*band,80,18,16,COLOR_WHITE);
	if(sel == 0)
	{
		GUI_Number(xs+0+24*(band+1),80,sAudioEQ.band[band].gain,ALIGNMENT_RIGHT,&Font8,COLOR_WHITE,COLOR_BLACK);
		GUI_Float(xs+0+24*(band+1),88,sAudioEQ.band[band].shapeFactor,1,ALIGNMENT_RIGHT,&Font8,COLOR_BLACK,COLOR_WHITE);
	}
	else if(sel == 1)
	{
		GUI_Number(xs+0+24*(band+1),80,sAudioEQ.band[band].gain,ALIGNMENT_RIGHT,&Font8,COLOR_BLACK,COLOR_WHITE);
		GUI_Float(xs+0+24*(band+1),88,sAudioEQ.band[band].shapeFactor,1,ALIGNMENT_RIGHT,&Font8,COLOR_WHITE,COLOR_BLACK);
	}
	else
	{
		GUI_Number(xs+0+24*(band+1),80,sAudioEQ.band[band].gain,ALIGNMENT_RIGHT,&Font8,COLOR_BLACK,COLOR_WHITE);
		GUI_Float(xs+0+24*(band+1),88,sAudioEQ.band[band].shapeFactor,1,ALIGNMENT_RIGHT,&Font8,COLOR_BLACK,COLOR_WHITE);
	}
}


void UI_DrawEq(int8_t band, int8_t sel)
{
	static int8_t lastBand = 0;
	
	if(lastBand != band)
		UI_DrawEqBand(lastBand,-1);
	
	UI_DrawEqBand(band,sel);
	lastBand = band;
}

void UI_DrawToneBar(int16_t band, int8_t gain)
{
	GUI_FillBuff(8+80+4,24*(band+1)+4,100,16,COLOR_LIGHT);
	GUI_FillBuff(8+80+4+(gain+12)*4,24*(band+1)+4,4,16,COLOR_BLACK);
}

void UI_DrawTone(uint8_t sel)
{
	UI_DrawValue(24,sAudioTone.bassGain,"dB");
	UI_DrawValue(48,sAudioTone.midGain,"dB");
	UI_DrawValue(72,sAudioTone.trebleGain,"dB");
	UI_DrawSelectBar(sel+1);
	UI_DrawToneBar(0,sAudioTone.bassGain);
	UI_DrawToneBar(1,sAudioTone.midGain);
	UI_DrawToneBar(2,sAudioTone.trebleGain);
}

void GUI_SeekALL(uint16_t fmin, uint16_t fmax, uint16_t fnow, uint8_t number, bool force)
{
	static uint8_t vpercent = 100;
	static uint8_t vnumber = 100;
	
	// xs:33 ys:48+3
	// len:160 height:19
	
	uint8_t percent = 160*(fnow-fmin)/(fmax-fmin);
	
	if(force == true)
	{
		vnumber = 100;
		GUI_FillBuff(33,51,159,19,COLOR_WHITE);
		GUI_FillBuff(231,54,21,12,COLOR_WHITE);
	}
	
	if(number != vnumber)
	{
		GUI_Number(231,54,number,ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
		vnumber = number;
	}
	
	if(percent != vpercent)
	{
		GUI_FillBuff(33,51,percent,19,COLOR_BLACK);
		vpercent = percent;
	}
}



/***********************************INTERFACE**********************************/

const uint8_t stepIndi[] = {170,140,105,75,45};

//extern uint16_t nBandStep[NUM_BANDS][NUM_STEPS];

void UI_Main(bool init)
{
	static uint16_t	vfreq 		= 0;
	static uint8_t	vstep 		= 0;
	static uint8_t	vfilter		= 0;
	static bool     vfiltsel  = 0;
	static uint8_t	vband 		= 0;
	static bool 		vstereo 	= 0;
	static int8_t		vrssi 		= 0;
	
	static uint16_t	vbat 			= 0;
	static uint8_t	vvol    	= 0;
	
	static bool 		vmute 		= 0;
	static uint8_t 	vchan   	= 0;
	static bool 		vfsel 		= 0;
	
	static uint8_t  vsec      = 0;
		
	uint8_t len;
	uint8_t str[16];
	uint16_t tmp, data, i, j;
	
	if(init)
	{
		// clear buff
		GUI_ClearBuff(COLOR_WHITE);
		
		// draw icon
		GUI_DrawBuff_Dot(2,24,34,12,MODE_GREY,-1,-1,img_bandwidth34x12);
		
		GUI_DrawBuff_Origin(0,64,16,16,img_antenna);
		GUI_DrawBuff_Origin(42,64,16,16,img_dbu);  // x-axis 2 pix
		
		GUI_DrawBuff_Origin(0,80,16,16,img_bat);
		GUI_DrawBuff_Origin(42,80,16,16,img_volt);
		
		GUI_DrawBuff_Origin(196,64,16,16,img_spk);
		GUI_DrawBuff_Origin(238,64,16,16,img_percent);
		
		GUI_Rectangle(196+2,80+2,53,13,1,COLOR_BLACK);
	}
	
	// frequency
	if(vfreq != sTuner.Radio.nBandFreq[sTuner.Radio.nBandMode] || vstep != sTuner.Radio.nFreqStep[sTuner.Radio.nBandMode] || init)
	{
		vfreq = sTuner.Radio.nBandFreq[sTuner.Radio.nBandMode];
		len = num2str(vfreq, str);
		
		GUI_FillBuff_Origin(41, 0, 60, 48, 0x00);  // Clear first two digits
		
		GUI_DrawBuff_Origin(166, 0, 30, 48, img_num48x+str[len-1]*360);
		GUI_DrawBuff_Origin(136, 0, 30, 48, img_num48x+str[len-2]*360);
		GUI_DrawBuff_Origin(101, 0, 30, 48, img_num48x+str[len-3]*360);
		if(len >= 4)
			GUI_DrawBuff_Origin(71, 0, 30, 48, img_num48x+str[len-4]*360);
		if(len >= 5)
			GUI_DrawBuff_Origin(41, 0, 30, 48, img_num48x+str[len-5]*360);
		
		// draw operate indicator
		vstep = sTuner.Radio.nFreqStep[sTuner.Radio.nBandMode];
		tmp = nBandStep[sTuner.Radio.nBandMode][vstep];
		len = Judge_length(tmp);
		GUI_FillBuff(stepIndi[len-1],46,22,2,COLOR_DARK);
		if(tmp == 5)
			GUI_Text(198,0,-1,-1,"x5KH",&Font12,COLOR_BLACK,COLOR_WHITE);
		else if(tmp == 9)
			GUI_Text(198,0,-1,-1,"x9KH",&Font12,COLOR_BLACK,COLOR_WHITE);
		else
			GUI_FillBuff(198,0,28,12,COLOR_WHITE);
	}
	
	// filter
	if(vfilter != sTuner.Config.nBandFilter[sTuner.Radio.nRFMode] || vfiltsel != bFilterSel || init)
	{
		vfilter = sTuner.Config.nBandFilter[sTuner.Radio.nRFMode];
		vfiltsel = bFilterSel;
		if(sTuner.Radio.nRFMode == RFMODE_FM)
			GUI_DrawBuff_Dot(2,24+14,34,8,MODE_MONO,COLOR_BLACK,vfiltsel? COLOR_LIGHT:COLOR_WHITE,img_filter_fm+34*vfilter);
		else
			GUI_DrawBuff_Dot(2,24+14,34,8,MODE_MONO,COLOR_BLACK,vfiltsel? COLOR_LIGHT:COLOR_WHITE,img_filter_am+34*vfilter);
	}
	
	if(vband != sTuner.Radio.nBandMode || init)
	{
		vband = sTuner.Radio.nBandMode;
		switch(vband)
		{
			case 0 :{	//FM
				GUI_DrawBuff_Origin(131, 40, 5, 8, img_dot48x);
				GUI_DrawBuff_Origin(196,12 ,59,36,img_unit+531);
			};break;
			case 1 :{	//LW
				GUI_FillBuff_Origin(131, 40, 5, 8, 0x00);  // erase dot, draw khz
				GUI_DrawBuff_Origin(196,12,59,36,img_unit);
			};break;
			case 2 :{	//MW
				GUI_DrawBuff_Origin(196,12,59,36,img_unit);
			};break;
			case 3 :{	//SW
				GUI_DrawBuff_Origin(196,12,59,36,img_unit);
			};break;
		}
	}
	
	if(vstereo != sTuner.Status.bSTIN || init)
	{
		vstereo = sTuner.Status.bSTIN;
		if(vstereo)
			GUI_DrawBuff_Origin(4,0,30,24,img_stereo30x24);
		else
			GUI_FillBuff_Origin(4,0,30,24,0x00);
	}
	
	if(vrssi != sTuner.Status.nRSSI || init)
	{
		vrssi = sTuner.Status.nRSSI;
		
		if(vrssi < 0)
		{
			data = -vrssi;
			GUI_DrawBuff_Origin(16,64,8,16,img_num16x+320);
		}
		else
		{
			data = vrssi;
			GUI_FillBuff_Origin(16,64,8,16,0x00);
		}
		len = num2str(data, str);
		if(len == 1)
		{
			GUI_DrawBuff_Origin(26,64,8,16,img_num16x);
			GUI_DrawBuff_Origin(34,64,8,16,img_num16x+str[0]*32);
		}
		else
		{
			GUI_DrawBuff_Origin(26,64,8,16,img_num16x+str[0]*32);
			GUI_DrawBuff_Origin(34,64,8,16,img_num16x+str[1]*32);
		}
	}
	
	if(vbat != sDevice.nBatVolt || init)
	{
		vbat = sDevice.nBatVolt;
		num2str(vbat, str);
		GUI_DrawBuff_Origin(16,80,8,16,img_num16x+str[0]*32);
		GUI_DrawBuff_Origin(24,92,2, 4,img_dot16x);
		GUI_DrawBuff_Origin(26,80,8,16,img_num16x+str[1]*32);
		GUI_DrawBuff_Origin(34,80,8,16,img_num16x+str[2]*32);
	}
	
	if(vvol != sTuner.Audio.nVolume[sTuner.Audio.index] || init)
	{
		vvol = sTuner.Audio.nVolume[sTuner.Audio.index];
		
		len = num2str(vvol, str);
		GUI_DrawBuff_Origin(230,64,8,16,img_num16x+str[len-1]*32);
		if(len >= 2)
			GUI_DrawBuff_Origin(222,64,8,16,img_num16x+str[len-2]*32);
		else
			GUI_FillBuff_Origin(222,64,8,16,0x00);
		if(len >= 3)
			GUI_DrawBuff_Origin(212,64,8,16,img_num16x+str[len-3]*32);
		else
			GUI_FillBuff_Origin(212,64,8,16,0x00);
	}
	
	if(vchan != nBandCh[sTuner.Radio.nBandMode] || init)
	{
		vchan = nBandCh[sTuner.Radio.nBandMode];
		
		GUI_FillBuff_Origin(0,48,36,16,COLOR_WHITE);
		GUI_Text(1,50,-1,-1,"CH.",&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Number(22,50,vchan,ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
	}
	
	// GSA
	if(nGsaVal[0] == 1 || init)
	{
		nGsaVal[0] = 0;
		// draw
		for(i=0;i<9;i++)
		{
			UI_DrawOneGSA(i, nGsaVal[i+1]);
		}
		
		UI_DrawVU(nQPeakDet);
	}
	
	if(vmute != sTuner.Audio.bMuted || init)
	{
		vmute = sTuner.Audio.bMuted;
		if(vmute)
			GUI_DrawBuff_Origin(196,64,16,16,img_spkMute);
		else
			GUI_DrawBuff_Origin(196,64,16,16,img_spk);
	}
	if(vsec != sDevice.sTime.second || init)
	{
		vsec = sDevice.sTime.second;
		GUI_Char(197,50,sDevice.sTime.hour/10+'0',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(204,50,sDevice.sTime.hour%10+'0',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(211,50,':',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(218,50,sDevice.sTime.minute/10+'0',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(225,50,sDevice.sTime.minute%10+'0',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(232,50,':',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(239,50,sDevice.sTime.second/10+'0',&Font12,COLOR_BLACK,COLOR_WHITE);
		GUI_Char(246,50,sDevice.sTime.second%10+'0',&Font12,COLOR_BLACK,COLOR_WHITE);
	}
	
}

void UI_Menu(int8_t index, bool init)
{
	if(init)
	{
		//GUI_ClearBuff(COLOR_WHITE);
		// vertical line between icon&text
		//GUI_Line_VH(12+50+1, 23, DIR_V, 50, 2, COLOR_BLACK);
		// horizontal line between name&detail
		//GUI_Line_VH(12+50+4,23+24+1, DIR_H, 120, 1, COLOR_LIGHT);
	}
	
	GUI_ClearBuff(COLOR_WHITE);
	// vertical line between icon&text
	//GUI_Line_VH(12+50+1, 23, DIR_V, 50, 2, COLOR_BLACK);
	// horizontal line between name&detail
	GUI_Line_VH(12+50+4,23+24+1, DIR_H, 120, 1, COLOR_LIGHT);
	
	switch(index)
	{
		case 0 :GUI_DrawBuff_Dot(12, 23, 50, 50, MODE_GREY, 0, 0, img_mDisplay50x);break;
		case 1 :GUI_DrawBuff_Dot(12, 23, 50, 50, MODE_GREY, 0, 0, img_mUntitled50x);break;
		case 2 :GUI_DrawBuff_Dot(12, 23, 50, 50, MODE_GREY, 0, 0, img_mUntitled50x);break;
		case 3 :GUI_DrawBuff_Dot(12, 23, 50, 50, MODE_GREY, 0, 0, img_mUntitled50x);break;
		case 4 :GUI_DrawBuff_Dot(12, 23, 50, 50, MODE_GREY, 0, 0, img_mUntitled50x);break;
		case 5 :GUI_DrawBuff_Dot(12, 23, 50, 50, MODE_GREY, 0, 0, img_mAbout50x);break;
		default :   break;
	}
	
	GUI_Text(12+50+4, 23,      12+50+4+120, 23+24,      menuName[index], &Font24, COLOR_BLACK, COLOR_WHITE);
	GUI_Text(12+50+4, 23+24+6, 12+50+4+120, 23+24+6+16, menuDetail[index], &Font8, COLOR_DARK, COLOR_WHITE);
	
	if(index > 0)
	{
		GUI_Text(12+50+4, 3, 12+50+4+120, 3+12, menuName[index-1], &Font12, COLOR_DARK, COLOR_WHITE);
		// ARROR UP
		GUI_DrawBuff_Dot(25, 3, 24,12,MODE_GREY,0,0,img_arrowUp12x);
	}
	if(index < MENU_MAIN_INDEX-1)
	{
		GUI_Text(12+50+4, 23+24+6+20+6, 12+50+4+120, 23+24+6+20+6+12, menuName[index+1], &Font12, COLOR_DARK, COLOR_WHITE);
		// ARROR DOWN
		GUI_DrawBuff_Dot(25, 23+24+6+20+6, 24,12,MODE_GREY,0,0,img_arrowDown12x);
	}
	
	// draw scroll bar
	UI_DrawScrollBar(MENU_MAIN_INDEX, index);
}





void UI_Display(int8_t index, bool init)
{
	static uint8_t lastPage = 0;
	uint8_t nowPage = index/4;
	bool aa = init;
	
	if(lastPage != nowPage)
	{
		aa = true;
		lastPage = nowPage;
	}
	
	if(aa)
	{
		GUI_ClearBuff(COLOR_WHITE);
		if(nowPage == 0)
		{
			UI_DrawValueBox(0,img_bright20x,"Brightness", true, sDisplay.brightness, "%");
			UI_DrawValueBox(1,img_lightTime20x,"Light Time", true, sDisplay.backTime, "S");
			UI_DrawValueBox(2,img_contrast20x,"Contrast", true, sDisplay.contrast, "");
			UI_DrawCheckBox(3,img_invDisp20x,"Inverse Disp", true, sDisplay.invDisp);
		}
		else
		{
			UI_DrawValueBox(0,img_commonSet20x,"Light Grey", true, sDisplay.greyLevel[0], "");
			UI_DrawValueBox(1,img_commonSet20x,"Dark Grey", true, sDisplay.greyLevel[1], "");
			GUI_Rectangle(32,48+2,160,20,1,COLOR_BLACK);
			GUI_FillBuff(33,48+3,39,19,COLOR_WHITE);
			GUI_FillBuff(72,48+3,40,19,COLOR_LIGHT);
			GUI_FillBuff(112,48+3,40,19,COLOR_DARK);
			GUI_FillBuff(152,48+3,40,19,COLOR_BLACK);
		}
	}
	
	switch(index)
	{
		case 0 :UI_DrawValueBox(0,img_bright20x,"Brightness", true, sDisplay.brightness, "%");break;
		case 1 :UI_DrawValueBox(1,img_lightTime20x,"Light Time", true, sDisplay.backTime, "S");break;
		case 2 :UI_DrawValueBox(2,img_contrast20x,"Contrast", true, sDisplay.contrast, "");break;
		case 3 :UI_DrawCheckBox(3,img_invDisp20x,"Inverse Disp", true, sDisplay.invDisp);break;
		case 4 :UI_DrawValueBox(0,img_commonSet20x,"Light Grey", true, sDisplay.greyLevel[0], "");break;
		case 5 :UI_DrawValueBox(1,img_commonSet20x,"Dark Grey", true, sDisplay.greyLevel[1], "");break;
		default : break;
	}
	
	UI_DrawSelectBar(index);
	UI_DrawScrollBar(MENU_DISP_INDEX, index);
	
}




void UI_Audio(int8_t index, int8_t band, int8_t sel, bool init)
{
	static int8_t lastIndex = 0;
	bool aa = init;
	uint8_t titleL = 0;
	const char* ptr = audioTitle[index];
	int16_t i,j;
	
	if(lastIndex != index)
	{
		aa = true;
		lastIndex = index;
	}
	
	// get title length
	while(*ptr != 0)
	{
		titleL+=8;
		ptr++;
	}
	
	if(aa) // init or change page
	{
		GUI_ClearBuff(COLOR_WHITE);
		GUI_Text((256-titleL)/2,0,256,16,audioTitle[index],&Font16,COLOR_BLACK,COLOR_WHITE);
		if(index > 0)
		{
			// arrow left
			GUI_DrawBuff_Origin(61,0,11,16,img_arrowLeft11x16);
		}
		if(index < MENU_AUDIO_INDEX-1)
		{
			// arrow right
			GUI_DrawBuff_Origin(184,0,11,16,img_arrowRight11x16);
		}
		
		switch(index)
		{
			case 0:{ // main vol / audio gain & balance
				UI_DrawSelectBar(band+1);
				GUI_DrawBuff_Dot(8,24+2,47,20,MODE_GREY,0,0,img_volGain47x20);
				GUI_Text(8+47+8,24+2,256,24+2+20,"Vol Gain:",&Font20, COLOR_BLACK, COLOR_WHITE);
				GUI_DrawBuff_Dot(8,48+2,47,20,MODE_GREY,0,0,img_balLeft47x20);
				GUI_Text(8+47+8,48+2,256,48+2+20,"Bal Left:",&Font20, COLOR_BLACK, COLOR_WHITE);
				GUI_DrawBuff_Dot(8,72+2,47,20,MODE_GREY,0,0,img_balRight47x20);
				GUI_Text(8+47+8,72+2,256,72+2+20,"Bal Right:",&Font20, COLOR_BLACK, COLOR_WHITE);
			};break;
			
			case 1:{ // dc block
				GUI_DrawBuff_Dot(8,24+2,44,44,MODE_GREY,0,0,img_filtHP44x44);
				GUI_Text(8+44+8, 24+2, 256,24+2+20,"1st IIR HighPass",&Font20,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(8+44+8, 48+2, 256,48+2+20,"[3.5 ~ 100] 0.5/div",&Font20,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(8,72+2,256,72+2+20,"Fc:",&Font20,COLOR_BLACK,COLOR_WHITE);
			};break;
			
			case 2:{ // eq
				UI_DrawEqBand(0,0);
				for(i=1;i<9;i++)
					UI_DrawEqBand(i,-1);
				GUI_Text(0,72,-1,-1,"F:",&Font8,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(0,80,-1,-1,"G:",&Font8,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(0,88,-1,-1,"Q:",&Font8,COLOR_BLACK,COLOR_WHITE);
			};break;
			
			case 3:{ // tone
				GUI_Text(8,24+2,-1,-1,"Bass  :",&Font20,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(8,48+2,-1,-1,"Mid   :",&Font20,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(8,72+2,-1,-1,"Treble:",&Font20,COLOR_BLACK,COLOR_WHITE);
				UI_DrawTone(band);
			};break;
			
			case 4:{ // wave gen
				
			};break;
			
			case 5:{ // filter
				
			};break;
			
			case 6:{ // ALE
				
			};break;
			
			case 7:{ // Ultra Bass
				GUI_DrawBuff_Dot(8,24+2,44,44,MODE_GREY,0,0,img_ultraBass44x44);
				GUI_Text(8+44+8, 24+2, 256,24+2+20,"Adaptive UltraBass",&Font20,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(8+44+8, 48+2, 256,48+2+20,"[0 ~ 24] 1dB/div",&Font20,COLOR_BLACK,COLOR_WHITE);
				GUI_Text(8,72+2,256,72+2+20,"Gain:",&Font20,COLOR_BLACK,COLOR_WHITE);
			};break;
		}
	}
	
	
	switch(index)
	{
		case 0:{ // main vol / audio gain & balance
			UI_DrawSelectBar(band+1);
			UI_DrawValue(24,sAudioBasic.mainVol,"dB");
			UI_DrawValue(48,sAudioBasic.balance[0],"dB");
			UI_DrawValue(72,sAudioBasic.balance[1],"dB");
		};break;
		
		case 1:{ // dc block
			UI_DrawFloat(72,sAudioBasic.dcBlock,1,"Hz");
		};break;
		
		case 2:{ // eq
			UI_DrawEq(band,sel);
		};break;
		
		case 3:{ // tone
			UI_DrawTone(band);
		};break;
		
		case 4:{ // wave gen
			
		};break;
		
		case 5:{ // filter
			
		};break;
		
		case 6:{ // ALE
			
		};break;
		
		case 7:{ // UltraBass
			UI_DrawFloat(72,sAudioKeyFunc.AUBGain,1,"dB");
		};break;
	}
}



void UI_Radio(int8_t index, bool init)
{
	static uint8_t lastPage = 0;
	uint8_t nowPage = index/4;
	bool aa = init;
	bool demo = sTuner.Config.bDemoMode;
	
	if(lastPage != nowPage)
	{
		aa = true;
		lastPage = nowPage;
	}
	
	if(aa)
	{
		GUI_ClearBuff(COLOR_WHITE);
		if(nowPage == 0)
		{
			UI_DrawTextBox(0,img_commonSet20x,"Antanna Select",true,antTitle[sTuner.Config.nFMANTsel]);
			UI_DrawCheckBox(1,img_commonSet20x,"FM iPD Dual Ant",demo?true:false,sTuner.Config.bFMiPD);
			UI_DrawTextBox(2,img_commonSet20x,"AM Audio BandW",true,bwTitle[sTuner.Config.nAMFixedLP]);
			UI_DrawTextBox(3,img_commonSet20x,"FM De-emphasis",true,deemTitle[sTuner.Config.nDeemphasis-1]);
		}
		else if(nowPage == 1)
		{
			UI_DrawCheckBox(0,img_commonSet20x,"FM FMSI",demo?true:false,sTuner.Config.bFMSI);
			UI_DrawCheckBox(1,img_commonSet20x,"FM CEQ",demo?true:false,sTuner.Config.bFMCEQ);
			UI_DrawCheckBox(2,img_commonSet20x,"FM iMS",true,sTuner.Config.bFMiMS);
			UI_DrawCheckBox(3,img_commonSet20x,"FM CNS",true,sTuner.Config.bFMCNS);
		}
		else if(nowPage == 2)
		{
			UI_DrawTextBox(0,img_commonSet20x,"Noise Blanker",true,levTitle[sTuner.Config.nNBSA[sTuner.Radio.nRFMode]]);
			UI_DrawTextBox(1,img_commonSet20x,"Soft Mute",true,levTitle[sTuner.Config.nSoftMute[sTuner.Radio.nRFMode]]);
			UI_DrawTextBox(2,img_commonSet20x,"FM Stereo",true,levTitle[sTuner.Config.nFMST]);
			UI_DrawTextBox(3,img_commonSet20x,"Dynamic Cut",true,levTitle[sTuner.Config.nDynamicCut[sTuner.Radio.nRFMode]]);
		}
	}
	
	switch(index)
	{
		case 0 :UI_DrawTextBox(0,img_commonSet20x,"Antanna Select",true,antTitle[sTuner.Config.nFMANTsel]);break;
		case 1 :UI_DrawCheckBox(1,img_commonSet20x,"FM iPD Dual Ant",demo?true:false,sTuner.Config.bFMiPD);break;
		case 2 :UI_DrawTextBox(2,img_commonSet20x,"AM Audio BandW",true,bwTitle[sTuner.Config.nAMFixedLP]);break;
		case 3 :UI_DrawTextBox(3,img_commonSet20x,"FM De-emphasis",true,deemTitle[sTuner.Config.nDeemphasis-1]);break;
		case 4 :UI_DrawCheckBox(0,img_commonSet20x,"FM FMSI",demo?true:false,sTuner.Config.bFMSI);break;
		case 5 :UI_DrawCheckBox(1,img_commonSet20x,"FM CEQ",demo?true:false,sTuner.Config.bFMCEQ);break;
		case 6 :UI_DrawCheckBox(2,img_commonSet20x,"FM iMS",true,sTuner.Config.bFMiMS);break;
		case 7 :UI_DrawCheckBox(3,img_commonSet20x,"FM CNS",true,sTuner.Config.bFMCNS);break;
		case 8 :UI_DrawTextBox(0,img_commonSet20x,"Noise Blanker",true,levTitle[sTuner.Config.nNBSA[sTuner.Radio.nRFMode]]);break;
		case 9 :UI_DrawTextBox(1,img_commonSet20x,"Soft Mute",true,levTitle[sTuner.Config.nSoftMute[sTuner.Radio.nRFMode]]);break;
		case 10:UI_DrawTextBox(2,img_commonSet20x,"FM Stereo",true,levTitle[sTuner.Config.nFMST]);break;
		case 11:UI_DrawTextBox(3,img_commonSet20x,"Dynamic Cut",true,levTitle[sTuner.Config.nDynamicCut[sTuner.Radio.nRFMode]]);break;
		default : break;
	}
	
	UI_DrawSelectBar(index);
	UI_DrawScrollBar(MENU_RADIO_INDEX, index);
	
}



void UI_Search(int8_t index, bool init)
{
	static uint8_t lastPage = 0;
	uint8_t nowPage = index/4;
	bool aa = init;
	
	if(lastPage != nowPage)
	{
		aa = true;
		lastPage = nowPage;
	}
	
	if(aa)
	{
		GUI_ClearBuff(COLOR_WHITE);
		if(nowPage == 0)
		{
			UI_DrawTextBox(0,img_commonSet20x,"FM Region",true,regionTitle[sTuner.ATS.nFMRegion]);
			UI_DrawTextBox(1,img_commonSet20x,"ATS Mode",true,atsTitle[sTuner.ATS.nATSMode]);
			UI_DrawValueBox(2,img_commonSet20x,"Threshold",true,sTuner.ATS.nSigThreshold,"dBuV");
			UI_DrawValueBox(3,img_commonSet20x,"MW Step",true,sTuner.ATS.nMWStep,"K");
		}
		else
		{
			UI_DrawCheckBox(0,img_commonSet20x,"Manage Memory",false,false);
			UI_DrawCheckBox(1,img_commonSet20x,"Start Search CH",true,false);
			GUI_Rectangle(32,48+2,160,20,1,COLOR_BLACK);
		}
	}
	
	switch(index)
	{
		case 0 :UI_DrawTextBox(0,img_commonSet20x,"FM Region",true,regionTitle[sTuner.ATS.nFMRegion]);break;
		case 1 :UI_DrawTextBox(1,img_commonSet20x,"ATS Mode",true,atsTitle[sTuner.ATS.nATSMode]);break;
		case 2 :UI_DrawValueBox(2,img_commonSet20x,"Threshold",true,sTuner.ATS.nSigThreshold,"dBuV");break;
		case 3 :UI_DrawValueBox(3,img_commonSet20x,"MW Step",true,sTuner.ATS.nMWStep,"K");break;
		case 4 :UI_DrawCheckBox(0,img_commonSet20x,"Manage Memory",false,false);break;
		case 5 :UI_DrawCheckBox(1,img_commonSet20x,"Start Search CH",true,false);break;
		default : break;
	}
	
	UI_DrawSelectBar(index);
	UI_DrawScrollBar(MENU_ATS_INDEX, index);
	
}


void UI_Device(int8_t index, bool init)
{
	static uint8_t lastPage = 0;
	uint8_t nowPage = index/4;
	bool aa = init;
	uint8_t tmp = 0;
	uint32_t tid = 0;
	
	if(lastPage != nowPage)
	{
		aa = true;
		lastPage = nowPage;
	}
	
	if(aa)
	{
		GUI_ClearBuff(COLOR_WHITE);
		if(nowPage == 0)
		{
			UI_DrawCheckBox(0,img_commonSet20x,"Mono Speaker", true, sDevice.bAutoMono);
			UI_DrawCheckBox(1,img_commonSet20x,"Demo Mode", true, sTuner.Config.bDemoMode);
			UI_DrawCheckBox(2,img_commonSet20x,"Soft Reboot", true, sDevice.bSoftReboot);
			UI_DrawCheckBox(3,img_commonSet20x,"Set Time", true, false);
		}
		else
		{
			UI_DrawCheckBox(0,img_commonSet20x,"Firmware Update", true, false);
			GUI_Text(8,24,246,96,"Unique ID:\n\nTuner ID:\n\nFirmware ID:",&Font12,COLOR_BLACK,COLOR_WHITE);
			// UID
			for(uint8_t j = 0;j<3;j++)
			{
				tid = sDevice.sInfo.uid[j];
				for(int8_t i = 0; i < 8; i++)
				{
					tmp = (tid & 0xF0000000) >> 28;
					if (tmp < 10)
						tmp = tmp + '0';
					else
						tmp = tmp - 10 + 'A';
					tid = (tid << 4) & 0xFFFFFFFF;
					GUI_Char(8+j*56+i*7,36,tmp,&Font12,COLOR_BLACK,COLOR_WHITE);
				}
			}
			
			// Tuner ID
			GUI_Number(8,60,(uint8_t)(sDevice.sInfo.tid>>24),ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
			GUI_Number(8+14,60,(uint8_t)(sDevice.sInfo.tid>>16),ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
			GUI_Number(8+14+14,60,(uint16_t)(sDevice.sInfo.tid&0xFFFF),ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
			
			// Firmware ID
			GUI_Number(8,84,sDevice.sInfo.pid[1],ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
			GUI_Char(15,84,'.',&Font12,COLOR_BLACK,COLOR_WHITE);
			GUI_Number(22,84,sDevice.sInfo.pid[2],ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
			GUI_Number(60,84,sDevice.sInfo.pid[3],ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
		}
	}
	
	switch(index)
	{
		case 0 :UI_DrawCheckBox(0,img_commonSet20x,"Mono Speaker", true, sDevice.bAutoMono);break;
		case 1 :UI_DrawCheckBox(1,img_commonSet20x,"Demo Mode", true, sTuner.Config.bDemoMode);break;
		case 2 :UI_DrawCheckBox(2,img_commonSet20x,"Soft Reboot", true, sDevice.bSoftReboot);break;
		case 3 :UI_DrawCheckBox(3,img_commonSet20x,"Set Time", true, false);break;
		case 4 :UI_DrawCheckBox(0,img_commonSet20x,"Firmware Update", true, false);break;
		default : break;
	}
	
	UI_DrawSelectBar(index);
	UI_DrawScrollBar(MENU_DEVICE_INDEX, index);
	
}

void UI_About(int8_t index, bool init)
{
	if(init)
	{
		GUI_ClearBuff(COLOR_WHITE);
		GUI_DrawBuff_Dot(187,28,41,41,MODE_GREY,COLOR_BLACK,COLOR_WHITE,img_qrcode);
		GUI_Text(4,4,180,92,"SAF7751 Radio\nBy NyaKoishi\nSupport me if you want\nThis is my QR Code  ->\nThanks!\nEmail:1194703727@qq.com\nQQ Group:616786582",&Font12,COLOR_BLACK,COLOR_WHITE);
	}
}







