#include "rds.h"
#include "stdbool.h"

#define RDS_ERR_MODE    RDS_LO

#define WORD_LOW_BYTE(a)     ((unsigned char)(a & 0x00FF))
#define WORD_HIGH_BYTE(a)    ((unsigned char)(a >> 8))

// PTY Definition [ 8 char ]
const unsigned char PTY_Table[32][9] =
{
	"PTY None",
	"News    ",
	"Affairs ",
	"Info    ",
	"Sport   ",
	"Educate ",
	"Drama   ",
	"Culture ",
	"Science ",
	"Varied  ",
	"Pop M   ",
	"Rock M  ",
	"Easy M  ",
	"Light M ",
	"Classics",
	"Other M ",
	"Weather ",
	"Finance ",
	"Children",
	"Social  ",
	"Religion",
	"Phone In",
	"Travel  ",
	"Leisure ",
	"Jazz    ",
	"Country ",
	"Nation M",
	"Oldies  ",
	"Folk M  ",
	"Document",
	"Test    ",
	"Alarm   "
};


struct RDSData* rds;
uint8_t GroupType = 0;
static uint16_t RTDataMask = 0;
static uint16_t oldpi = 0;

uint16_t FindMin(uint8_t *str, uint16_t lengh)
{
	uint16_t i = 0;
	uint16_t min = 0;
	for(i=0;i<lengh;i++)
	{
		if(str[min]>str[i])
			min = i;
	}
	return min;
}

void CopyStr(unsigned char* des, const unsigned char* source)
{
	unsigned char* r = des;
  while(*source != '\0')
	{
		*r++ = *source++;
	}
	*r = '\0';
}

void FillStr(unsigned char* str, uint16_t length, unsigned char ch)
{
	uint16_t i = 0;
	for(i=0;i<length-1;i++)
	{
		str[i] = ch;
	}
	str[i] = '\0';
}

/*
 * 0:no error
 * 1:small error
 * 2:large error
 * 3:uncorrectable error
 */
uint8_t CheckError(struct RDSBuffer *rawdata, uint8_t block)
{
	return ( (rawdata->error >> block) & 0x03 );
}

uint8_t CheckVersionType(struct RDSBuffer *rawdata)
{
	return ( (rawdata->status >> 4) & 0x01 );
}

uint8_t CheckGroupType(struct RDSBuffer *rawdata)
{
	return ( (rawdata->block_B >> 12) & 0x000F );
}

void RDS_Init(struct RDSData* init)
{
	rds = init;
	
	rds->PI = 0;
	oldpi = 0;
	FillStr(rds->PTY, 9, ' ');
	FillStr(rds->PS, 8, ' ');
	rds->PS[8] = 0;
	FillStr(rds->RT, 64, ' ');
	rds->RT[64] = 0;
	rds->Hour = 0;
	rds->Minute = 0;
	rds->RDSFlag = 0;
}

void RDS_Refresh(void)
{
	rds->PI = 0;
	oldpi = 0;
	FillStr(rds->PTY, 9, ' ');
	FillStr(rds->PS, 8, ' ');
	rds->PS[8] = 0;
	
	//FillStr(rds->RT, 64, ' ');
	//rds->RT[64] = 0;
	//rds->Hour = 0;
	//rds->Minute = 0;
	//rds->RDSFlag = 0;
}

uint16_t ReadPI(struct RDSBuffer *rawdata)
{
	uint16_t newpi = 0;
	uint8_t error[2];
	uint8_t min;
	if(CheckVersionType(rawdata) == VERSION_B)
	{
		error[0] = CheckError(rawdata, BLOCK_A);
		error[1] = CheckError(rawdata, BLOCK_C);
		min = FindMin(error, 2);
		if( error[min] <= RDS_ERR_MODE )
		{
			if(min == 0)
				newpi = rawdata->block_A;
			else
				newpi = rawdata->block_C;
		}
		else
			return 0;
	}
	else
	{
		if(CheckError(rawdata, BLOCK_A) <= RDS_ERR_MODE)
		{
			newpi = rawdata->block_A;
		}
		else
			return 0;
	}
	if(newpi != oldpi)
	{
		oldpi = newpi;
		rds->RDSFlag |= ReadyBit_PI;
	}
	return newpi;
}

uint8_t ReadPTY(struct RDSBuffer *rawdata)
{
	uint8_t pty;
	static uint8_t oldpty = 0;
	pty = ( (rawdata->block_B >> 5) & 0x001F );
	
	if(pty != 0 && oldpty != pty)
	{
		oldpty = pty;
		rds->RDSFlag |= ReadyBit_PTY;
	}
	return pty;
}

void ReadPS(struct RDSBuffer *rawdata)
{
	static unsigned char oldps[8];
	uint8_t segment = 0;
	segment = (uint8_t)(rawdata->block_B & 0x0003);
	
	if(CheckError(rawdata, BLOCK_D) <= RDS_ERR_MODE)
	{
		rds->PS[segment*2] = WORD_HIGH_BYTE(rawdata->block_D);
		rds->PS[segment*2+1] = WORD_LOW_BYTE(rawdata->block_D);
		rds->PS[8] = '\0';
	}
	for(uint8_t i = 0;i<8;i++)
	{
		if(rds->PS[i] != oldps[i])
		{
			rds->RDSFlag |= ReadyBit_PS;
			oldps[i] = rds->PS[i];
		}
	}
}

void ReadRT(struct RDSBuffer *rawdata)
{
	uint8_t segment = 0;
	static uint8_t old_ind = 0;
	
	// segment = [ 0:15 ]
	segment = (uint8_t)(rawdata->block_B & 0x000F);
	rds->RT_Indicator = (rawdata->block_B >> 4) & 0x0001; 
	
	if(old_ind != rds->RT_Indicator)
	{
		FillStr(rds->RT, 64, ' ');
		rds->RT[64] = 0;
		rds->RDSFlag &= ~ReadyBit_RT;
		RTDataMask = 0;
		old_ind = rds->RT_Indicator;
	}
	
	if(CheckVersionType(rawdata) == VERSION_A)
	{
		if(CheckError(rawdata, BLOCK_C) <= RDS_ERR_MODE && CheckError(rawdata, BLOCK_D) <= RDS_ERR_MODE)
		{
			rds->RT[segment*4] = WORD_HIGH_BYTE(rawdata->block_C);
			rds->RT[segment*4+1] = WORD_LOW_BYTE(rawdata->block_C);
			rds->RT[segment*4+2] = WORD_HIGH_BYTE(rawdata->block_D);
			rds->RT[segment*4+3] = WORD_LOW_BYTE(rawdata->block_D);
			RTDataMask |= 1<<segment;
		}
		if(RTDataMask == 0xFFFF)
		{
			rds->RDSFlag |= ReadyBit_RT;
			rds->RT[64] = '\0';
			RTDataMask = 0;
		}
		
	}
	else
	{
		if(CheckError(rawdata, BLOCK_D) <= RDS_ERR_MODE)
		{
			rds->RT[segment*2] = WORD_HIGH_BYTE(rawdata->block_D);
			rds->RT[segment*2+1] = WORD_LOW_BYTE(rawdata->block_D);
			RTDataMask |= 1<<segment;
		}
		if(RTDataMask == 0xFFFF)
		{
			rds->RDSFlag |= ReadyBit_RT;
			rds->RT[32] = '\0';
			RTDataMask = 0;
		}
	}
	
}

void ReadCT(struct RDSBuffer *rawdata)
{
	uint8_t temp = 0;
	int8_t temp_h = 0;
	int8_t offset_h = 0;
	if((CheckError(rawdata, BLOCK_C) <= RDS_ERR_MODE) && (CheckError(rawdata, BLOCK_D) <= RDS_ERR_MODE))
	{
		offset_h = (rawdata->block_D & 0x001F)/2;
		if( ((rawdata->block_D >> 5) & 0x0001) )
			offset_h = -offset_h;
		
		rds->Minute = ((rawdata->block_D >> 6) & 0x003F);
		temp = ((rawdata->block_D >> 12) & 0x000F);
		temp |= ((rawdata->block_C << 4) & 0x0010);
		
		temp_h = temp + offset_h;
		if(temp_h < 0)
			rds->Hour = temp_h+24;
		else if(temp_h >= 24)
			rds->Hour = temp_h-24;
		else
			rds->Hour = temp_h;
		
		rds->RDSFlag |= ReadyBit_CT;
	}
}

void DecodeRDS(struct RDSBuffer *rawdata)
{
	rds->PI = ReadPI(rawdata);
	
	if(CheckError(rawdata, BLOCK_B) <= RDS_ERR_MODE)
	{
//		CopyStr(rds->PTY, PTY_Table[ReadPTY(rawdata)]);
		
		GroupType = CheckGroupType(rawdata);
		
		switch(GroupType)
		{
			case 0:{  // Program Service name
				ReadPS(rawdata);
			};break;
			
			case 2:{  // Radio Text
				ReadRT(rawdata);
			};break;
			
//			case 4:{  // Clock-Time and Date
//				ReadCT(rawdata);
//			};break;
			
			default:break;
		}
	}
	
}




