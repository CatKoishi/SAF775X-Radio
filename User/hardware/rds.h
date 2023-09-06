#include "stdint.h"

#define BLOCK_A    6
#define BLOCK_B    4
#define BLOCK_C    2
#define BLOCK_D    0

#define VERSION_A    0
#define VERSION_B    1

#define ReadyBit_PI    1
#define ReadyBit_PTY   2
#define ReadyBit_PS    4
#define ReadyBit_RT    8
#define ReadyBit_CT    16
#define ReadyBit_HB    128

#define RDS_LO    0
#define RDS_NORM  1
#define RDS_DX    2

struct RDSBuffer
{
  uint8_t status;
  uint16_t block_A;
	uint16_t block_B;
	uint16_t block_C;
	uint16_t block_D;
	uint16_t error;
};

struct RDSData
{
	uint8_t RDSFlag;
	uint16_t PI;	   // Program Identification
	unsigned char PTY[9];    // Program Type
	unsigned char PS[9];    // Program Service name  [ 0A/B ]
	unsigned char RT[65];    // Radio Text  [ 2A/B ]
	uint8_t RT_Indicator;    // 0->A, 1->B
	uint8_t Hour;
	uint8_t Minute;    // Clock-Time and Date  [ 4A ]
};


void RDS_Init(struct RDSData* init);
void RDS_Refresh(void);
void DecodeRDS(struct RDSBuffer *rawdata);




