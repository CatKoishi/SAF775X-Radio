#ifndef __LCD_H
#define __LCD_H

/****************************************************************/

#include "main.h"
#include "font.h"
#include "gd32f30x.h"


#define LCD_RST_PORT	GPIOC
#define LCD_RST_PIN		GPIO_PIN_7

#define LCD_DC_PORT		GPIOB
#define LCD_DC_PIN		GPIO_PIN_14

#define LCD_CS_PORT		GPIOB
#define LCD_CS_PIN		GPIO_PIN_12

#define CMD    0
#define DATA   1

// Size
#define LCD_DRV_MAX_X (256)
#define LCD_DRV_MAX_Y (96)
#define LCD_DRV_COLOUR_BIT (2)
#define LCD_DRV_COLOUR_BIT_MSK (0x03)
#define LCD_DRV_PAGE_ROW (8 / LCD_DRV_COLOUR_BIT)
#define LCD_DRV_PAGE_MAX (LCD_DRV_MAX_Y / LCD_DRV_PAGE_ROW)

#define MODE_MONO      0
#define MODE_GREY      1

#define COLOR_TRANS    0xFF    // Alpha 透明
#define COLOR_WHITE    0x00
#define COLOR_LIGHT    0x01
#define COLOR_DARK     0x02
#define COLOR_BLACK    0x03

#define DIR_V   0
#define DIR_H   1

#define ALIGNMENT_LEFT     0
#define ALIGNMENT_RIGHT    1

#define CS_PIN_LOW 		gpio_bit_reset(LCD_CS_PORT,LCD_CS_PIN)
#define CS_PIN_HIGH 	gpio_bit_set(LCD_CS_PORT,LCD_CS_PIN)
#define DC_PIN_LOW 		gpio_bit_reset(LCD_DC_PORT,LCD_DC_PIN)
#define DC_PIN_HIGH 	gpio_bit_set(LCD_DC_PORT,LCD_DC_PIN)
#define RST_PIN_LOW 	gpio_bit_reset(LCD_RST_PORT,LCD_RST_PIN)
#define RST_PIN_HIGH 	gpio_bit_set(LCD_RST_PORT,LCD_RST_PIN)

struct displayConfig
{
  uint8_t brightness;
  uint8_t greyLevel[2];
  bool invDisp;
  uint16_t backTime;
  uint16_t contrast;
  int32_t backTimeCounter;
  bool emiFree;
};

extern struct displayConfig sDisplay;

void LCD_StructInit(bool initPara);
void LCD_Init(void);
void DispFill(uint8_t page ,uint8_t column ,uint16_t x ,uint8_t y ,uint8_t color);
void DispPic(uint8_t page, uint8_t column, uint16_t x ,uint8_t y ,const unsigned char *dp);



void DispContrast(uint16_t contrast);
void DispGreyLevel(uint8_t light, uint8_t dark);
void DispInverse(bool inv);
void DispPixelOn(uint8_t mode);
void DispShutDown(void);

/****************************************************************/

void lcd_dma_init(void);
void lcd_dma_callback(void);
void lcd_update(void);



void GUI_ClearBuff(uint8_t color);
//void GUI_DrawDot(uint16_t x, uint8_t y, uint8_t color);
void GUI_FillBuff_Origin(uint16_t xs, uint8_t ys, uint16_t x ,uint8_t y, uint8_t data);
void GUI_FillBuff(uint16_t xs, uint8_t ys, uint16_t x ,uint8_t y, uint8_t color);
void GUI_DrawBuff_Origin(uint8_t xs, uint8_t ys, uint16_t x, uint8_t y, const unsigned char *dp);
void GUI_DrawBuff(uint16_t xs, uint8_t ys, uint16_t x, uint8_t y, uint8_t pic_type, uint8_t color, uint8_t bcolor, const unsigned char *dp);

void GUI_Char(uint16_t xs, uint8_t ys, char ascii, sFONT* font, uint8_t color, uint8_t bcolor);
void GUI_Text(uint16_t xs, uint8_t ys, int16_t xe, int16_t ye, const char* text, sFONT* font, uint8_t color, uint8_t bcolor);
uint8_t GUI_Number(uint16_t xs, uint8_t ys, int32_t number, uint8_t alignment, sFONT* font, uint8_t color, uint8_t bcolor);
void GUI_Float(uint16_t xs, uint8_t ys, float number, uint8_t numfrac, uint8_t alignment, sFONT* font, uint8_t color, uint8_t bcolor);

void GUI_Line_VH(uint16_t xs, uint8_t ys, uint8_t direction, uint16_t length, uint16_t line_w, uint8_t color);
void GUI_Line(uint16_t xs, uint8_t ys, uint16_t xe, uint8_t ye, uint8_t color);
void GUI_Rectangle(uint16_t xs, uint8_t ys, uint16_t length, uint8_t height, uint8_t line_w, uint8_t color);

#endif
