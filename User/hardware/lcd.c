/****************************************************************/
// JLX25696
// 串行接口
// 驱动IC : ST75256
// 水平显示，256x96像素
// 共有9 6行，4行为一个page，共有24个page，写入页数据时，由下到上写入，及最下面的像素对应数据最低位。
// 共有256列，即256column，写入顺序由左到右，写完最后一列不会自动换行。
//
// 取图像模使用img2lcd，数据水平，字节垂直，不包含图像头数据
// 取字模使用Pctolcd2002，阴码，列行式
/****************************************************************/

// 6.56ms / update
// 每次同步LCD显存需要6.56ms，最大同步速率150FPS (PSC = 8, F = 7.5MHz)
// 每30/1000秒同步一次时，总线利用率(1-78.14%)

#include "lcd.h"
#include "stdint.h"
#include "gd32f30x.h"
#include "systick.h"

#include "stdarg.h"

//#include "pic.h"

const uint8_t color_page[4] = {
	0x00, 0x55, 0xAA, 0xFF
};

/****************************************************************/

static uint8_t LCD_Buff[LCD_DRV_PAGE_MAX][LCD_DRV_MAX_X] = {0};
static uint8_t LCD_Buff_CPY[LCD_DRV_PAGE_MAX*LCD_DRV_MAX_X] = {0};

static uint8_t dma_trans_flag = 0;

/**********************Normal SPI Drv****************************/

void lcd_write_cmd(uint8_t cmd, int len, ...)
{
	uint8_t temp = 0;
	va_list vArgs;
	va_start(vArgs, len);
	
	CS_PIN_LOW;
	while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
	gpio_bit_write(LCD_DC_PORT, LCD_DC_PIN, CMD);
	spi_i2s_data_transmit(SPI1, cmd);
	while(SET == spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS));
	
	if(len != 0)
	{
		gpio_bit_write(LCD_DC_PORT, LCD_DC_PIN, DATA);
		
		for (uint8_t i = 0; i < len; i++)
		{
			temp = va_arg(vArgs, uint32_t);
			while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
			spi_i2s_data_transmit(SPI1, temp);
		}
		while(SET == spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS));
	}
	
	CS_PIN_HIGH;
	va_end(vArgs);
	
}

/**
 * @brief  写多个数据
 * @param  data 数据指针
 * @param  length 数据长度
 */
void lcd_write_multi(uint8_t * data, uint16_t length)
{
	uint16_t i = 0;
	CS_PIN_LOW;
	while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
	DC_PIN_HIGH;
	for(i = 0;i<length;i++)
	{
		while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
		spi_i2s_data_transmit(SPI1, data[i]);
	}
	while(SET == spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS));
	CS_PIN_HIGH;
}


void lcd_write_multi_repeat(uint8_t data, uint16_t length)
{
	uint16_t i = 0;
	CS_PIN_LOW;
	while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
	DC_PIN_HIGH;
	for(i = 0;i<length;i++)
	{
		while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
		spi_i2s_data_transmit(SPI1, data);
	}
	while(SET == spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS));
	CS_PIN_HIGH;
}


void lcd_set_cursor(uint8_t x, uint8_t page)
{
	lcd_write_cmd(0x75, 2, page+16, 0X17+16);
	
	lcd_write_cmd(0x15, 2, x, 0xff);
}


void lcd_set_window(uint8_t xs, uint8_t pages, uint16_t lx, uint8_t lpage)
{
	lcd_write_cmd(0x75, 2, pages+16, pages+lpage-1+16);
	
	lcd_write_cmd(0x15, 2, xs, xs+lx-1);
}

/**********************DMA SPI Drv****************************/

// SPI1_TX -> DMA0_CH4 <- TIM0_UP
void lcd_dma_init(void)
{
	rcu_periph_clock_enable(RCU_DMA0);
	
	dma_parameter_struct dma_init_struct;
	dma_struct_para_init(&dma_init_struct);

	/* configure SPI1 transmit DMA: DMA0_CH4 */
	dma_deinit(DMA0, DMA_CH4);
	dma_init_struct.periph_addr  = (uint32_t)&SPI_DATA(SPI1);
	dma_init_struct.memory_addr  = (uint32_t)LCD_Buff;
	dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority     = DMA_PRIORITY_LOW;
	dma_init_struct.number       = LCD_DRV_PAGE_MAX*LCD_DRV_MAX_X;
	dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
	dma_init(DMA0, DMA_CH4, &dma_init_struct);
	/* configure DMA mode */
	
	nvic_irq_enable(DMA0_Channel4_IRQn, 1U, 0U);
	dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_G);
	dma_interrupt_enable(DMA0, DMA_CH4, DMA_INT_FTF);
	
	dma_circulation_disable(DMA0, DMA_CH4);
	dma_memory_to_memory_disable(DMA0, DMA_CH4);
	
}

void lcd_update(int16_t xs, int16_t ys, uint16_t lx, uint16_t ly)
{
	if(dma_trans_flag == 1)
		return;
	
	if(xs < 0 || ys < 0)  // regular full screen update
	{
		lcd_set_cursor(0, 0);
		lcd_write_cmd(0x5c, 0);
		
		dma_channel_disable(DMA0, DMA_CH4);
		dma_transfer_number_config(DMA0, DMA_CH4, LCD_DRV_PAGE_MAX*LCD_DRV_MAX_X);
		dma_memory_address_config(DMA0, DMA_CH4, (uint32_t)LCD_Buff);
		dma_channel_enable(DMA0, DMA_CH4);
	}
	else  // part update
	{
		int16_t pgs, pge, lpg;
		int16_t i, j, k;
		
		pgs = ys/4;
		pge = (ys+ly-1)/4;
		lpg = pge-pgs+1;
		
		// memory copy
		k = 0;
		for(i = pgs; i <= pge; i++)
		{
			for(j = xs; j <= xs+lx-1; j++)
			{
				LCD_Buff_CPY[k] = LCD_Buff[i][j];
				k++;
			}
		}
		
		lcd_set_window(xs, pgs, lx, lpg);
		lcd_write_cmd(0x5c, 0);
		
		dma_channel_disable(DMA0, DMA_CH4);
		dma_transfer_number_config(DMA0, DMA_CH4, lpg*lx);
		dma_memory_address_config(DMA0, DMA_CH4, (uint32_t)LCD_Buff_CPY);
		dma_channel_enable(DMA0, DMA_CH4);
	}
	CS_PIN_LOW;
	DC_PIN_HIGH;
	spi_dma_enable(SPI1, SPI_DMA_TRANSMIT);
	dma_trans_flag = 1;
}

void lcd_dma_callback(void)
{
	if(dma_trans_flag == 1)
	{
		while(SET == spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS));
		CS_PIN_HIGH;
		dma_trans_flag = 0;
	}
}


uint8_t lcd_get_busy(void)
{
	return dma_trans_flag;
}

/**********************DMA SPI APP Func****************************/

void GUI_ClearBuff(uint8_t color)
{
	uint16_t i, j;
	
	for(i=0;i<LCD_DRV_PAGE_MAX;i++)
	{
		for(j=0;j<LCD_DRV_MAX_X;j++)
		{
			LCD_Buff[i][j] = color_page[color];
		}
	}
}

void GUI_DrawDot(uint16_t x, uint8_t y, uint8_t color)
{
	uint8_t page = y/4;
	uint8_t left = (3-y%4)*2;
	
	LCD_Buff[page][x] &= ~(0x03<<left);
	LCD_Buff[page][x] |= (color<<left);
}

void GUI_FillBuff_Origin(uint16_t xs, uint8_t ys, uint16_t x ,uint8_t y, uint8_t data)
{
	uint16_t i,j;
	uint16_t ps, p;
	ps = ys/4;
	p = y/4;
	
	for(j=ps;j<ps+p;j++)
	{
		for(i=xs;i<xs+x;i++)
		{
			LCD_Buff[j][i] = data;
		}
	}
	
}

void GUI_FillBuff(uint16_t xs, uint8_t ys, uint16_t x ,uint8_t y, uint8_t color)
{
	uint16_t i, j;
	
	uint16_t yleft = y;
	uint16_t pageleft;
	uint16_t ynow = ys;
	
	uint16_t yoff = ynow%4;
	uint8_t pagenow = ynow/4;
	
	uint8_t mix, mixMask, tmp;
	
	while(yleft)
	{
		(yleft<4-yoff)? (pageleft = yleft):(pageleft = 4-yoff);
		
		mix = 0;
		mixMask = 0;
		if(pageleft == 4)
		{
			mix = color_page[color];
			for(i=xs;i<x+xs;i++)
			{
				LCD_Buff[pagenow][i] = mix;
			}
		}
		else
		{
			for(i=0;i<pageleft;i++)
			{
				tmp = 2*(3-i-yoff);
				mix |= color << tmp;
				mixMask |= 0x03 << tmp;
			}
			for(i=xs;i<x+xs;i++)
			{
				LCD_Buff[pagenow][i] &= ~mixMask;
				LCD_Buff[pagenow][i] |=  mix;
			}
		}
		
		yleft-=pageleft;
		ynow+=pageleft;
		yoff = ynow%4;
		pagenow = ynow/4;
	}
	
	
}

void GUI_DrawBuff_Origin(uint8_t xs, uint8_t ys, uint16_t x, uint8_t y, const unsigned char *dp)
{
	uint16_t i, j;
	uint16_t ps, p;
	uint16_t count = 0;
	ps = ys/4;
	p = y/4;
	
	for(i=ps;i<ps+p;i++)
	{
		for(j=xs;j<xs+x;j++)
		{
			LCD_Buff[i][j] = dp[count];
			count++;
		}
	}
	
}

//                          {mono, 4grey}
const uint8_t byte2dot[2] = {8, 4};
void GUI_DrawBuff_Dot(uint16_t xs, uint8_t ys, uint16_t x, uint8_t y, uint8_t pic_type, uint8_t color, uint8_t bcolor, const unsigned char *dp)
{
	uint16_t i, j, k;
	
	uint8_t buf;
	uint16_t count = 0;
	
	uint8_t page = y/byte2dot[pic_type] + 1;
	uint8_t tmp, yleft, yadd;
	
	yadd = 0;
	for(k=0;k<page;k++)
	{
		tmp = y-k*byte2dot[pic_type];
		if(tmp >= byte2dot[pic_type])
			yleft = byte2dot[pic_type];
		else
			yleft = tmp;
		
		for(i=0;i<x;i++)
		{
			buf = dp[count];
			for(j=0;j<yleft;j++)
			{
				if(pic_type == MODE_MONO)
				{
					GUI_DrawDot(xs+i, ys+j+yadd, (buf&0x80)? color:bcolor);
					buf = buf<<1;
				}
				else
				{
					GUI_DrawDot(xs+i, ys+j+yadd, buf>>6);
					buf = buf<<2;
				}
			}
			count++;
		}
		yadd+=byte2dot[pic_type];
	}
	
}


void GUI_Char(uint16_t xs, uint8_t ys, char ascii, sFONT* font, uint8_t color, uint8_t bcolor)
{
	char stdch = ascii;
	if(stdch < 32 || stdch > 126)
		stdch = '?';
	uint8_t tmp;
	
	(font->Height%8 == 0)? (tmp = font->Height/8):(tmp = font->Height/8 + 1);
	uint16_t char_offset = (stdch - ' ') * tmp * font->Width;
	const unsigned char* ptr = &font->table[char_offset];
	
	GUI_DrawBuff_Dot(xs, ys, font->Width, font->Height, MODE_MONO, color, bcolor, ptr);
	
}

void GUI_Text(uint16_t xs, uint8_t ys, int16_t xe, int16_t ye, const char* text, sFONT* font, uint8_t color, uint8_t bcolor)
{
	const char* p_text = text;
	uint16_t x = xs;
	uint8_t y = ys;
	uint16_t endx = xe;
	uint16_t endy = ye;
	if(xe < 0 || xe > LCD_DRV_MAX_X)
		endx = LCD_DRV_MAX_X;
	if(ye < 0 || ye > LCD_DRV_MAX_Y)
		endy = LCD_DRV_MAX_Y;
	
	while(*p_text != 0)
	{
		if(*p_text == 10)  // \n
		{
			x = xs;
			y+= font->Height;
			if(y+font->Height > endy)
				break;
			p_text++;
			continue;
		}
		
		GUI_Char(x, y, *p_text, font, color, bcolor);
		x+=font->Width;
		if(x+font->Width > endx)
		{
			x = xs;
			y+= font->Height;
			if(y+font->Height > endy)
				break;
			
			if(*(p_text+1) == ' ')  // skip space when warpping
				p_text++;
		}
		p_text++;
	}
}

void GUI_Text_Part(uint16_t xs, uint8_t ys, const char* text, sFONT* font, uint8_t color, uint8_t start, uint8_t end)
{
	
}

// INT32 max 10位+1符号
uint8_t GUI_Number(uint16_t xs, uint8_t ys, int32_t number, uint8_t alignment, sFONT* font, uint8_t color, uint8_t bcolor)
{
	uint8_t digit = 0;
	uint8_t i;
	char str[12];
	
	int32_t temp = number;
	if(temp < 0)
		temp = -temp;
	
	do{
		str[digit] = temp%10;
		temp/=10;
		digit++;
	}while(temp);
	
	if(number < 0)
	{
		str[digit] = '-';
		digit++;
	}
	
	if(alignment == ALIGNMENT_LEFT)
	{
		for(i=0;i<digit;i++)
		{
			if(str[digit-1-i] == '-')
				GUI_Char(xs,ys,str[digit-1-i],font,color,bcolor);
			else
				GUI_Char(xs,ys,'0'+str[digit-1-i],font,color,bcolor);
			xs+=font->Width;
		}
	}
	else
	{
		for(i=0;i<digit;i++)
		{
			if(str[i] == '-')
				GUI_Char(xs-font->Width,ys,str[i],font,color,bcolor);
			else
				GUI_Char(xs-font->Width,ys,'0'+str[i],font,color,bcolor);
			xs-=font->Width;
		}
	}
	
	return digit;
}

const uint32_t fraction_lut[6] = { 0,10,100,1000,10000,100000 };
// INT32 max 10位+1符号
void GUI_Float(uint16_t xs, uint8_t ys, float number, uint8_t numfrac, uint8_t alignment, sFONT* font, uint8_t color, uint8_t bcolor)
{
	uint8_t digit = 0;
	
	int32_t integer = number;
	int32_t fraction = (number - integer)*fraction_lut[numfrac];
	
	if(fraction < 0)
		fraction = -fraction;
	
	if(alignment == ALIGNMENT_RIGHT)
	{
		GUI_Number(xs,ys,fraction,ALIGNMENT_RIGHT,font,color,bcolor);
		GUI_Char(xs-(numfrac+1)*font->Width,ys,'.',font,color,bcolor);
		GUI_Number(xs-(numfrac+1)*font->Width,ys,integer,ALIGNMENT_RIGHT,font,color,bcolor);
	}
	else
	{
		digit = GUI_Number(xs,ys,fraction,ALIGNMENT_LEFT,font,color,bcolor);
		GUI_Char(xs+digit*font->Width,ys,'.',font,color,bcolor);
		GUI_Number(xs+(digit+1)*font->Width,ys,integer,ALIGNMENT_LEFT,font,color,bcolor);
	}
}

void GUI_Line_VH(uint16_t xs, uint8_t ys, uint8_t direction, uint16_t length, uint16_t line_w, uint8_t color)
{
	// guard
	if(direction == DIR_H)
	{
		if(xs+length > LCD_DRV_MAX_X)
			length = LCD_DRV_MAX_X - xs;
		if(ys+line_w > LCD_DRV_MAX_Y)
			line_w = LCD_DRV_MAX_Y - ys;
		GUI_FillBuff(xs, ys, length, line_w, color);
	}
	else
	{
		if(ys+length > LCD_DRV_MAX_Y)
			length = LCD_DRV_MAX_Y - ys;
		if(xs+line_w > LCD_DRV_MAX_X)
			line_w = LCD_DRV_MAX_X - xs;
		GUI_FillBuff(xs, ys, line_w, length, color);
	}
}

void GUI_Line(uint16_t xs, uint8_t ys, uint16_t xe, uint8_t ye, uint8_t color)
{
	float k = 0;
	int16_t x, y;
	uint16_t count;
	uint16_t x1,x2,y1,y2;
	
	if(xe==xs)
	{
		if(ye<ys)
			y1=ye,y2=ys;
		else
			y1=ys,y2=ye;
		GUI_Line_VH(xs,y1,DIR_V,y2-y1+1,1,color);
		return;
	}
	
	if(xe<xs)
		x1=xe,x2=xs,y1=ye,y2=ys;
	else
		x1=xs,x2=xe,y1=ys,y2=ye;
	
	k = ((float)(y2-y1))/((float)(x2-x1));
	count = x2-x1+1;
	
	for(x=0;x<count;x++)
	{
		y=k*x;
		// round?
		GUI_DrawDot(x1+x, y1+y, color);
	}
	
}

void GUI_Rectangle(uint16_t xs, uint8_t ys, uint16_t length, uint8_t height, uint8_t line_w, uint8_t color)
{
	GUI_Line_VH(xs, ys, DIR_H, length, line_w, color);
	
	GUI_Line_VH(xs, ys+height, DIR_H, length, line_w, color);
	
	GUI_Line_VH(xs, ys, DIR_V, height, line_w, color);
	
	GUI_Line_VH(xs+length, ys, DIR_V, height+line_w, line_w, color);
}

/**********************Normal SPI APP Func****************************/

/**
 * @brief  填充屏幕（高度需为4的倍数）
 * @param  page 起始页
 * @param  column 起始列
 * @param  x 长度
 * @param  y 高度
 * @param  color 颜色 0:白色
 */
void DispFill(uint8_t page ,uint8_t column ,uint16_t x ,uint8_t y ,uint8_t color)
{
	uint16_t length, height;
	length = x;
	height = (uint8_t)(y/4);
	
	lcd_set_window(column, page, x, height);
	
	lcd_write_cmd(0x5c, 0);
	lcd_write_multi_repeat(color, length*height);
}


/**
 * @brief  写入任意大小的图像（高度需为8的倍数）
 * @param  page 页
 * @param  column 列
 * @param  x 长度
 * @param  y 高度
 * @param  *dp 图像指针
 */
void DispPic(uint8_t page, uint8_t column, uint16_t x ,uint8_t y ,const unsigned char *dp)
{
	uint16_t length, height;
	length = x;
	height = (uint8_t)(y/4);
	
	lcd_set_window(column, page, x, height);
	lcd_write_cmd(0x5c, 0);
	lcd_write_multi((uint8_t *)dp, length*height);
}


///**
// * @brief  显示ASCII字符串的一部分
// * @param  page 页
// * @param  column 列
// * @param  text 字符串
// * @param  font 字体
// * @param  start 起始字符位号
// * @param  length 显示长度
// */
//void DispStringPart(uint8_t page, uint8_t column, const char* text, sFONT* font, uint16_t start, uint16_t length)
//{
//	const char* p_text = text;
//	uint8_t x = column;
//	uint16_t count = length;
//	p_text += start;
//	
//	while(*p_text != 0)
//	{
//		DispChar(page,x,*p_text,font);
//		
//		count--;
//		if(count == 0)
//			break;
//		
//		x = x + font->Width;
//		if(x+font->Width > 193)  // Wrap
//		{
//			x=column;
//			page += font->Height/8;
//			if(page > 8)
//				break;
//		}
//		p_text++;
//	}
//}


/****************************************************************/

///**
// * @brief  LCD背光模式
// * @param  lumi 0-100 背光强度
// */
//void DispBacklight(uint8_t lumi)
//{
//	DAC_OutVol(2, lumi);	//背光开启
//}

/**
 * @brief  LCD对比度
 * @param  contrast [85,360] -> [7.00,18.00]V  norm:266
 */
void DispContrast(uint16_t contrast)
{
	if(contrast<85 || contrast > 360)
		return;
	uint8_t temp[2] = {0};
	temp[0] = (uint8_t)contrast&0x3F;
	temp[1] = (uint8_t)(contrast>>6)&0x07;
	lcd_write_cmd(0x81,2,temp[0],temp[1]); //调对比度
}


/**
 * @brief  LCD灰度调整
 * @param  light,dark [0x00, 0x1F]
 */
void DispGreyLevel(uint8_t light, uint8_t dark)
{
	if(light > 0x1F || dark > 0x1F)
		return;
	lcd_write_cmd(0x31,0); //EXT=1 ****EXT1****
	lcd_write_cmd(0x20,16,0x00,0x00,0x00,light,light,light,0x00,0x00,dark,0x00,0x00,dark,dark,dark,0x00,0x00); // Gray Level
	lcd_write_cmd(0x30,0); //EXT=0 ****EXT0****
}


/**
 * @brief  屏幕反显
 * @param  inv true->Inverse display
 */
void DispInverse(bool inv)
{
	uint8_t temp = 0xA6;
	if(inv)
		temp+=1;
	lcd_write_cmd(temp, 0);
}

/**
 * @brief  LCD显示
 * @param  mode 1:正常显示 0:关闭模式
 */
void DispPixelOn(uint8_t mode)
{
	if(mode == 0)
		lcd_write_cmd(0xAE,0);	//PD = 0 省电模式
	else
		lcd_write_cmd(0xAF,0);	//PD = 1
}

void DispShutDown(void)
{
	DispPixelOn(0);
	lcd_write_cmd(0x95,0);	// sleep mode
	delay_ms(120);
	RST_PIN_LOW;
	delay_ms(200);
}

void lcd_grey_test(void)
{
	uint16_t i,j;
	
	DispFill(0,0,256,96,0x00);
	delay_ms(1000);
	DispFill(0,0,256,96,0x55);
	delay_ms(1000);
	DispFill(0,0,256,96,0xAA);
	delay_ms(1000);
	DispFill(0,0,256,96,0xFF);
	delay_ms(1000);
	DispFill(0,0,256,96,0x00);
	delay_ms(1000);
	
	DispFill(0 ,0,256,24,0x00);
	DispFill(6 ,0,256,24,0x55);
	DispFill(12,0,256,24,0xaa);
	DispFill(18,0,256,24,0xff);
	delay_ms(1000);
	delay_ms(1000);
	
	DispFill(0 ,0,256,24,0xff);
	DispFill(6 ,0,256,24,0xaa);
	DispFill(12,0,256,24,0x55);
	DispFill(18,0,256,24,0x00);
	delay_ms(1000);
	delay_ms(1000);
	
	DispFill(0,0,256,96,0x00);
	delay_ms(1000);
	
	lcd_set_cursor(0, 0);
	lcd_write_cmd(0x5c, 0);
	for(j=0;j<6;j++)
	{
		lcd_write_multi_repeat(0x00, 256);
		lcd_write_multi_repeat(0x55, 256);
		lcd_write_multi_repeat(0xAA, 256);
		lcd_write_multi_repeat(0xFF, 256);
	}
	delay_ms(1000);
	delay_ms(1000);
}

void lcd_gui_test(void)
{
	uint16_t i;
	
	// line test
	GUI_FillBuff(0,0,256,96,COLOR_WHITE);
	for(i=0;i<6;i++)
		GUI_Line(0,0,255,16*i,COLOR_BLACK);
	for(i=0;i<16;i++)
		GUI_Line(0,0,16*i,95,COLOR_BLACK);
	GUI_Line(0,0,255,95,COLOR_BLACK);
	delay_ms(1000);delay_ms(1000);delay_ms(1000);delay_ms(1000);delay_ms(1000);
	
	// line test 2
	GUI_FillBuff(0,0,256,96,COLOR_WHITE);
	for(i=0;i<6;i++)
		GUI_Line_VH(0,16*i,DIR_H,256,i+1,COLOR_BLACK);
	for(i=0;i<16;i++)
		GUI_Line_VH(16*i,0,DIR_V,96,1,COLOR_BLACK);
	delay_ms(1000);delay_ms(1000);delay_ms(1000);delay_ms(1000);delay_ms(1000);
	
	// rect test
	GUI_FillBuff(0,0,256,96,COLOR_WHITE);
	for(i=1;i<=3;i++) // 32,64,96/12,24,36 - 
		GUI_Rectangle(32*i, 12*i, 256-64*i, 96-24*i, 4, COLOR_BLACK);
	delay_ms(1000);delay_ms(1000);delay_ms(1000);delay_ms(1000);delay_ms(1000);
	
	// number test
	GUI_FillBuff(0,0,256,96,COLOR_WHITE);
	GUI_Number(3,2,83472,ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
	GUI_Number(3,22,-12495,ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
	
	GUI_Number(80,42,193,ALIGNMENT_RIGHT,&Font12,COLOR_BLACK,COLOR_WHITE);
	GUI_Number(80,62,-8236,ALIGNMENT_RIGHT,&Font12,COLOR_BLACK,COLOR_WHITE);
	
	GUI_Number(3,82,0,ALIGNMENT_LEFT,&Font12,COLOR_BLACK,COLOR_WHITE);
}

void LCD_SPI_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_SPI1);
	
	delay_us(10);
	
	//PB12, 14 -> LCD_CS, DC
	GPIO_BOP(GPIOB) = GPIO_PIN_12 | GPIO_PIN_14;
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_12 | GPIO_PIN_14);
	//PB13,15 -> SPI1_SCK,SDA
	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_13 | GPIO_PIN_15);
	
	spi_parameter_struct sspi;
	/* deinitilize SPI and the parameters */
	spi_struct_para_init(&sspi);
	
	spi_i2s_deinit(SPI1);
	/* SPI1 parameter config */
	sspi.trans_mode           = SPI_TRANSMODE_BDTRANSMIT;
	sspi.device_mode          = SPI_MASTER;
	sspi.frame_size           = SPI_FRAMESIZE_8BIT;
	sspi.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
	sspi.nss                  = SPI_NSS_SOFT;
	sspi.prescale             = SPI_PSC_32;  //F = 60MHz(APB1) / 8(PSC) = 7.5MHz (Max = 12.5MHz)
	sspi.endian               = SPI_ENDIAN_MSB;
	spi_init(SPI1, &sspi);
	
	spi_nss_output_disable(SPI1);
	spi_bidirectional_transfer_config(SPI1, SPI_BIDIRECTIONAL_TRANSMIT);
	
	spi_enable(SPI1);
}

void LCD_StructInit(struct displayConfig* init, bool initPara)
{
	if(initPara)
	{
		init->backTime = 5;
		init->backTimeCounter = init->backTime*10;
		init->brightness = 20;
		init->contrast = 265;
		init->greyLevel[0] = 16;
		init->greyLevel[1] = 20;
		init->invDisp = false;
	}
}

/**
 * @brief  LCD初始化
 */
void LCD_Init(struct displayConfig init)
{
	LCD_SPI_Init();
	
	RST_PIN_LOW;
	delay_ms(100);
	RST_PIN_HIGH;
	delay_ms(100);
/****************************************************************/
	
	lcd_write_cmd(0x30,0); //EXT=0 ****EXT0****
	
	lcd_write_cmd(0x94,0); //Sleep out
	
	lcd_write_cmd(0x31,0); //EXT=1 ****EXT1****
	
	lcd_write_cmd(0xD7,1,0x9f); //Autoread disable
	
	lcd_write_cmd(0x32,3,0x00,0x01,0x03); //Analog SET:booster level 1, Bias=1/11
	
	lcd_write_cmd(0xf2,3,0x1e,0x28,0x32); //温度补偿
	
	lcd_write_cmd(0xf0,4,0x0d,0x0d,0x0d,0x0d); //频率设置:73fps
	
	lcd_write_cmd(0x30,0); //EXT=0 ****EXT0****
	
	lcd_set_cursor(0,0);
	
//	lcd_write_cmd(0x0C,0); //Data format (LSB on top)
	
	lcd_write_cmd(0xBC,1,0x00); //Data scan direction MV,MX,MY = 0
	
	lcd_write_cmd(0xCA,3,0x00,0x9f,0x0a); //Display Control:Duty = 160 | Nline = 11
	
	lcd_write_cmd(0xF0,1,0x11); //Display Mode:10=Monochrome Mode, 11=4Gray
	
	lcd_write_cmd(0x20,1,0x0b); //Power control:all on
	
	DispContrast(init.contrast);
	DispGreyLevel(init.greyLevel[0],init.greyLevel[1]);
	DispInverse(init.invDisp);
	
	GUI_ClearBuff(COLOR_WHITE);
	DispFill(0 ,0 ,256 ,96 ,COLOR_WHITE);
	
	delay_us(100);
	DispPixelOn(1); //Display on
	
//	lcd_grey_test();

/****************************************************************/
	dac_enable(DAC0);
	
	delay_ms(5);
}

