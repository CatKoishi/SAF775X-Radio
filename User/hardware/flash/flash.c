#include "flash.h"
#include "systick.h"

uint8_t flash_readWriteByte(uint8_t TxData)
{
  while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_TBE));        //等待发送区空  
	spi_i2s_data_transmit(SPI2, TxData);                         //通过外设SPIx发送一个byte  数据
  while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_RBNE));       //等待接收完一个byte  
	return spi_i2s_data_receive(SPI2);                           //返回通过SPIx最近接收的数据	
}


/**
 * @brief  读取FLASH的状态寄存器
       BIT  7   6   5   4    3    2    1     0
					 SPR  RV  TB  BP2  BP1  BP0  WEL  BUSY
					 SPR:默认0,状态寄存器保护位,配合WP使用
					 TB,BP2,BP1,BP0:FLASH区域写保护设置
					 WEL:写使能锁定
					 BUSY:忙标记位(1,忙;0,空闲)
					 默认:0x00
 */
uint8_t flash_readStatueReg(void)
{
	uint8_t byte=0;   
	FLASH_CS(0);
	flash_readWriteByte(FLASH_ReadStatusReg);
	byte=flash_readWriteByte(0Xff);
	FLASH_CS(1);
	return byte;   
}

void flash_writeStatueReg(uint8_t sr)
{
	FLASH_CS(0);
	flash_readWriteByte(FLASH_WriteStatusReg);
	flash_readWriteByte(sr);
  FLASH_CS(1);
}

// onoff 1: enable write    0: disable write
void flash_writeEnable(uint8_t onoff)
{
	FLASH_CS(0);
	onoff ? flash_readWriteByte(FLASH_WriteEnable):flash_readWriteByte(FLASH_WriteDisable);
	FLASH_CS(1);
}

// state 1: power on    0: power off
void flash_power(uint8_t state)
{
	FLASH_CS(0);
	state ? flash_readWriteByte(FLASH_ReleasePowerDown):flash_readWriteByte(FLASH_PowerDown);
	FLASH_CS(1);
	delay_us(100);
}

void flash_waitBusy(uint8_t time)
{
	uint8_t temp;
	while(1)
	{
		if(time != 0)
			delay_ms(time);
		temp = (flash_readStatueReg()&0x01);
		if(temp == 0)
			break;
		delay_us(10);
	}
}

uint16_t flash_readID(void)
{
	uint16_t temp = 0;
	FLASH_CS(0);
	flash_readWriteByte(FLASH_ManufactDeviceID);
	flash_readWriteByte(0x00);
	flash_readWriteByte(0x00);
	flash_readWriteByte(0x00);
	temp |= flash_readWriteByte(0xff) << 8;
	temp |= flash_readWriteByte(0xff);
	FLASH_CS(1);
	return temp;
}

void flash_read(uint32_t addr, uint8_t* buffer, uint32_t numRx)
{
	uint32_t i;
	flash_waitBusy(0);
	FLASH_CS(0);
	flash_readWriteByte(FLASH_ReadData);
	flash_readWriteByte((uint8_t)(addr >> 16));
	flash_readWriteByte((uint8_t)(addr >> 8));
	flash_readWriteByte((uint8_t)(addr));
	for(i=0;i<numRx;i++)
	{
		buffer[i] = flash_readWriteByte(0xff);
	}
	FLASH_CS(1);
}

// numTx <= 256
void flash_program_page(uint32_t addr, uint8_t* data, uint16_t numTx)
{
	uint16_t i;
	flash_waitBusy(0);
	flash_writeEnable(1);
	FLASH_CS(0);
	flash_readWriteByte(FLASH_PageProgram);
	flash_readWriteByte((uint8_t)(addr >> 16));
	flash_readWriteByte((uint8_t)(addr >> 8));
	flash_readWriteByte((uint8_t)(addr));
	for(i=0;i<numTx;i++)
	{
		flash_readWriteByte(data[i]);
	}
	FLASH_CS(1);
}


void flash_program(uint32_t addr, uint8_t* data, uint32_t numTx)
{
	uint8_t* data_ptr = data;
	uint32_t nowaddr = addr;
	uint32_t numByte = numTx;
	uint16_t pageremain = 256-nowaddr%256;
	
	if(numByte<=pageremain)
		pageremain = numByte;
	
	while(1)
	{
		flash_program_page(nowaddr, data_ptr, pageremain);
		if(numByte == pageremain)
			break;
		else
		{
			data_ptr+=pageremain;
			nowaddr+=pageremain;
			numByte-=pageremain;
			if(numByte>256)
				pageremain = 256;
			else
				pageremain = numByte;
		}
	}
	
}


uint8_t flash_erase(uint8_t type, uint16_t index)
{
	uint32_t addr;
	
	flash_waitBusy(0);
	flash_writeEnable(1);
	FLASH_CS(0);
	flash_readWriteByte(type);
	if(type != ERASE_CHIP)
	{
		switch(type)
		{
			case ERASE_PAGE:{
				if(index >= NUM_PAGE)
					return 1;
				addr=index<<8; // *256(2^8)
			};break;
			
			case ERASE_SECTOR:{
				if(index >= NUM_SECTOR)
					return 1;
				addr=index<<12; // *4096(2^12)
			};break;
			
			case ERASE_BLOCK32K:{
				if(index >= NUM_BLOCK32K)
					return 1;
				addr=index<<15; // *4096*8(2^15)
			};break;
			
			case ERASE_BLOCK64K:{
				if(index >= NUM_BLOCK64K)
					return 1;
				addr=index<<16; // *4096*16(2^16)
			};break;
		}
		flash_readWriteByte((uint8_t)(addr >> 16));
		flash_readWriteByte((uint8_t)(addr >> 8));
		flash_readWriteByte((uint8_t)(addr));
	}
	FLASH_CS(1);
	flash_waitBusy(15);
	return 0;
}


uint8_t local_buffer[4096];
void flash_write(uint32_t addr, uint8_t* data, uint32_t numTx)
{
	uint16_t i;
	uint32_t byteleft = numTx;
	uint32_t addrnow = addr;
	
	uint16_t secleft;
	uint32_t secaddr;
	uint32_t secofst;
	
	uint8_t* ptr = data;
	
	while(byteleft)
	{
		secaddr = addrnow/4096;
		secofst = addrnow%4096;
		
		(byteleft < 4096-secofst)? (secleft = byteleft):(secleft = 4096-secofst);
		
		flash_read(secaddr*4096, local_buffer, 4096);
		for(i=0;i<secleft;i++)
		{
			local_buffer[secofst+i] = ptr[i];
		}
		// erase
		flash_erase(ERASE_SECTOR, secaddr);
		// program
		flash_program(secaddr*4096, local_buffer, 4096);
		
		byteleft-=secleft;
		addrnow +=secleft;
		ptr += secleft;
	}
}

void flash_spi_init(void)
{
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOD);
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_SPI2);
	
	delay_us(10);
	
	//PD2 -> FLASH_CS
	GPIO_BOP(GPIOD) = GPIO_PIN_2;
	gpio_init(GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	//PB3,5 -> SPI2_SCK,MOSI
	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3 | GPIO_PIN_5);
	//PB4 -> SPI2_MISO
	gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
	
	
	spi_parameter_struct sspi;
	/* deinitilize SPI and the parameters */
	spi_struct_para_init(&sspi);
	
	spi_i2s_deinit(SPI2);
	/* SPI2 parameter config */
	sspi.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
	sspi.device_mode          = SPI_MASTER;
	sspi.frame_size           = SPI_FRAMESIZE_8BIT;
	sspi.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
	sspi.nss                  = SPI_NSS_SOFT;
	sspi.prescale             = SPI_PSC_2;  //F = 60MHz(APB1) / 2(PSC) = 30MHz (Max = 33MHz)
	sspi.endian               = SPI_ENDIAN_MSB;
	spi_init(SPI2, &sspi);
	
	spi_nss_output_disable(SPI2);
	
	spi_enable(SPI2);
}

uint16_t flash_init(void)
{
	flash_spi_init();
	
	FLASH_CS(1);
	
	flash_power(1);
	
	return flash_readID();
}



