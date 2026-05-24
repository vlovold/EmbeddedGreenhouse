/*
 * tca.c – Generell PWM / servo / LED-modul for AVR128DB48
 */

#define F_CPU 4000000UL
#include "board.h"
#include "tca.h"
#include <util/delay.h>


//------------------------------led------------------------------------------------------------------------------------------------------------
/*
void led_pwm_init(void)
{
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTF_gc;     // rout TCA0 til PORTF
	PORTF.DIRSET = PIN2_bm;                     // PF2 som utgang
	//PORTF.PIN2CTRL |= PORT_INVEN_bm;

	TCA0.SINGLE.CTRLA = 0;                         // stopp timer under oppsett
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;

	TCA0.SINGLE.PER = 1250;                        // periode
	TCA0.SINGLE.CMP2 = 0;                          // start på 0 %
	TCA0.SINGLE.CNT = 0;

	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;     // aktiver WO2 = PF2

	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc
	| TCA_SINGLE_ENABLE_bm;      // start TCA0
}
*/

//----------GROLYS LED INIT--------------------------------------------------------------------------------------

void led_pwm_init(void)
{
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTF_gc;   // route TCA0 til PORTF
	PORTF.DIRSET = PIN4_bm;                      // PF4 som utgang

	TCA0.SPLIT.CTRLA = 0;                        // stopp timer under oppsett
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;      // split mode
	TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm;     // HCMP1 -> WO4 -> PF4

	TCA0.SPLIT.HPER  = 99;                       // 40 kHz ved F_CPU=4 MHz, DIV=1
	TCA0.SPLIT.HCMP1 = 0;						// duty cycle
	TCA0.SPLIT.HCNT  = 0;						// start teller fra 0

	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1_gc
	| TCA_SPLIT_ENABLE_bm;      // start timer
}


/*
void led_set_percent(uint8_t percent)
{
	if (percent > 100)
	{
		percent = 100;
	}

	uint16_t cmp_value = ((uint32_t)percent * TCA0.SINGLE.PER) / 100;
	TCA0.SINGLE.CMP2 = cmp_value;
}
*/

void led_set_percent(uint8_t percent)
{
	if (percent > 100)
	{
		percent = 100;
	}

	uint16_t cmp_value = ((uint32_t)percent * TCA0.SPLIT.HPER) / 100;
	TCA0.SPLIT.HCMP1 = cmp_value;
}
