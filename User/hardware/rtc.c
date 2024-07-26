#include "stdint.h"
#include "gd32f30x.h"
#include "systick.h"

uint32_t ValIRC;

void rtc_set_osc(uint16_t osc)
{
	rtc_lwoff_wait();
	
	rtc_prescaler_set(osc-1);
  
	rtc_lwoff_wait();
}

void rtc_set_time(uint32_t time)
{
	rtc_lwoff_wait();
	
	rtc_counter_set(time);
	
	rtc_lwoff_wait();
}

uint32_t rtc_get_time(void)
{
	uint32_t t;
	rtc_register_sync_wait();
	rtc_lwoff_wait();
	
	t = rtc_counter_get();
	
	rtc_lwoff_wait();
	return t;
}

void rtc_set_alarm(uint32_t alarm)
{
	rtc_lwoff_wait();
	
	rtc_alarm_config(alarm);
	rtc_lwoff_wait();
	rtc_flag_clear(RTC_INT_ALARM);
	rtc_lwoff_wait();
	rtc_interrupt_enable(RTC_INT_ALARM);
	rtc_lwoff_wait();
	
	nvic_irq_enable(RTC_IRQn,8,0);
}

uint16_t rtc_calibration(void)
{
	/* init timer */
	timer_ic_parameter_struct timer_icinitpara;
	timer_parameter_struct timer_initpara;
	ValIRC = 0;
	
	gpio_pin_remap_config(GPIO_TIMER4CH3_IREMAP,ENABLE);
	
	rcu_periph_clock_enable(RCU_TIMER4);
	delay_ms(5);
	timer_deinit(TIMER4);
	
	/* TIMER4 configuration */
	timer_initpara.prescaler         = 0;
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;
	timer_initpara.period            = 65535;
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER4,&timer_initpara);
	
	/* TIMER4  configuration */
	/* TIMER4 CH3 input capture configuration */
	timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;
	timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;
	timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV8;
	timer_icinitpara.icfilter    = 0U;
	timer_input_capture_config(TIMER4,TIMER_CH_3,&timer_icinitpara);
	
	/* auto-reload preload enable */
  timer_auto_reload_shadow_enable(TIMER4);
	/* clear channel 3 interrupt bit */
  timer_interrupt_flag_clear(TIMER4,TIMER_INT_CH3);
  /* channel 3 interrupt enable */
	nvic_irq_enable(TIMER4_IRQn, 0, 0);
  timer_interrupt_enable(TIMER4,TIMER_INT_CH3);
	/* TIMER4 counter enable */
	timer_enable(TIMER4);
	while(!ValIRC);
	timer_interrupt_disable(TIMER4,TIMER_INT_CH3);
	timer_interrupt_flag_clear(TIMER4,TIMER_INT_CH3);
	nvic_irq_disable(TIMER4_IRQn);
	timer_disable(TIMER4);
	gpio_pin_remap_config(GPIO_TIMER4CH3_IREMAP,DISABLE);
	timer_deinit(TIMER4);
	rcu_periph_clock_disable(RCU_TIMER4);
	
	ValIRC = 108000000 * 8 / ValIRC;
	
	rtc_set_osc(ValIRC);
	
	return ValIRC;
}


void RTC_Init(void)
{
	rcu_periph_clock_enable(RCU_BKPI);
	rcu_periph_clock_enable(RCU_PMU);
	
	/* allow access to BKP domain */
	pmu_backup_write_enable();
	
	/* reset backup domain */
	bkp_deinit();
	
	/* enable LXTAL */
	rcu_osci_on(RCU_LXTAL);
	/* wait till LXTAL is ready */
	rcu_osci_stab_wait(RCU_LXTAL);
	
	/* select RCU_LXTAL as RTC clock source */
	rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
	
	/* enable RTC Clock */
	rcu_periph_clock_enable(RCU_RTC);
	
	/* wait for RTC registers synchronization */
	rtc_register_sync_wait();
	rtc_lwoff_wait();
	
	/* enable the RTC second interrupt*/
  rtc_interrupt_enable(RTC_INT_SECOND);
	rtc_lwoff_wait();
	
	/* set RTC prescaler: set RTC period to 1s */
	rtc_prescaler_set(32767);
	rtc_lwoff_wait();
	
	rtc_set_time(0);
	
	rtc_set_alarm(170*60);  // 2h50m
}

