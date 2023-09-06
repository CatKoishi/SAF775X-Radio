#include "gui.h"
#include "LCD19264.h"
#include "pic.h"
#include "systick.h"



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

uint8_t num2str(uint16_t num, uint8_t * str)
{
	uint16_t temp = num;
	uint8_t len = Judge_length(num);
	
	for(uint8_t i = 0;i<5;i++)
	{
		str[4-i] = temp%10;
		temp = temp/10;
	}
	return len;
}

void stroffset(uint8_t * str)
{
	for(uint8_t i = 0; i<5; i++)
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


// 需要构建主页
//GUI_Main(nBandFreq[nBandMode], nBandMode, nBandStep[nBandMode][nFreqStep[nBandMode]], nFMFilter, nAMFilter, nRSSI, bBIAS, BAT_Volt, nVolume, bSTIN, bMuted, nBandCh[nBandMode], bFilterSel, true);
// 需要刷新主页
//GUI_Main(nBandFreq[nBandMode], nBandMode, nBandStep[nBandMode][nFreqStep[nBandMode]], nFMFilter, nAMFilter, nRSSI, bBIAS, BAT_Volt, nVolume, bSTIN, bMuted, nBandCh[nBandMode], bFilterSel, false);

void GUI_Main(uint16_t freq, uint8_t band, uint16_t step, uint8_t FMfilter, uint8_t AMfilter, int8_t RSSI, bool bias, uint8_t bat, uint8_t volume, bool stereo, bool mute, uint8_t channel, bool fsel, struct RDSData *rds, bool force)
{
	static uint16_t	vfreq 		= 0;
	static uint8_t	vband 		= 0;
	static uint16_t	vstep 		= 0;
	static uint8_t	vFMfilter = 0;
	static uint8_t	vAMfilter = 0;
	static int8_t		vRSSI 		= 0;
	static bool 		vbias 		= 0;
	static uint8_t	vbat 			= 0;
	static uint8_t	vvolume 	= 0;
	static bool 		vstereo 	= 0;
	static bool 		vmute 		= 0;
	static uint8_t 	vchannel 	= 0;
	static bool 		vfsel 		= 0;
	
	static uint8_t count = 0;
	
	uint8_t ch[5] = {0,0,0,0,0};
	uint8_t len, data;
	uint16_t temp;
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispPic(5,2,17,16,STEP_BAND);
		DispPic(7,43,16,16,DB);
		DispPic(7,77,16,16,CHANNEL);
	}
	
	// 刷新频率
	if(freq != vfreq || force == true)
	{
		len = num2str(freq, ch);
		DispFill(1,46,40,32,0);			// 只需要清除前两位和小数点
		
		DispFill(5,51,102,16,0);
			
		
		// 有些频率不存在第4，5位
		if(len >= 5)
			DispPic(1,46, 20, 32, NUM32+ch[0]*80);
		if(len >= 4)
			DispPic(1,66, 20, 32, NUM32+ch[1]*80);
		// 最后3位数字一定存在
		DispPic(1, 86, 20, 32, NUM32+ch[2]*80);
		DispPic(1,114, 20, 32, NUM32+ch[3]*80);
		DispPic(1,134, 20, 32, NUM32+ch[4]*80);
		
		vfreq = freq;
	}
	
	// 刷新步进
	if(step != vstep || band != vband || force == true)
	{  // 25->19
		if(band == 3)	//FM波段步进值是实际值的1/10
		{
			switch(step)
			{
				case 1 		: DispPic(5,19,32,8,STEP+96);break;	//10khz
				case 10		: DispPic(5,19,32,8,STEP+128);break;	//100khz
			}
		}
		else
		{
			switch(step)
			{
				case 1 		: DispPic(5,19,32,8,STEP   );break;	//1khz
				case 5  	: DispPic(5,19,32,8,STEP+32);break;	//5khz
				case 9  	: DispPic(5,19,32,8,STEP+64);break;	//9khz
				case 10		: DispPic(5,19,32,8,STEP+96);break;	//10khz
				case 100 	: DispPic(5,19,32,8,STEP+128);break;	//100khz
				case 1000 : DispPic(5,19,32,8,STEP+160);break;	//1000khz
			}
		}
		vstep = step;
	}
	
	// 刷新带宽
	if(FMfilter != vFMfilter || AMfilter != vAMfilter || band != vband || force == true)
	{
		if(band == 3)	//FM
			DispPic(6,19,32,8,FILTER+(FMfilter*32));
		else
			DispPic(6,19,32,8,FILTER+(544+AMfilter*32));
		vFMfilter = FMfilter;
		vAMfilter = AMfilter;
	}
	
	// 刷新波段与频率单位
	if(band != vband || force == true)
	{
		switch(band)
		{
			case 0 :{	//LW
				DispFill(4,106,8,8,0);
				DispPic(1,1, 33, 32, BAND);
				DispPic(2,154, 39, 24, FREQ);
			};break;
			case 1 :{	//MW
				
				DispPic(1,1, 33, 32, BAND+132);
				DispPic(2,154, 39, 24, FREQ);
			};break;
			case 2 :{	//SW
				
				DispPic(1,1, 33, 32, BAND+264);
				DispPic(2,154, 39, 24, FREQ);
			};break;
			case 3 :{	//FM
				DispPic(4, 106, 8, 8, DOT32);	//FM模式的小数点
				DispPic(1,1, 33, 32, BAND+396);
				DispPic(2,154, 39, 24, FREQ+117);
			};break;
		}
		vband = band;
	}
	
	// 刷新RSSI
	if(RSSI != vRSSI || force == true)
	{
		count++;
		if(count == 5)
		{
			count = 0;
			if(RSSI<0)
			{
				data = -RSSI;
				num2str(data,ch);
				DispPic(7, 17, 8 ,16 ,MINUS);
				DispPic(7, 25, 8 ,16 ,NUM16+ch[3]*16);
				DispPic(7, 33, 8 ,16 ,NUM16+ch[4]*16);
			}
			else
			{
				data = RSSI;
				num2str(data,ch);
				if(ch[2] == 1)
					DispPic(7, 17, 8 ,16 ,NUM16+ch[2]*16);
				else
					DispFill(7 ,17 ,8 ,16 ,0);
				DispPic(7, 25, 8 ,16 ,NUM16+ch[3]*16);
				DispPic(7, 33, 8 ,16 ,NUM16+ch[4]*16);
			}
		}
		vRSSI = RSSI;
	}
	
	// 刷新立体声指示
	if(stereo != vstereo || force == true)
	{
		if(stereo)
			DispPic(7, 129, 16 ,16 ,STEREO);
		else
			DispFill(7 ,129 ,16 ,16 ,0);
		vstereo = stereo;
	}
	
	// 刷新电池电量
	if(bat != vbat || force == true)
	{
		if(bat<=34)
			DispPic(5,153,16,16,BAT_WARN);
		else
			DispPic(5,153,16,16,BAT);
		num2str(bat,ch);
		DispPic(5, 171, 8 ,16 ,NUM16+ch[3]*16);
		DispPic(6, 180, 2 ,8  ,DOT16);
		DispPic(5, 183, 8 ,16 ,NUM16+ch[4]*16);
		vbat = bat;
	}
	
	// 刷新音量
	if(volume != vvolume || force == true)
	{
		num2str(volume,ch);
		DispPic(7, 171, 8 ,16 ,NUM16+ch[3]*16);
		DispPic(7, 183, 8 ,16 ,NUM16+ch[4]*16);
		vvolume = volume;
	}
	
	// 刷新频道序号
	if(channel != vchannel || force == true)
	{
		num2str(channel,ch);
		DispPic(7,93,8,16,NUM16+ch[3]*16);
		DispPic(7,101,8,16,NUM16+ch[4]*16);
		vchannel = channel;
	}
	
	// 刷新静音标记
	if(mute != vmute || force == true)
	{
		if(mute)
			DispPic(7,153,16,16,MUTE);
		else
			DispPic(7,153,16,16,SPEAKER);
		vmute = mute;
	}
	
	// 刷新天线馈电标志
	if(bias != vbias || force == true)
	{
		if(bias)
			DispPic(7,1,16,16,BIAS);
		else
			DispPic(7,1,16,16,ANT);
		vbias = bias;
	}
	
	if(fsel != vfsel || force == true)
	{
		if(fsel)
			DispPic(6,2,17,8,BAND_SEL);
		else
			DispPic(5,2,17,16,STEP_BAND);
		vfsel = fsel;
	}
	
	// RDS
	if((rds->RDSFlag & ReadyBit_PI) != 0 || (force == true && (rds->RDSFlag & ReadyBit_PI) != 0))
	{
		rds->RDSFlag &= ~ReadyBit_PI;
		temp = rds->PI;
		for(int8_t i = 3; i >= 0; i--)
		{
			data = temp & 0x000F;
			if (data < 10)
				ch[i] = data + '0';
			else
				ch[i] = data - 10 + 'A';
			temp = temp >> 4;
		}
		ch[4] = 0;
		DispString(6,63,(const char*)ch,&Font8);
	}
	if((rds->RDSFlag & ReadyBit_PS) != 0 || (force == true && (rds->RDSFlag & ReadyBit_PS) != 0))
	{
		rds->RDSFlag &= ~ReadyBit_PS;
		DispString(6,99,(const char*)rds->PS,&Font8);
	}
	
//	if((rds->RDSFlag & ReadyBit_PTY) != 0 || (force == true && (rds->RDSFlag & ReadyBit_PTY) != 0))
//	{
//		rds->RDSFlag &= ~ReadyBit_PTY;
//		DispString(6,63,(const char*)rds->PTY,&Font8);
//	}
	
}


void GUI_Menu(uint8_t sel, bool force)
{
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1, 9, "Display", &Font8);
		DispString(2, 9, "Audio", &Font8);
		DispString(3, 9, "Tuning", &Font8);
		DispString(4, 9, "Memory", &Font8);
		DispString(5, 9, "Device", &Font8);
		DispString(6, 9, "Advance", &Font8);
		DispString(7, 9, "Auto Power Off", &Font8);
		DispString(8, 9, "About", &Font8);
	}
	DispFill(1,1,8,64,0);
	DispString(sel, 2, "*", &Font8);
}

//菜单“显示”
//GUI_Menu_Display(DispSel, nBL, nBLTime, nContrast, nBLBaseline, nLCDBias, nLCDFps, bLCDInv, true);
void GUI_Menu_Display(uint8_t sel, uint8_t lumi, uint16_t BLtime, uint8_t contrast, uint16_t BLB, uint8_t bias, uint8_t fps, bool inv, bool force)
{
	uint8_t ch[6] = {0,0,0,0,0,0};
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1, 9, "Brightness", &Font8);//%
		DispString(2, 9, "DC Baseline", &Font8);
		DispString(3, 9, "Backlight Time", &Font8);//s
		DispString(4, 9, "Contrast", &Font8);
		DispString(5, 9, "LCD Bias", &Font8);
		DispString(6, 9, "Frame Rate", &Font8);
		DispString(7, 9, "Invert Display", &Font8);
		
		DispChar(1, 179, '%', &Font8);
		DispChar(3, 179, 'S', &Font8);
		DispString(6, 167, "FPS", &Font8);//193-8-3*6
	}
	DispFill(1,1,8,64,0);
	DispString(sel, 2, "*", &Font8);
	
	num2str(lumi,ch);
	stroffset(ch);
	DispString(1,161,(const char*)ch+2,&Font8);
	
	num2str(BLB,ch);
	stroffset(ch);
	DispString(2,167,(const char*)ch+2,&Font8);
	
	num2str(BLtime/10,ch);
	stroffset(ch);
	DispString(3,167,(const char*)ch+3,&Font8);
	
	num2str(contrast,ch);
	stroffset(ch);
	DispString(4,167,(const char*)ch+2,&Font8);
	
	switch(bias)
	{
		case 0:DispString(5,167,"1/6",&Font8);break;
		case 1:DispString(5,167,"1/7",&Font8);break;
		case 2:DispString(5,167,"1/8",&Font8);break;
		case 3:DispString(5,167,"1/9",&Font8);break;
	}
	
	switch(fps)
	{
		case 0:DispString(6,149," 76",&Font8);break;
		case 1:DispString(6,149," 95",&Font8);break;
		case 2:DispString(6,149,"132",&Font8);break;
		case 3:DispString(6,149,"168",&Font8);break;
	}
	
	if(inv)
		DispString(7,167,"O N",&Font8);
	else
		DispString(7,167,"OFF",&Font8);
	
}

//菜单“音频”
//GUI_Menu_Audio(AudioSel, bFMEMS, bFMCEQ, nDeemphasis, bFMST, nAudioSource, nAudioGain, bBASS, true);
void GUI_Menu_Audio(uint8_t sel, bool mph, bool ceq, uint8_t deempahsis, uint8_t stereo, uint8_t output, int8_t gain, bool bass, bool force)
{
	uint8_t ch[6] = {0,0,0,0,0,0};
	uint16_t temp = 0;
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1, 9, "MphSuppression", &Font8);
		DispString(2, 9, "Ch Equalizer", &Font8);
		DispString(3, 9, "Deemphasis", &Font8);
		DispString(4, 9, "FM Stereo", &Font8);
		DispString(5, 9, "Audio Output", &Font8);
		DispString(6, 9, "Audio Gain", &Font8);
		DispString(7, 9, "BassMax", &Font8);
		DispString(8, 9, "SinWave Gen", &Font8);
		
		DispString(6, 173, "dB", &Font8);
	}
	DispFill(1,1,8,64,0);
	DispString(sel, 2, "*", &Font8);
	
	mph ? DispString(1, 167, "O N", &Font8):DispString(1, 167, "OFF", &Font8);
	
	ceq ? DispString(2, 167, "O N", &Font8):DispString(2, 167, "OFF", &Font8);
	
	switch(deempahsis)
	{
		case 0 : DispString(3, 161, " OFF", &Font8);break;
		case 1 : DispString(3, 161, "50us", &Font8);break;
		case 2 : DispString(3, 161, "75us", &Font8);break;
	}
	
	switch(stereo)
	{
		case 0 : DispString(4, 161, " OFF", &Font8);break;
		case 1 : DispString(4, 161, " O N", &Font8);break;
		case 2 : DispString(4, 161, "STL0", &Font8);break;
		case 3 : DispString(4, 161, "STL1", &Font8);break;
		case 4 : DispString(4, 161, "STL2", &Font8);break;
	}
	
	if(output)
		DispString(5, 137, "MPX+Mono", &Font8);
	else
		DispString(5, 137, "   Radio", &Font8);
	
	if(gain < 0)
	{
		temp = -gain;
		DispChar(6,155,'-',&Font8);
	}
	else
	{
		temp = gain;
		DispChar(6,155,' ',&Font8);
	}
	num2str(temp,ch);
	stroffset(ch);
	DispString(6,161,(const char*)ch+3,&Font8);
	
	bass ? DispString(7, 167, "O N", &Font8):DispString(7, 167, "OFF", &Font8);
	
}

//菜单“调谐”
//GUI_Menu_Tune(sel, nATSMODE, nLowSig, bBIAS, bool force);
void GUI_Menu_Tune(uint8_t sel, uint8_t atsmode, uint8_t threshold, uint16_t fmin, uint16_t fmax, bool bias, int8_t offset, uint8_t step, bool force)
{
	uint8_t ch[6] = {0,0,0,0,0,0};
	uint16_t temp = 0;
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1, 9, "ATS Mode", &Font8);
		DispString(2, 9, "ATS Threshold", &Font8);
		DispString(3, 9, "FM ATS Min", &Font8);
		DispString(4, 9, "FM ATS Max", &Font8);
		DispString(5, 9, "Antenna Bias", &Font8);
		DispString(6, 9, "Level Offset", &Font8);
		DispString(7, 9, "MW Step", &Font8);
		DispString(8, 9, "Seek CH", &Font8);
		
		DispString(2, 161, "dBuV", &Font8);
		DispString(3, 167, "MHz", &Font8);
		DispString(4, 167, "MHz", &Font8);
		DispString(6, 173, "dB", &Font8);
		DispString(8, 173, "->", &Font8);
	}
	DispFill(1,1,8,64,0);
	DispString(sel, 2, "*", &Font8);
	
	switch(atsmode)
	{
		case 0 : DispString(1, 161, "SLOW", &Font8);break;
		case 1 : DispString(1, 161, "FAST", &Font8);break;
		case 2 : DispString(1, 161, "A  F", &Font8);break;
	}
	
	num2str(threshold,ch);
	stroffset(ch);
	DispString(2,149,(const char*)ch+3,&Font8);
	
	num2str(fmin/100,ch);
	stroffset(ch);
	DispString(3,149,(const char*)ch+2,&Font8);
	
	num2str(fmax/100,ch);
	stroffset(ch);
	DispString(4,149,(const char*)ch+2,&Font8);
	
	bias ? DispString(5, 167, "O N", &Font8):DispString(5, 167, "OFF", &Font8);
	
	if(offset < 0)
	{
		temp = -offset;
		DispChar(6,155,'-',&Font8);
	}
	else
	{
		temp = offset;
		DispChar(6,155,' ',&Font8);
	}
	num2str(temp,ch);
	stroffset(ch);
	DispString(6,161,(const char*)ch+3,&Font8);
	
	if(step == 9)
		DispString(7, 167, " 9K", &Font8);
	else
		DispString(7, 167, "10K", &Font8);
	
}

//菜单“存储”
//
void GUI_Menu_Memory(uint16_t sel, uint8_t band, uint8_t ch_num, uint16_t * ch_freq, bool force)
{
	uint8_t ch[7] = {0,0,0,0,0,0,0};
	uint16_t temp = 0;
	uint8_t i,j,len;
	uint8_t ch_show = 0;
	
	uint16_t nsel = 1; 
	static uint16_t vsel = 1; 
	
	uint8_t page = 0;
	static uint8_t vpage = 0;
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		vpage = 0xFF;
	}
	
	if(sel<=32)
		page = 0;
	else if(sel<=64)
		page = 1;
	else if(sel<=96)
		page = 2;
	else if(sel<=128)
		page = 3;
	
	if(page != vpage)
	{
		DispFill(1,1,192,64,0);
		if(band == 3)    // FM
		{
			for(i=0;i<8;i++)
			{
				for(j = 0;j<4;j++)
				{
					if(ch_show+(page*32) >= ch_num)
						break;
					len = num2str(ch_freq[ch_show+(page*32)], ch);
					stroffset(ch);
					ch[5] = ch[4], ch[4] = ch[3], ch[3] = '.', ch[6] = 0;
					DispString(i+1,j*48+1+6,(const char*)ch+(5-len),&Font8);
					ch_show++;
				}
			}
		}
		else    // AM
		{
			for(i=0;i<8;i++)
			{
				for(j = 0;j<4;j++)
				{
					if(ch_show+(page*32) >= ch_num)
						break;
					len = num2str(ch_freq[ch_show+(page*32)], ch);
					stroffset(ch);
					ch[5] = 0, ch[6] = 0;
					DispString(i+1,j*48+1+6,(const char*)ch+(5-len),&Font8);
					ch_show++;
				}
			}
		}
		
		vpage = page;
	}
	
	nsel = sel-page*32;
	// wipe vsel char
	DispFill((vsel-1)/4 + 1, ((vsel-1)%4)*48+1, 6, 8, 0);
	// show nsel char
	DispChar((nsel-1)/4 + 1, ((nsel-1)%4)*48+1, '*', &Font8);
	vsel = nsel;
}

//菜单“设备”
//GUI_Menu_Device(dsp_id,dsp_hw1,dsp_hw2,dsp_sw1,dsp_sw2,mcuid,flash,sram,temperature,vref,irc40k_value,true);
void GUI_Menu_Device(uint8_t dsp_id,uint8_t dsp_hw1,uint8_t dsp_hw2,uint8_t dsp_sw1,uint8_t dsp_sw2,uint32_t *mcuid,uint16_t flash,uint16_t sram,uint16_t temperature,uint16_t vref,uint16_t irc40k,bool force)
{
	uint8_t ch[6] = {0,0,0,0,0,0};
	uint8_t str[9] = {0,0,0,0,0,0,0,0,0};
	uint8_t tmp = 0;
	uint32_t tid = 0;
	int16_t temp = 0;
	
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1, 9, "Tuner:", &Font8);
		DispString(2, 9, "HW .    SW .    ", &Font8);
		DispString(3, 9, "UID:", &Font8);
		DispString(4, 9, "Flash:   KB         SRAM:   KB", &Font8);
		DispString(5, 9, "Temperature                  C", &Font8);
		DispString(6, 9, "Int Vref                     V", &Font8);
		DispString(7, 9, "Software Version          2.14", &Font8);
		DispString(8, 9, "LSI Calibration             Hz", &Font8);
	}
	
	switch(dsp_id)
	{
		case 0 :DispString(1,51,"Not a Lithio Chip!",&Font8);break;
		case 1 :DispString(1,51,"TEF6687 Lithio FMSI",&Font8);break;
		case 3 :DispString(1,51,"TEF6689 Lithio FMSI DR",&Font8);break;
		case 9 :DispString(1,51,"TEF6688 Lithio DR",&Font8);break;
		case 14:DispString(1,51,"TEF6686 Lithio",&Font8);break;
	}
	
	//Patch Ver
	if(dsp_hw1 == 1 && dsp_sw1 == 2)
		DispString(2, 9+16*6, "P2.24", &Font8);
	else if(dsp_hw1 == 2 && dsp_sw1 == 5)
		DispString(2, 9+16*6, "P5.12", &Font8);
	else
		DispString(2, 9+16*6, "P?.??", &Font8);
	
	//DSP Ver
	num2str(dsp_hw1,ch);
	stroffset(ch);
	DispString(2,21,(const char*)ch+4,&Font8);
	num2str(dsp_hw2,ch);
	stroffset(ch);
	DispString(2,33,(const char*)ch+3,&Font8);
	
	num2str(dsp_sw1,ch);
	stroffset(ch);
	DispString(2,69,(const char*)ch+4,&Font8);
	num2str(dsp_sw2,ch);
	stroffset(ch);
	DispString(2,81,(const char*)ch+3,&Font8);
	
	//UID
	for(uint8_t j = 0;j<3;j++)
	{
		tid = mcuid[j];
		for(int8_t i = 7; i >= 0; i--)
		{
			tmp = tid & 0x0F;
			if (tmp < 10)
				str[i] = tmp + '0';
			else
				str[i] = tmp - 10 + 'A';
			tid = tid >> 4;
		}
		DispString(3,33+j*48,(const char*)str,&Font8);
	}
	
	//Flash SRAM
	num2str(flash,ch);
	stroffset(ch);
	DispString(4,45,(const char*)ch+2,&Font8);
	num2str(sram,ch);
	stroffset(ch);
	DispString(4,9+25*6,(const char*)ch+2,&Font8);
	
	//Temperature
	temp = 3575.581395-1.873637*temperature;
	if(temp<0)
	{
		temp = -temp;
		DispChar(5,9+24*6,'-',&Font8);
	}
	else
	{
		DispChar(5,9+24*6,' ',&Font8);
	}
	num2str(temp,ch);
	stroffset(ch);
	ch[1] = ch[2], ch[2] = ch[3], ch[3] = '.';
	DispString(5,9+25*6,(const char*)ch+1,&Font8);
	//Vref
	temp = 0.8056641*vref;
	num2str(temp,ch);
	stroffset(ch);
	ch[0] = ch[1], ch[1] = '.';
	DispString(6,9+24*6,(const char*)ch,&Font8);
	
	//IRC40K
	num2str(irc40k,ch);
	stroffset(ch);
	DispString(8,9+23*6,(const char*)ch,&Font8);
}

//菜单“高级”
//
void GUI_Menu_Advance(uint8_t sel, uint8_t amnb, uint8_t fmnb, uint8_t amsm, uint8_t fmsm, uint8_t amhc, uint8_t fmhc, uint8_t fmsc, uint8_t fmsb, bool force)
{
	uint8_t ch[6] = {0,0,0,0,0,0};
	uint16_t temp = 0;
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1, 9, "AM NoiseBlanker", &Font8);
		DispString(2, 9, "FM NoiseBlanker", &Font8);
		DispString(3, 9, "AM Softmute", &Font8);
		DispString(4, 9, "FM Softmute", &Font8);
		DispString(5, 9, "AM Highcut", &Font8);
		DispString(6, 9, "FM Highcut", &Font8);
		DispString(7, 9, "FM Stereo", &Font8);
		DispString(8, 9, "FM Stereo HiBlend", &Font8);
		
		DispString(1, 161, "Lv. ", &Font8);
		DispString(2, 161, "Lv. ", &Font8);
		DispString(3, 161, "Lv. ", &Font8);
		DispString(4, 161, "Lv. ", &Font8);
		DispString(5, 161, "Lv. ", &Font8);
		DispString(6, 161, "Lv. ", &Font8);
		DispString(7, 161, "Lv. ", &Font8);
		DispString(8, 161, "Lv. ", &Font8);
	}
	DispFill(1,1,8,64,0);
	DispString(sel, 2, "*", &Font8);
	
	DispChar(1, 179, amnb+'0', &Font8);
	DispChar(2, 179, fmnb+'0', &Font8);
	DispChar(3, 179, amsm+'0', &Font8);
	DispChar(4, 179, fmsm+'0', &Font8);
	DispChar(5, 179, amhc+'0', &Font8);
	DispChar(6, 179, fmhc+'0', &Font8);
	DispChar(7, 179, fmsc+'0', &Font8);
	DispChar(8, 179, fmsb+'0', &Font8);
	
}


//菜单“APO”
//
void GUI_Menu_APO(uint16_t time, bool state, bool force)
{
	uint8_t ch[5] = {0,0,0,0,0};
	uint16_t temp = 0;
	uint8_t len;
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispPic(2,144, 39, 24, MINUTE);
		DispPic(1,10,32,32,CLOCK);
	}
	
	temp = time/60;
	len = num2str(temp, ch);
	DispFill(1,84,60,32,0);
	if(len >= 3)
		DispPic(1, 84, 20, 32, NUM32+ch[2]*80);
	if(len >= 2)
		DispPic(1,104, 20, 32, NUM32+ch[3]*80);
	if(len >= 1)
		DispPic(1,124, 20, 32, NUM32+ch[4]*80);
	
	state ? DispString(5, 12, "APO    SET", &Font16):DispString(5, 12, "APO    OFF", &Font16);
	
}


//菜单“关于”
void GUI_Menu_About(uint8_t sel, bool force)
{
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(1,9,"TEF6686 DSP Radio",&Font16);
		DispString(3,9,"By :NyaKoishi",&Font16);
		DispString(5,9,"    Nyawv",&Font16);
		DispString(7,9,"What's hidden?", &Font16);
	}
	
	switch(sel)
	{
		case 0 :DispString(7, 121, "    ", &Font16);break;
		case 1 :DispString(7, 121, "<   ", &Font16);break;
		case 2 :DispString(7, 121, "<>  ", &Font16);break;
		case 3 :DispString(7, 121, "<>< ", &Font16);break;
		case 4 :DispString(7, 121, "<><>", &Font16);break;
	}
}


void GUI_Startup(void)
{
	DispContrast(10);
	DispPic(1,1,192,64,NEYO);
	for(uint8_t i = 0;i<20;i++)
	{
		DispContrast(5*i+10);
		delay_ms(50);
	}
}

void GUI_SeekALL(uint16_t fmin, uint16_t fmax, uint16_t fnow, uint8_t number, bool force)
{
	uint8_t ch[6] = {0,0,0,0,0,0};
	const unsigned char TAB[4] = {0x7E,0x42,0x42,0x7E}; 
	static uint8_t vpercent = 100;
	static uint8_t vnumber = 100;
	
	uint8_t percent = 50*(fnow-fmin)/(fmax-fmin);
	
	if(force == true)
	{
		vnumber = 100;
		DispString(8,161,"C000",&Font8);
		DispPic(8,77,2,8,TAB);
		DispPic(8,131,2,8,TAB+2);
		DispFill(8,80,50,8,0);
	}
	
	if(number != vnumber)	//展示搜索到的频道数
	{
		num2str(number, ch);
		stroffset(ch);
		DispString(8,167,(const char *)ch+2,&Font8);
		vnumber = number;
	}
	
	if(percent != vpercent)
	{
		DispFill(8 ,80 ,percent ,8 ,0x3C);
		vpercent = percent;
	}
}

void GUI_SeekDIR(int8_t dir)
{
	static uint8_t count = 0;
	if(dir == 1)
	{
		if(count == 0)
			DispString(7,77,">>  ",&Font16);
		else if(count == 4)
			DispString(7,77,"> > ",&Font16);
		else if(count == 8)
			DispString(7,77,">  >",&Font16);
	}
	else
	{
		if(count == 0)
			DispString(7,77,"  <<",&Font16);
		else if(count == 4)
			DispString(7,77," < <",&Font16);
		else if(count == 8)
			DispString(7,77,"<  <",&Font16);
	}
	count++;
	if(count == 12)
		count = 0;
}

void GUI_WaveGen(uint8_t wsel, uint8_t asel, uint8_t multi, uint16_t mode,uint16_t ch1f, int16_t ch1a, uint16_t ch2f, int16_t ch2a, bool force)
{
	uint8_t len;
	uint8_t ch[7] = {0,0,0,0,0,0,0};
	
	if(force == true)
	{
		DispFill(1,1,192,64,0);
		DispString(4,9,"Channel 1:", &Font8);
		DispString(6,9,"Channel 2:", &Font8);
		
		asel ? DispString(2,9,"Amplitude", &Font8):DispString(2,9,"Frequency", &Font8);
		
		switch(multi)
		{
			case 1  : DispString(2,9+6*16,"x1  ", &Font8);break;
			case 10 : DispString(2,9+6*16,"x10 ", &Font8);break;
			case 100: DispString(2,9+6*16,"x100", &Font8);break;
		}
		
		switch(mode)
		{
			case 0: DispString(1,9,"Left:OFF        Right:OFF    ", &Font8);break;
			case 1: DispString(1,9,"Left:CH1        Right:OFF    ", &Font8);break;
			case 2: DispString(1,9,"Left:OFF        Right:CH2    ", &Font8);break;
			case 3: DispString(1,9,"Left:CH1        Right:CH2    ", &Font8);break;
			case 4: DispString(1,9,"Left:CH1        Right:CH1    ", &Font8);break;
			case 5: DispString(1,9,"Left:CH2        Right:CH2    ", &Font8);break;
			case 6: DispString(1,9,"Left:CH1+CH2    Right:CH1+CH2", &Font8);break;
		}
	}
	
	DispFill(1,1,8,64,0);
	switch(wsel)
	{
		case 1: DispChar(1, 2, '*', &Font8);break;
		case 2: DispChar(5, 2, '*', &Font8);break;
		case 3: DispChar(7, 2, '*', &Font8);break;
	}
	
	DispString(5,9,"     Hz         -    dB", &Font8);
	DispString(7,9,"     Hz         -    dB", &Font8);
	
	len = num2str(ch1f, ch);
	stroffset(ch);
	DispString(5,9,(const char*)ch+(5-len),&Font8);
	len = num2str(ch2f, ch);
	stroffset(ch);
	DispString(7,9,(const char*)ch+(5-len),&Font8);
	
	len = num2str(-ch1a, ch);
	stroffset(ch);
	ch[5] = ch[4], ch[4] = '.', ch[6] = 0;
	DispString(5,9+17*6,(const char*)ch+(5-len),&Font8);
	len = num2str(-ch2a, ch);
	stroffset(ch);
	ch[5] = ch[4], ch[4] = '.', ch[6] = 0;
	DispString(7,9+17*6,(const char*)ch+(5-len),&Font8);
	
	
}
