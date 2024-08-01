#include "init.h"
#include "gd32f30x.h"
#include "systick.h"


void SYS_Init(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	systick_config();
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOD);
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_TIMER5);
	rcu_periph_clock_enable(RCU_TIMER6);
	rcu_periph_clock_enable(RCU_ADC0);
	rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV16);  // APB2 120MH/16 = 7.5MH
	rcu_periph_clock_enable(RCU_DAC);
	//rcu_periph_clock_enable(RCU_I2C0);
	//rcu_periph_clock_enable(RCU_I2C1);
	
	rcu_periph_clock_enable(RCU_USART0);
	
	delay_ms(5);
}
    


void GPIO_Init(void)
{
	/******************************GPIO INPUT***********************************/
	
	gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);
	
	//   | S1 | HP_DET | SAF_BUSY |
	//PA | 0  |   7    |    15    |
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_0 | GPIO_PIN_7);
	gpio_init(GPIOA, GPIO_MODE_IPD, GPIO_OSPEED_2MHZ, GPIO_PIN_15);
	
	//A1,B1,A2,B2 -> PB0,1,10,11
	gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_2MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11);
	
	/******************************GPIO OUTPUT***********************************/
	
	//HP_EN -> PA6
	GPIO_BC(GPIOA) = GPIO_PIN_6;
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_6);
	
	//   | ADC_CTR | AMP_EN | PWR_CTR | LCD_RST | PWR_LCD | SAF_RST | SAF_LNA |
	//PC |    3    |    4   |    6    |    7    |    8    |   10    |    11   |
	GPIO_BC(GPIOC) = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_10 | GPIO_PIN_11;
	gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_10 | GPIO_PIN_11);
	
	//BOOT_CTR -> PB8
	GPIO_BC(GPIOB) = GPIO_PIN_8;
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_8);
	
	/******************************ANALOG PIN**********************************/
	
	//PC2 -> BAT_VOLT(ADC012_IN12)
	gpio_init(GPIOC, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
	
	//PA4,5 -> DAC0
	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_5);
	
	//PA2 -> KEY_AD(ADC012_IN2)
	gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	
	/******************************COM PORT***********************************/
	
	//PB6,7 -> I2C0_SCL,SDA
	GPIO_BOP(GPIOB) = GPIO_PIN_6 | GPIO_PIN_7;
	gpio_init(GPIOB, GPIO_MODE_OUT_OD, GPIO_OSPEED_2MHZ, GPIO_PIN_6 | GPIO_PIN_7);
	
	//PA9,10 -> USART0_TX,RX
  gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
  gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
	
}


//TIMER 0-7
void Timer_Init(void)
{
	timer_parameter_struct stim;
	uint32_t ck_ahb, ck_apb1, ck_tim;
	
	ck_ahb = rcu_clock_freq_get(CK_AHB);
	ck_apb1 = rcu_clock_freq_get(CK_APB1);
	if(ck_ahb != ck_apb1)
		ck_tim = ck_apb1 * 2;
	else
		ck_tim = ck_apb1;
	
	timer_struct_para_init(&stim);
	
	// TIM5 1ms/1KHz
	timer_deinit(TIMER5);
	stim.prescaler = ck_tim/1000000 - 1;
	stim.alignedmode = TIMER_COUNTER_EDGE;
	stim.counterdirection = TIMER_COUNTER_UP;
	stim.period = 1000-1;
	stim.clockdivision     = TIMER_CKDIV_DIV1;
	timer_init(TIMER5, &stim);
	
	nvic_irq_enable(TIMER5_IRQn, 2U, 0U);
	timer_interrupt_flag_clear(TIMER5, TIMER_INT_FLAG_UP);
	timer_interrupt_enable(TIMER5, TIMER_INT_UP);
	
	// TIM6 100ms/10Hz
	timer_deinit(TIMER6);
	stim.prescaler = ck_tim/100000 - 1;
	stim.alignedmode = TIMER_COUNTER_EDGE;
	stim.counterdirection = TIMER_COUNTER_UP;
	stim.period = 10000-1;
	stim.clockdivision     = TIMER_CKDIV_DIV1;
	timer_init(TIMER6, &stim);
	
	nvic_irq_enable(TIMER6_IRQn, 4U, 0U);
	timer_interrupt_flag_clear(TIMER6, TIMER_INT_FLAG_UP);
	timer_interrupt_enable(TIMER6, TIMER_INT_UP);
}


//ADC0
void ADC_Init(void)
{
	adc_deinit(ADC0);
	adc_mode_config(ADC_MODE_FREE);
	adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
	adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
	
  //Totol time = (1.5+SAMPLETIME)/Fadc(MHz) us
	adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL,1U);
	adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_2, ADC_SAMPLETIME_71POINT5);
	adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
	adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
	
	// GD32F303 Injection mode not found
	adc_channel_length_config(ADC0, ADC_INSERTED_CHANNEL, 2U);
	adc_inserted_channel_config(ADC0, 0, ADC_CHANNEL_12, ADC_SAMPLETIME_239POINT5);
  adc_inserted_channel_config(ADC0, 1, ADC_CHANNEL_16, ADC_SAMPLETIME_239POINT5);
	adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL, ADC0_1_2_EXTTRIG_INSERTED_NONE);
	adc_external_trigger_config(ADC0, ADC_INSERTED_CHANNEL, ENABLE);
	
	nvic_irq_enable(ADC0_1_IRQn, 3U, 0U);
	adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOC);
	adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
	adc_interrupt_enable(ADC0, ADC_INT_EOC);
	adc_interrupt_enable(ADC0, ADC_INT_EOIC);
	
	adc_enable(ADC0);
	delay_ms(5);
	adc_calibration_enable(ADC0);
  
  adc_tempsensor_vrefint_enable();
}


//DAC0,1
void DAC_Init(void)
{
	dac_deinit();
	
	//PA4-BLSIG
	dac_trigger_source_config(DAC0,DAC_TRIGGER_SOFTWARE);
	dac_trigger_enable(DAC0);
	dac_wave_mode_config(DAC0, DAC_WAVE_DISABLE);
	dac_output_buffer_disable(DAC0);
	
	//PA5-RFSIG
	dac_trigger_source_config(DAC1,DAC_TRIGGER_SOFTWARE);
	dac_trigger_enable(DAC1);
	dac_wave_mode_config(DAC1, DAC_WAVE_DISABLE);
	dac_output_buffer_disable(DAC1);
	
	dac_disable(DAC0);
	dac_disable(DAC1);
}


//I2C0,1
void I2C_Init(void)
{
//	//I2C0 -> EEPROM
//	i2c_clock_config(I2C0, 400000U, I2C_DTCY_2);
//	i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0xA0);
//	i2c_enable(I2C0);
//	i2c_ack_config(I2C0, I2C_ACK_ENABLE);
	
//	//I2C1 -> DAC,TUNER
//	i2c_clock_config(I2C1, 400000U, I2C_DTCY_2);
//	i2c_mode_addr_config(I2C1, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0xC8);
//	i2c_enable(I2C1);
//	i2c_ack_config(I2C1, I2C_ACK_ENABLE);
}

void USART_Init(void)
{
	/* USART configure */
	
	usart_deinit(USART0);
	usart_baudrate_set(USART0, 921600U);
	usart_word_length_set(USART0, USART_WL_8BIT);
	usart_stop_bit_set(USART0, USART_STB_1BIT);
	usart_parity_config(USART0, USART_PM_NONE);
	usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
	usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
	usart_receive_config(USART0, USART_RECEIVE_ENABLE);
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
	usart_enable(USART0);
	
}


