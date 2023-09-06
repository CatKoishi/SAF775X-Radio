#include "iic_sw.h"
#include "systick.h"


/**************************************************************************************/
static void I2C_Delay(uint8_t i)
{
	uint8_t cnt = 3;
	while(i--)
	{
		cnt = 3;
		while(cnt--);
	}
}

bool I2C_Start(uint8_t addr)
{
	I2C_SDA(0);
	I2C_Delay(4);
	I2C_SCL(0);
	return I2C_WriteByte(addr);
}

bool I2C_Restart(uint8_t addr)
{
	I2C_Delay(4);
	I2C_SDA(1);
	I2C_Delay(4);
	I2C_SCL(1);
	I2C_Delay(4);
	return I2C_Start(addr);
}

void I2C_Stop(void)
{
	I2C_SDA(0);
	I2C_Delay(4);
	I2C_SCL(1);
	I2C_Delay(4);
	I2C_SDA(1);
	I2C_Delay(4);
}

bool I2C_WriteByte(uint8_t Data)
{
	
	for (uint8_t m = 0x80; m != 0; m >>= 1)
	{
		I2C_Delay(1);
		(m & Data) ? I2C_SDA(1) : I2C_SDA(0);
		I2C_Delay(1);
		I2C_SCL(1);
		I2C_Delay(2);
		I2C_SCL(0);
	}
	I2C_Delay(1);
	I2C_SDA(1);
	I2C_Delay(1);
	uint8_t rtn = READ_SDA;
	I2C_SCL(1);
	I2C_Delay(2);
	I2C_SCL(0);
	I2C_SDA(0);
	return rtn == 0;
}

uint8_t I2C_ReadByte(bool last)
{
	uint8_t b = 0;
	I2C_SDA(1);
	for (uint8_t count = 0; count < 8; count++)
	{
		b <<= 1;
		I2C_Delay(2);
		I2C_SCL(1);
		I2C_Delay(1);
		while (!READ_SCL);
		if (READ_SDA) b |= 0x01;
		I2C_Delay(1);
		I2C_SCL(0);
	}
	(last) ? I2C_SDA(1) : I2C_SDA(0);
	I2C_Delay(2);
	I2C_SCL(1);
	I2C_Delay(2);
	I2C_SCL(0);
	I2C_SDA(0);
	return b;
}

//true -> ack
bool I2C_Check(uint8_t addr)
{
	bool state = false;
	state = I2C_Start(addr&0xFE);
	I2C_Stop();
	return state;
}

void I2C_Transmit(uint8_t addr, uint8_t *data, uint16_t size)
{
	I2C_Start(addr&0xFE);
	
	for(uint16_t i = 0;i<size;i++)
	{
		I2C_WriteByte(data[i]);
	}
	
	I2C_Stop();
}

void I2C_Receive(uint8_t addr, uint8_t *data, uint16_t size)
{
	uint16_t i = 0;
	I2C_Start(addr|0x01);
	
	for(i = 0;i<size-1;i++)
	{
		data[i] = I2C_ReadByte(0);
	}
	data[i] = I2C_ReadByte(1);
	
	I2C_Stop();
}

void I2C_ReadREG(uint8_t iaddr, uint8_t raddr, uint8_t* data, uint16_t size)
{
	uint16_t i = 0;
	I2C_Start(iaddr&0xFE);
	I2C_WriteByte(raddr);
	I2C_Restart(iaddr|0x01);
	
	for(i = 0;i<size-1;i++)
	{
		data[i] = I2C_ReadByte(0);
	}
	data[i] = I2C_ReadByte(1);
	
	I2C_Stop();
}



