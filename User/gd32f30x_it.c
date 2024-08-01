/*!
    \file    gd32f30x_it.c
    \brief   interrupt service routines

    \version 2017-02-10, V1.0.0, firmware for GD32F30x
    \version 2018-10-10, V1.1.0, firmware for GD32F30x
    \version 2018-12-25, V2.0.0, firmware for GD32F30x
    \version 2020-09-30, V2.1.0, firmware for GD32F30x 
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f30x_it.h"
#include "main.h"
#include "systick.h"
#include "lcd.h"
#include "rtc.h"

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void)
{
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void)
{
}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
}

//void TIMER5_IRQHandler(void)
//{
//	static uint8_t i = 0;
//	if(timer_interrupt_flag_get(TIMER5, TIMER_INT_FLAG_UP) != RESET)
//	{
//		//TIM_Callback(5);
//		i++;
//		if(i==20)
//		{
//			lcd_update();
//			i=0;
//		}
//	}
//}

void TIMER5_IRQHandler(void)
{
	if(timer_interrupt_flag_get(TIMER5, TIMER_INT_FLAG_UP) != RESET)
	{
		TIM_Callback(5);
	}
}

void TIMER6_IRQHandler(void)
{
	if(timer_interrupt_flag_get(TIMER6, TIMER_INT_FLAG_UP) != RESET)
	{
		TIM_Callback(6);
	}
}

void ADC0_1_IRQHandler(void)
{
	if(adc_interrupt_flag_get(ADC0,ADC_INT_FLAG_EOC) != RESET)
	{
		ADC_Callback(0,'R');
	}
	if(adc_interrupt_flag_get(ADC0,ADC_INT_FLAG_EOIC) != RESET)
	{
		ADC_Callback(0,'I');
	}
	
}

/*!
    \brief      this function handles RTC global interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void RTC_IRQHandler(void)
{
	uint32_t time;
	if (rtc_flag_get(RTC_FLAG_SECOND) != RESET){
		/* clear the RTC second interrupt flag*/
		rtc_flag_clear(RTC_FLAG_SECOND);
		/* wait until last write operation on RTC registers has finished */
		//rtc_lwoff_wait();
		RTC_Callback(0);
	}
	else if (rtc_flag_get(RTC_FLAG_ALARM) != RESET){
		/* clear the RTC second interrupt flag*/
		rtc_flag_clear(RTC_FLAG_ALARM);
		/* wait until last write operation on RTC registers has finished */
		rtc_lwoff_wait();
		time = rtc_get_time();
		rtc_set_alarm(time+170*60);
		RTC_Callback(1);
	}
}

/*!
    \brief      this function handles DMA_Channel1_2_IRQHandler interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DMA0_Channel4_IRQHandler(void)
{
	if(dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INT_FLAG_FTF))
	{
		dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_G);
		lcd_dma_callback();
	}
}


