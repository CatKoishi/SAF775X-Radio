#ifndef __TUNERADAPT_H
#define __TUNERADAPT_H

#include "gd32f30x.h"
#include "stdbool.h"

//====================I2C===================
#define I2C_SCL(n) ( n ? (GPIO_BOP(GPIOB) = GPIO_PIN_6) : (GPIO_BC(GPIOB) = GPIO_PIN_6))
#define I2C_SDA(n) ( n ? (GPIO_BOP(GPIOB) = GPIO_PIN_7) : (GPIO_BC(GPIOB) = GPIO_PIN_7))
#define READ_SCL	gpio_input_bit_get(GPIOB,GPIO_PIN_6)
#define READ_SDA	gpio_input_bit_get(GPIOB,GPIO_PIN_7)
//==========================================

void I2C_Init(void);
bool I2C_Start(uint8_t addr);
bool I2C_Restart(uint8_t addr);
void I2C_Stop(void);
bool I2C_WriteByte(uint8_t Data);
uint8_t I2C_ReadByte(bool last);

bool I2C_Check(uint8_t addr);
void I2C_Transmit(uint8_t addr, uint8_t *data, uint16_t size);
void I2C_Receive(uint8_t addr, uint8_t *data, uint16_t size);
void I2C_ReadREG(uint8_t iaddr, uint8_t raddr, uint8_t* data, uint16_t size);
#endif
