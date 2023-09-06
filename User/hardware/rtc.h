#ifndef __RTC_H
#define __RTC_H

/****************************************************************/

#include "main.h"
#include "gd32f30x.h"

extern uint32_t ValIRC;

void rtc_set_osc(uint16_t osc);
void rtc_set_time(uint32_t time);
uint32_t rtc_get_time(void);
void rtc_set_alarm(uint32_t alarm);
void RTC_Init(void);


#endif
