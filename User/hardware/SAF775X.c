#include "SAF775X.h"
#include "gd32f30x.h"
#include "stdarg.h"
#include "systick.h"
#include "iic_sw.h"
#include "Dirana3_ABB_E7A0.h"
#include "Dirana3_ABB_E7A1.h"
#include "Dirana3BasicDSP.h"


#define WORD_LOW_BYTE(a)     ((unsigned char)(a & 0x00FF))
#define WORD_HIGH_BYTE(a)    ((unsigned char)(a >> 8))
	
/************************************************************************************************************************************/

struct Dirana3Radio *sys;

/************************************************************************************************************************************/

const uint8_t nBandRFMode[NUM_BANDS] = 
{ RFMODE_FM, RFMODE_AM, RFMODE_AM, RFMODE_AM };

//不同波段的步进值
uint16_t nBandStep[NUM_BANDS][NUM_STEPS] = 
{
	{ 1, 10 , 0  , 0    },	//x10 KHz  FM
	{ 1,  9 , 0  , 0    },  //x1  KHz  LW
	{ 1,  9 , 100, 0    },  //x1  KHz  MW
	{ 1,  5 , 100, 1000 }   //x1  KHz  SW
};

//不同波段的步进值的数量
const uint8_t nBandStepNum[NUM_BANDS] = 
{ 2, 2, 3, 4 };

uint8_t nBandStepATS[NUM_BANDS] = 
{ 10, 1, 9, 5};

const uint16_t nFastATS[14][2] = 
{
	{2300, 2500},
	{3200, 3400},
	{3900, 4000},
	{4750, 5060},
	{5900, 6200},
	{7100, 7600},
	{9100, 10000},
	{11600, 12100},
	{13500, 13870},
	{15100, 15800},
	{17480, 17900},
	{18900, 19020},
	{21450, 21850},
	{25600, 26100}
};

const uint16_t nFMATSRegion[4][2] = {
	{8700,10800},
	{6500,7300},
	{7600,9000},
	{8750,10800},
};

const int16_t nBandFMin[NUM_BANDS] =
{ 6500, 144, 522, 2300 };

const int16_t nBandFMax[NUM_BANDS] =
{ 10800, 288, 1710, 27000 };

/************************************************************************************************************************************/

const uint8_t DSP_FIRM0_PRODUCTION[] =
{
	1,0xE8,
	0x81,0x2C,
	0
};

const uint8_t DSP_FIRM1_PRODUCTION[] =
{
	//Load Firmware Version 7.1
	25,0xE1,0x9C,0x40,0xCC,0x6B,0x8C,0x70,0xE4,0xD6,0x01,0xBB,0xDB,0x00,0x45,0x75,0x2C,0x68,0xA1,0x39,0x33,0x53,0xF5,0x4E,0x9D,0x8F,
	25,0xE1,0x2B,0xC3,0xA0,0x0D,0xC7,0x85,0xA4,0x9D,0x99,0x4D,0x5C,0x15,0x44,0x08,0x16,0x52,0x8E,0x6C,0xAA,0xF5,0xB5,0x09,0x08,0x25,
	25,0xE1,0x9A,0xBE,0xEB,0x91,0xCC,0x98,0xBC,0x4D,0x89,0x8E,0x0F,0x18,0x8C,0x3A,0xD3,0xD2,0x11,0xA7,0x49,0xE8,0x69,0x4A,0x3C,0x22,
	25,0xE1,0xA8,0x90,0xF1,0x8B,0x74,0xC3,0x97,0xA1,0xEB,0x25,0x4C,0x56,0xBC,0x29,0x46,0x85,0xA1,0x52,0x22,0x75,0x3E,0xBD,0xC6,0xFC,
	25,0xE1,0x4B,0x76,0xE7,0x85,0x74,0xBB,0x2E,0x3E,0x14,0xFE,0xAF,0xAC,0x37,0xF0,0x8A,0xA2,0x9D,0x90,0x9E,0xC3,0xC8,0xC9,0xEB,0x22,
	25,0xE1,0x6E,0x24,0x4B,0x07,0xDD,0xC5,0x43,0x36,0x0F,0x71,0x9D,0x06,0x64,0x69,0x77,0x91,0xD6,0x58,0x29,0xB4,0x41,0xFF,0xD6,0x65,
	25,0xE1,0x34,0x04,0x67,0xE1,0x77,0xC5,0x34,0xB7,0xCD,0x37,0x32,0x4E,0x56,0x9F,0x77,0xE8,0x5C,0x60,0x13,0x34,0x7E,0x44,0xD3,0xEE,
	25,0xE1,0x46,0x8F,0x9F,0x06,0xD3,0x9E,0x22,0x38,0xA0,0x62,0x28,0x45,0x26,0xC4,0x80,0x72,0xF4,0x22,0xF0,0x4B,0x45,0xD9,0xA1,0x7D,
	25,0xE1,0x54,0xD9,0x0D,0x88,0x02,0x9C,0x95,0x51,0xAE,0x97,0x91,0xA1,0xB8,0x66,0x33,0xE4,0x87,0x1D,0xC7,0xB8,0x00,0x1E,0x7B,0x30,
	25,0xE1,0x3C,0xE5,0x08,0x7F,0x39,0xC4,0x54,0x2C,0x3D,0xFF,0xAF,0xF8,0x1D,0xF5,0xDB,0x8F,0xE9,0xEE,0x9F,0x1B,0x38,0x42,0x96,0x48,
	25,0xE1,0xEB,0xFC,0x0A,0x05,0x68,0x9B,0x4E,0x9B,0xC7,0xDF,0x40,0x11,0x6E,0xFA,0x01,0x2F,0xD9,0x3E,0x60,0xEA,0xEB,0x13,0x3D,0x73,
	25,0xE1,0x66,0x8D,0x66,0x31,0xA6,0x3C,0xD8,0x9B,0x8C,0x22,0x64,0x5A,0x79,0xE5,0xFF,0x8B,0x1A,0x73,0xA7,0xBE,0xE3,0x2C,0xE6,0x9F,
	25,0xE1,0x06,0xB3,0x3E,0xC0,0x00,0xF5,0xFE,0xBC,0x22,0xF8,0xDD,0xD7,0xF3,0xAF,0x71,0x01,0xB2,0x52,0xC0,0x63,0x73,0x02,0xE1,0x60,
	25,0xE1,0x4A,0xD7,0x5C,0x81,0xF7,0x37,0xCD,0x4A,0xFA,0x60,0x16,0x0E,0x25,0xAE,0xFA,0x75,0x17,0xAD,0xA7,0x69,0xFA,0x6A,0x8E,0xEF,
	25,0xE1,0xF0,0x0F,0x4A,0x45,0x40,0x60,0x8E,0x05,0x0D,0xC5,0x79,0x70,0x7C,0xEB,0xD6,0x3D,0xC2,0xEC,0x9A,0xE1,0x3E,0x4F,0x31,0x6C,
	25,0xE1,0x07,0x6A,0xB4,0xFE,0xAB,0x73,0x97,0xC7,0x99,0x2C,0x98,0x46,0xBC,0xAA,0xCC,0xEF,0x26,0x8E,0x3A,0x7C,0x84,0x5E,0x63,0x8E,
	25,0xE1,0x97,0xB3,0x4E,0xBF,0xB6,0xA2,0x9C,0x71,0x82,0xC9,0xBC,0x02,0x7D,0xC9,0xB4,0x14,0x6C,0x6F,0x03,0x88,0xBD,0x70,0xF0,0x86,
	25,0xE1,0xA4,0xEE,0x3A,0xB5,0xF8,0x1B,0x34,0x3B,0xFF,0x99,0x7A,0x2F,0x1D,0x09,0x3B,0x77,0x31,0x63,0x5D,0x05,0x40,0xE8,0x7F,0x2E,
	25,0xE1,0x2E,0xAA,0x63,0x2F,0xC8,0x6C,0x00,0x8A,0xC9,0xCC,0x1B,0xC5,0xBC,0xD2,0xD8,0x4D,0xB6,0x1E,0x01,0xC3,0xD2,0x89,0x89,0x07,
	25,0xE1,0x5F,0xBA,0x1B,0xA0,0x75,0xC3,0x0D,0x40,0x83,0x83,0x2A,0x71,0xC3,0x60,0xB3,0x23,0xC6,0x88,0x33,0x27,0x74,0xE6,0x4A,0xFD,
	25,0xE1,0x59,0xC7,0x0D,0x1F,0xD5,0xAB,0x27,0xDD,0xA1,0x19,0xC2,0x93,0x53,0xE1,0x9F,0x7D,0x23,0xC4,0x74,0xC7,0x7B,0x7A,0x28,0xBA,
	25,0xE1,0x59,0x8F,0x8B,0xFC,0x69,0xEB,0x27,0x21,0x90,0xAA,0xBE,0xE8,0xC9,0x8A,0xFC,0xD8,0xB7,0x24,0xD5,0x4D,0x54,0x9D,0x69,0x86,
	25,0xE1,0x15,0x1E,0xC8,0x14,0x81,0x38,0xD7,0x8F,0xF1,0x61,0xD2,0xC9,0xCD,0x61,0x5A,0xC2,0xBC,0xE0,0x43,0x35,0x4D,0xA8,0x03,0xC2,
	25,0xE1,0xC7,0xF7,0xD0,0x71,0x03,0x45,0xDF,0x35,0xE6,0xF8,0x93,0x03,0x0B,0x2E,0x22,0x5E,0x24,0xA7,0x4A,0x04,0xD3,0x57,0x33,0xCF,
	25,0xE1,0x23,0xBD,0xE0,0xD8,0xAF,0x67,0x74,0x98,0x4A,0xA2,0xC9,0xF9,0x5D,0x09,0x36,0x91,0xD2,0x77,0xA6,0x98,0x22,0xF1,0x74,0xB8,	
	5,0xE2,0x00,0x14,0x00,0x20,
	4,0xEB,0x00,0x00,0x00,
	0x80,50,
	4,0xEB,0x01,0x00,0x00,
	0x80,50,
	4,0xEB,0x02,0x00,0x00,
	0x80,50,
	4,0xEB,0x03,0x00,0x00,
	0x80,50,
	4,0xEB,0x04,0x00,0x00,
	0x80,50,
	4,0xEB,0x05,0x00,0x00,
	0x80,50,
	4,0xEB,0x05,0x0F,0x01,
	0x80,50,
	4,0xEB,0x05,0x10,0x02,
	0x80,50,
	4,0xEB,0x05,0x1F,0x03,
	0x80,50,
	4,0xEB,0x05,0x50,0x04,
	0x80,50,
	4,0xEB,0x05,0x5F,0x05,
	0x80,50,
	4,0xEB,0x05,0x6E,0x06,
	0x80,50,
	4,0xEB,0x05,0x7D,0x07,
	0x80,50,
	4,0xEB,0x05,0x8C,0x08,
	0x80,50,
	4,0xEB,0x46,0x0F,0x00,
	0x80,50,
	4,0xEB,0x06,0x00,0x01,
	0x80,50,
	1,0xE8,
	0x80,100,
	0
};

const uint8_t DSP_FIRM2_PRODUCTION[] =
{
	//Load Firmware Version 8.0
	5,0xE2,0x00,0x14,0x00,0x20,
	4,0xEB,0x00,0x00,0x01,
	0x80,50,
	4,0xEB,0x01,0x00,0x01,
	0x80,50,
	4,0xEB,0x02,0x00,0x01,
	0x80,50,
	4,0xEB,0x03,0x00,0x01,
	0x80,50,
	4,0xEB,0x04,0x00,0x01,
	0x80,50,
	4,0xEB,0x05,0x00,0x09,
	0x80,50,
	4,0xEB,0x05,0x0F,0x0A,
	0x80,50,
	4,0xEB,0x05,0x10,0x0B,
	0x80,50,
	4,0xEB,0x05,0x1F,0x0C,
	0x80,50,
	4,0xEB,0x05,0x50,0x0D,
	0x80,50,
	4,0xEB,0x05,0x5F,0x0E,
	0x80,50,
	4,0xEB,0x05,0x6E,0x0F,
	0x80,50,
	4,0xEB,0x05,0x7D,0x10,
	0x80,50,
	4,0xEB,0x05,0x8C,0x11,
	0x80,50,
	4,0xEB,0x46,0x0F,0x02,
	0x80,50,
	4,0xEB,0x06,0x00,0x03,
	0x80,50,
	1,0xE8,
	0x80,100,
	0
};

const uint8_t DSP_KEYCODE_PRODUCTION[] =
{
	25,0xE1,0xF7,0x7F,0xC4,0xE9,0xE8,0xBE,0x1B,0x94,0x98,0x0F,0xE3,0x1A,0xDD,0x72,0x34,0xB3,0x91,0x0A,0x59,0x3B,0x80,0xD4,0x1D,0xDE, // Demo Keycode for Production variants
	0
};

const uint8_t DSP_INIT[] =
{
	3,0xA9,0x28,0x01,                // Audio ADCs off
	2,0x20,0x00,                     // Primary Audio Input:Primary Radio
	2,0x04,0x00,                     // Primary Radio Select Antenna 0
	2,0x64,0x04,                     // Secondary Radio Select Antenna 1
//	4,0xC0,0x02,0x11,0x0E,           // GPIO2:ANT1 ExtAGC
	4,0xC0,0x03,0x11,0x0D,           // GPIO3:ANT0 ExtAGC
//	2,0xC9,0x0A,                     // Enable INCA
	4,0x00,0x10,0x22,0x74,           // Dummy Tuning, Start Active Mode
	2,0x3F,0x00,                     // Audio Power Control:System Power;Sample Rate Freq:44.1kHz
	3,0xA9,0x32,0x00,                // Front DAC on
	3,0xA9,0x33,0x00,                // Rear DAC on
	6,0xF3,0x03,0x82,0x80,0x00,0x00, // Switch On Sample Rate Converter 0,Primary Channel
	0
};

/************************************************************************************************************************************/

void Set_REG(uint8_t addr, uint8_t* data, uint8_t size)
{
	I2C_Start(DSP_I2C_ADDR&0xFE);
	I2C_WriteByte(addr);
	
	for(uint16_t i = 0;i<size;i++)
	{
		I2C_WriteByte(data[i]);
	}
	
	I2C_Stop();
}

void Set_REGFree(int len, ...)
{
	uint8_t tmp;
	uint32_t i;
	va_list vArgs;
	va_start(vArgs, len);
	
	I2C_Start(DSP_I2C_ADDR&0xFE);
	for(i=0;i<len;i++)
	{
		tmp = va_arg(vArgs, uint32_t);
		I2C_WriteByte(tmp);
	}
	I2C_Stop();
	
	va_end(vArgs);
}


void Get_REG(uint8_t addr, uint8_t* data, uint8_t size)
{
	uint16_t i = 0;
	I2C_Start(DSP_I2C_ADDR&0xFE);
	I2C_WriteByte(addr);
	I2C_Restart(DSP_I2C_ADDR|0x01);
	
	for(i = 0;i<size-1;i++)
	{
		data[i] = I2C_ReadByte(0);
	}
	data[i] = I2C_ReadByte(1);
	
	I2C_Stop();
}

void Set_ADSP(uint32_t subaddr, uint32_t data)
{
	I2C_Start(DSP_I2C_ADDR&0xFE);
	
	I2C_WriteByte((uint8_t)(subaddr >> 16));
	I2C_WriteByte((uint8_t)(subaddr >> 8));
	I2C_WriteByte((uint8_t)subaddr);
	
	if(subaddr&0x4000)	// YMEM
	{
		I2C_WriteByte((uint8_t)(data >> 8));
		I2C_WriteByte((uint8_t)data);
	}
	else	//XMEM
	{
		I2C_WriteByte((uint8_t)(data >> 16));
		I2C_WriteByte((uint8_t)(data >> 8));
		I2C_WriteByte((uint8_t)data);
	}
	
	I2C_Stop();
}

void Set_ADSP_multi(uint32_t subaddr, uint32_t *data, uint8_t size)
{
	I2C_Start(DSP_I2C_ADDR&0xFE);
	
	I2C_WriteByte((uint8_t)(subaddr >> 16));
	I2C_WriteByte((uint8_t)(subaddr >> 8));
	I2C_WriteByte((uint8_t)subaddr);
	
	for(uint16_t i = 0;i<size;i++)
	{
		if(subaddr&0x4000)	// YMEM
		{
			I2C_WriteByte((uint8_t)(data[i] >> 8));
			I2C_WriteByte((uint8_t)data[i]);
		}
		else	//XMEM
		{
			I2C_WriteByte((uint8_t)(data[i] >> 16));
			I2C_WriteByte((uint8_t)(data[i] >> 8));
			I2C_WriteByte((uint8_t)data[i]);
		}
	}
	
	I2C_Stop();
}

uint32_t Get_ADSP(uint32_t subaddr)
{
	uint32_t data = 0;
	I2C_Start(DSP_I2C_ADDR&0xFE);
	
	I2C_WriteByte((uint8_t)(subaddr >> 16));
	I2C_WriteByte((uint8_t)(subaddr >> 8));
	I2C_WriteByte((uint8_t)subaddr);
	
	I2C_Restart(DSP_I2C_ADDR|0x01);
	
	if(subaddr&0x4000)	// YMEM
	{
		data  = (uint32_t)I2C_ReadByte(0) << 8;
		data |= (uint32_t)I2C_ReadByte(1);
	}
	else	//XMEM
	{
		data  = (uint32_t)I2C_ReadByte(0) << 16;
		data |= (uint32_t)I2C_ReadByte(0) << 8;
		data |= (uint32_t)I2C_ReadByte(1);
	}
	
	I2C_Stop();
	
	return data;
}

void WaitBusy(void)
{
	delay_ms(1);
	while(1)
	{
		if(gpio_input_bit_get(GPIOA,GPIO_PIN_15) == SET)
			break;
	}
}

/**
 * @brief  发送DSP软件数据
 * @param  data 软件数据指针
 */
void DSP_WRITE_DATA(const uint8_t* data)
{
	uint8_t len;
	uint8_t *pa = (uint8_t *)data;	//指针复制
	for (;;)
	{
		len = *pa;
		pa++;
		if (!len)  // len = 0, transmit complete
			break;
		if(len == 0xFF)  // len = 0xFF, Wait SAF775X Ready
		{
			WaitBusy();
			continue;
		}
		if (len & 0x80)  // len = 0x80, delay
		{
			delay_ms(((uint16_t)(len & 0x7f) << 8) | *(pa++));
		}
		else
		{
			I2C_Transmit(DSP_I2C_ADDR, pa, len);
			pa = pa + len;
		}
	}
}


void BootDirana3(void)
{
	gpio_bit_reset(GPIOC, GPIO_PIN_10);
	delay_ms(5);
	gpio_bit_set(GPIOC, GPIO_PIN_10);
	delay_ms(20);
	
	if(sys->Config.bDemoMode == true)
		DSP_WRITE_DATA(DSP_KEYCODE_PRODUCTION);
	
	DSP_WRITE_DATA(DSP_FIRM0_PRODUCTION);
	
	DSP_WRITE_DATA(DSP_INIT);
}

/****************************************************************/

void GetStatus(void)
{
	uint8_t rxSeq[5];
	
	Get_REG(0x00, rxSeq, 5);
	sys->Status.nQRS = (rxSeq[0]>>5)&0x07;
	sys->Status.bSTIN = (rxSeq[0]>>3)&0x01;
	sys->Status.nRSSI = ((rxSeq[1]>>1)&0x7F)-8;
	
	if(sys->Radio.nBandMode == BAND_FM)
	{
		sys->Status.nUSN = rxSeq[2];
		sys->Status.nWAM = rxSeq[3];
	}
	else
	{
		sys->Status.nACD = rxSeq[2];
	}
	sys->Status.nOffset = rxSeq[4]&0x7F;
	if(rxSeq[4]&0x80)
		sys->Status.nOffset = -sys->Status.nOffset;
	
}

void GetQRS(void)
{
	uint8_t rxSeq;
	Get_REG(0x00, &rxSeq, 1);
	sys->Status.nQRS = (rxSeq>>5)&0x07;
	sys->Status.bSTIN = (rxSeq>>3)&0x01;
}

void GetRDS(struct RDSBuffer* rds)
{
	uint8_t sts[12];
	
	if(sys->Status.nRSSI < 20 || sys->Radio.nBandMode != BAND_FM)
		return;
	
	Get_REG(0x07, sts, 1);
	
	rds->status  = sts[0];
	//BIT   7     6     5      4      3  2  1     0
	//FUNC  RDAV  DOFL  SDATA  TBGRP  /  /  SYNC  /
	if( ((rds->status & 0x80) != 0) && ((rds->status & 0x20) == 0) )
	{
		Get_REG(0x07, sts, 10);
		rds->status  = sts[0];
		rds->block_A = (((uint16_t)sts[1]) << 8) + sts[2];
		rds->block_B = (((uint16_t)sts[3]) << 8) + sts[4];
		rds->block_C = (((uint16_t)sts[5]) << 8) + sts[6];
		rds->block_D = (((uint16_t)sts[7]) << 8) + sts[8];
		rds->error   = sts[9];
		DecodeRDS(rds);
	}
	
}


/**
 * @brief  设置音量
 * @param  percent 音量大小 0~100
 */
void SetVolume(uint8_t percent)
{
	float tmp;
	if(percent > MAX_VOL)
		return;
	sys->Audio.nVolume[sys->Audio.index] = percent;
	if(sys->Audio.index == 0)  // headphone
	{
		setFader('F', 0.67*(100-percent));
	}
	else  // speaker
	{
		tmp = percent/100.0-1.0;
		setFader('R', -67.0*tmp*tmp*tmp);
	}
}

void SetMute(bool mute)
{
	if(mute == sys->Audio.bMuted)
		return;
	sys->Audio.bMuted = mute;
	
	if(sys->Audio.bMuted == true)
	{
		setMute(ADSP_MUTE_MAIN, 1);
	}
	else if(sys->Audio.bMuted == false)
	{
		setMute(ADSP_MUTE_MAIN, 0);
	}
}

void TuneFreq(uint16_t freq, uint8_t mode)
{
	uint8_t str[3] = {0,0,0};
	sys->Radio.nBandFreq[sys->Radio.nBandMode] = freq;
	
	str[0] = (mode<<4) + sys->Radio.nBandMode;
	str[1] = WORD_HIGH_BYTE(freq);
	str[2] = WORD_LOW_BYTE(freq);
	Set_REG(0x00, str, 3);
}

/**************/

void SetTuner(void)
{
	uint8_t data = 0;;
	data = (sys->Config.nBandAGC[sys->Radio.nRFMode]<<6) + sys->Config.nBandFilter[sys->Radio.nRFMode];
	Set_REG(0x03, &data, 1);
}

/**
 * @brief  设置收音FIR滤波器带宽 
 * @param  f 带宽		[0-15] -> [自动,72,89,107,124,141,159,176,193,211,228,245,262,280,297,314]
 *					        [0-15] -> [自动,2,2.2,2.4,2.7,3.0,3.3,3.6,4.0,4.4,4.9,5.4,5.9,6.6,7.2,8.0]
 */
void SetFilter(uint8_t index)
{
	if(index >= 16)
		return;
	sys->Config.nBandFilter[sys->Radio.nRFMode] = index;
	SetTuner();
}

void SetAGC(uint8_t index)
{
	if(index >= 4)
		return;
	sys->Config.nBandAGC[sys->Radio.nRFMode] = index;
	SetTuner();
}

/**************/

void SetTunerOPT(void)
{
	uint8_t data = 0;
	uint8_t data2 = 0;
	if(sys->Radio.nRFMode == RFMODE_FM)
	{
		sys->Config.bFMAGCext ? (data |= 0x80):(data &= 0x7F);
		data2 = data;
		data |= (sys->Config.nFMANTsel << 2)&0x04;
		if(data & 0x04)
			data2 &= 0xFB;
		else
			data2 |= 0x04;
		sys->Config.bFMiPD ? (data |= 0x01):(data &= 0xFE);
	}
	else
	{
		sys->Config.bAMANTtyp ? (data |= 0x80):(data &= 0x7F);
	}
	
	Set_REG(0x04, &data, 1);
	Set_REG(0x64, &data2, 1);
}

void SetExtAGC(bool on)
{
	sys->Config.bFMAGCext = on;
	//set gpio
	SetTunerOPT();
}

void SetFMAntenna(uint8_t ant)
{
	if(ant > 1)
		return;
	sys->Config.nFMANTsel = ant;
	SetTunerOPT();
}

void SetFMiPhaseDiversity(bool on)
{
	if(sys->Config.bDemoMode == false)
		return;
	sys->Config.bFMiPD = on;
	SetTunerOPT();
}

void SetAMAntenna(uint8_t ant_type)
{
	if(ant_type > 1)
		return;
	sys->Config.bAMANTtyp = ant_type;
	// go to idle/standby
	SetTunerOPT();
	// return normal state
}

/**************/

void SetRadioDSP(void)
{
	uint8_t data = 0;
	
	if(sys->Radio.nRFMode == RFMODE_FM)
	{
		sys->Config.bFMiMS ? (data |= 0x80):(data &= 0x7F);
		sys->Config.bFMCNS ? (data |= 0x40):(data &= 0xBF);
		sys->Config.bFMCEQ ? (data |= 0x20):(data &= 0xDF);
		data |= (sys->Config.nNBSA[sys->Radio.nRFMode]<<2)&0x0C;
		data |= 0x02;
	}
	else
	{
		data |= 0x40;
		data |= (sys->Config.nNBSA[sys->Radio.nRFMode]<<2)&0x0C;
		data |= (sys->Config.nNBSB)&0x03;
	}
	
	Set_REG(0x05, &data, 1);
}

void SetFMiMultipathSuppression(bool on)
{
	sys->Config.bFMiMS = on;
	SetRadioDSP();
}

void SetFMClickNoiseSuppression(bool on)
{
	sys->Config.bFMCNS = on;
	SetRadioDSP();
}

void SetFMChannelEqualizer(bool on)
{
	if(sys->Config.bDemoMode == false)
		return;
	sys->Config.bFMCEQ = on;
	SetRadioDSP();
}

void SetNoiseBlanker(uint8_t rfmode, uint8_t sens)
{
	if(sens >= 4)
		return;
	sys->Config.nNBSA[rfmode] = sens;
	SetRadioDSP();
}

void SetNoiseBlankerB(uint8_t sens)
{
	if(sens >= 4)
		return;
	sys->Config.nNBSB = sens;
	SetRadioDSP();
}

/**************/

void SetRadioSignal(void)
{
	uint8_t data = 0;
	if(sys->Radio.nRFMode == RFMODE_FM)
	{
		if(sys->Config.nFMST == 0)
			data |= 1<<7;
		if(sys->Config.bFMSI == true)
			data |= 1<<5;
		
		data |= sys->Config.nDeemphasis;
	}
	else
	{
		data |= (sys->Config.nAMFixedHP<<2) + sys->Config.nAMFixedLP;
	}
	Set_REG(0x06, &data, 1);
}

void SetFMStereoImprovement(bool on)
{
	if(sys->Config.bDemoMode == false)
		return;
	sys->Config.bFMSI = on;
	SetRadioSignal();
}

void SetFMDeemphasis(uint8_t tao)
{
	sys->Config.nDeemphasis = tao;
	SetRadioSignal();
}

void SetAMFixedHP(uint8_t hp)
{
	sys->Config.nAMFixedHP = hp;
	SetRadioSignal();
}

void SetAMFixedLP(uint8_t lp)
{
	sys->Config.nAMFixedLP = lp;
	SetRadioSignal();
}

/**************/

void SetRadioAutoBW(uint8_t sens, uint8_t lev)
{
	uint8_t data = 0;
	if(sys->Radio.nRFMode != RFMODE_FM)
		return;
	
	data |= (sens<<2)+lev;
	Set_REG(0x07, &data, 1);
}

/**************/

void SetFMStereo(uint8_t level)
{
	sys->Config.bFMSI = level;
	SetRadioSignal();
	// Stereo Blend
	// HiBlend
}

/**************/

void SwitchBand(uint8_t band)
{
	if(band >= NUM_BANDS)
		return;
	sys->Radio.nBandMode = band;
	sys->Radio.nRFMode = nBandRFMode[band];
	
	SetMute(true);
	TuneFreq(sys->Radio.nBandFreq[sys->Radio.nBandMode], Preset);
	SetTuner();
	SetTunerOPT();
	SetRadioDSP();
	SetRadioSignal();
	SetRadioAutoBW(1,1);
	SetMute(false);
}


void TunerStructInit(struct Dirana3Radio* init, bool initPara)
{
	sys = init;
	if(initPara)
	{
		sys->Config.bDemoMode = true;
		
		sys->Config.nBandAGC[0] = 0;
		sys->Config.nBandAGC[1] = 0;
		sys->Config.nBandFilter[0] = 0;
		sys->Config.nBandFilter[1] = 0;
		sys->Config.bFMAGCext = 0;
		sys->Config.nFMANTsel = 0;
		sys->Config.bFMiPD = 0;
		sys->Config.bFMiMS = 1;
		sys->Config.bFMCNS = 1;
		sys->Config.bFMCEQ = 1;
		sys->Config.bFMSI = 1;
		sys->Config.nNBSA[RFMODE_FM] = 2;
		sys->Config.nNBSA[RFMODE_AM] = 2;
		sys->Config.nNBSB = 2;
		sys->Config.nFMST = 1;
		sys->Config.nDeemphasis = 1;
		
		sys->Config.bAMANTtyp = 0;
		sys->Config.nAMFixedHP = 1;
		sys->Config.nAMFixedLP = 0;
		
		sys->Radio.nBandMode = BAND_FM;
		sys->Radio.nRFMode = RFMODE_FM;
		sys->Radio.nBandFreq[0] = 9160;
		sys->Radio.nBandFreq[1] = 210;
		sys->Radio.nBandFreq[2] = 639;
		sys->Radio.nBandFreq[3] = 9400;
		sys->Radio.nFreqStep[0] = 1;
		sys->Radio.nFreqStep[1] = 1;
		sys->Radio.nFreqStep[2] = 1;
		sys->Radio.nFreqStep[3] = 1;
		
		sys->Audio.bMuted = false;
		sys->Audio.nVolume[0] = 20;
		sys->Audio.nVolume[1] = 80;
		sys->Audio.index = 0;
		
		sys->ATS.nATSMode = 1;
		sys->ATS.nFMRegion = 0;
		sys->ATS.nSigThreshold = 23;
		sys->ATS.nMWStep = 9;
	}
}

/**
 * @brief  SAF7751初始化
 */
void TunerInit(void)
{
	BootDirana3();
	
	SetTuner();
	SetTunerOPT();
	SetRadioDSP();
	SetRadioSignal();
	SetRadioAutoBW(1,1);
	
	TuneFreq(sys->Radio.nBandFreq[sys->Radio.nBandMode], Preset);
}

