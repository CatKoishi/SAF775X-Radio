
#include "func.h"
#include "systick.h"

float inRangeFloat(float min, float max, float num)
{
	float tmp;
	if(num > max)
		tmp = max;
	else if(num < min)
		tmp = min;
	else
		tmp = num;
	return tmp;
}

int32_t inRangeInt(int32_t min, int32_t max, int32_t num)
{
	int32_t tmp;
	if(num > max)
		tmp = max;
	else if(num < min)
		tmp = min;
	else
		tmp = num;
	return tmp;
}

int32_t inRangeLoop(int32_t min, int32_t max, int32_t num, int32_t dir)
{
	int32_t tmp = num + dir;
//	if(tmp > max)
//		tmp = tmp + min - max - 1;
//	else if(tmp < min)
//		tmp = tmp - min + max + 1;
	if(tmp > max)
		tmp = min;
	else if(tmp < min)
		tmp = max;
	return tmp;
}

uint32_t myabs(int32_t number)
{
	if(number<0)
		return -number;
	else
		return number;
}

/**************************************************************************************/
/*
DAC CHANNEL 1 -> PA4 -> BackLight
DAC CHANNEL 2 -> PA5 -> RF_SIG
*/
void DAC_Start(uint8_t ch)
{
	switch(ch)
	{
		case 1 : dac_enable(DAC0);break;
		case 2 : dac_enable(DAC1);break;
		default :{
			dac_enable(DAC0);
			dac_enable(DAC1);
		}break;
	}
}


void DAC_Stop(uint8_t ch)
{
	switch(ch)
	{
		case 1 : dac_disable(DAC0);break;
		case 2 : dac_disable(DAC1);break;
		default :{
			dac_disable(DAC0);
			dac_disable(DAC1);
		}break;
	}
}


void DAC_OutVal(uint8_t ch, uint16_t val)
{
	uint16_t output;
	switch(ch)
	{
		case 1 : {	//BackLight max:4095 -> 3.3V -> 0.58V -> 58mA
			if(val == 0)
				output = 0;
			else
				output = (uint16_t)(40.8*val+12);
			dac_data_set(DAC0,DAC_ALIGN_12B_R, output);
			dac_software_trigger_enable(DAC0);
		};break;
		
		case 2 : {	//RF_SIG
			output = inRangeInt(0,1300,val);
			dac_data_set(DAC1,DAC_ALIGN_12B_R, output);
			dac_software_trigger_enable(DAC1);
		};break;
	}
	
}

uint16_t DAC_GetValue(uint8_t ch)
{
	uint16_t val = 0;
	switch(ch)
	{
		case 1 : val = dac_output_value_get(DAC0);break;
		case 2 : val = dac_output_value_get(DAC1);break;
	}
	return val;
}

/**************************************************************************************/

void GetTempVref(uint16_t *temperature, uint16_t *int_ref, uint8_t oversample)
{
	adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
	adc_interrupt_disable(ADC0, ADC_INT_EOC);
	
	/* ADC temperature and Vrefint enable */
	adc_tempsensor_vrefint_enable();
	delay_ms(20);
	
	for(uint8_t i = 0;i<oversample;i++)
	{
		adc_software_trigger_enable(ADC0, ADC_INSERTED_CHANNEL);
		while(!adc_flag_get(ADC0, ADC_FLAG_EOIC));
		adc_flag_clear(ADC0, ADC_FLAG_EOIC);
		*temperature += adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
		*int_ref += adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_1);
	}
	*temperature /= oversample;
	*int_ref /= oversample;
	
	adc_tempsensor_vrefint_disable();
	adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
	adc_interrupt_enable(ADC0, ADC_INT_EOC);
}

/*
	id[0-2] -> 96bit UniqueID
	flash   -> flash [kbyte]
	sram    -> SRAM  [kbyte]
*/
void GetMCUInfo(uint32_t *id, uint16_t *flash, uint16_t *sram)
{
	__IO uint32_t *ptr = (__IO uint32_t *)0x1FFFF7E0;
	uint32_t temp = *ptr;
	*flash = (uint16_t)(temp&0xFFFF);
	*sram = (uint16_t)((temp>>16)&0xFFFF);
	
	ptr = (__IO uint32_t *)0x1FFFF7E8;
	for(uint8_t i = 0;i<3;i++)
	{
		id[i] = *ptr;
		ptr++;
	}
	
}

