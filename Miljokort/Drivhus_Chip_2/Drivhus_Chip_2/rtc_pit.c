/*
 * rtc_pit.c
 *
 * Created: 05.04.2026 11:44:58
 *  Author: Elisabeth
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include "rtc_pit.h"

void rtc_pit_init_1s(void)
{
	// stopp PIT før oppsett
	while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) { ; }
	RTC.PITCTRLA = 0;

	// enable PIT interrupt
	RTC.PITINTCTRL = RTC_PI_bm;

	// 32768 RTC-klokker = ca 1 sekund
	while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) { ; }
	RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
}

void rtc_pit_init_2s(void)
{
	rtc_pit_init_1s();
}