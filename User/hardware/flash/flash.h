#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"
#include "gd32f30x.h"


#define FLASH_CS(n)    n ? gpio_bit_set(GPIOD, GPIO_PIN_2):gpio_bit_reset(GPIOD, GPIO_PIN_2)


//指令表
#define FLASH_WriteEnable       0x06 
#define FLASH_WriteDisable      0x04 

#define FLASH_ReadStatusReg     0x05 
#define FLASH_WriteStatusReg    0x01 

#define FLASH_ReadData          0x03 
#define FLASH_FastReadData      0x0B 

#define FLASH_PageProgram       0x02 

#define FLASH_PageErase         0x81
#define FLASH_SectorErase       0x20 
#define FLASH_BlockErase32      0x52 
#define FLASH_BlockErase64      0xD8 
#define FLASH_ChipErase         0xC7 

#define FLASH_PowerDown         0xB9 
#define FLASH_ReleasePowerDown	0xAB 

#define FLASH_DeviceID          0xAB 
#define FLASH_ManufactDeviceID  0x90 
#define FLASH_JedecDeviceID     0x9F 



#define ERASE_PAGE              FLASH_PageErase
#define ERASE_SECTOR            FLASH_SectorErase
#define ERASE_BLOCK32K          FLASH_BlockErase32
#define ERASE_BLOCK64K          FLASH_BlockErase64
#define ERASE_CHIP              FLASH_ChipErase

#define SIZE_PAGE               256
#define SIZE_SECTOR             4096
#define SIZE_BLOCK32K           32768
#define SIZE_BLOCK64K           65536
#define SIZE_CHIP               2097152

#define NUM_PAGE                8192
#define NUM_SECTOR              512
#define NUM_BLOCK32K            64
#define NUM_BLOCK64K            32
#define NUM_CHIP                1



uint8_t flash_readStatueReg(void);
void flash_writeStatueReg(uint8_t sr);
void flash_writeEnable(uint8_t onoff);
void flash_power(uint8_t state);
uint16_t flash_readID(void);
void flash_read(uint32_t addr, uint8_t* buffer, uint32_t numRx);
void flash_program_page(uint32_t addr, uint8_t* data, uint16_t numTx);
void flash_program(uint32_t addr, uint8_t* data, uint32_t numTx);
uint8_t flash_erase(uint8_t type, uint16_t index);
void flash_write(uint32_t addr, uint8_t* data, uint32_t numTx);

uint16_t flash_init(void);


#endif