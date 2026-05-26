#include <avr/cpufunc.h>
#include <stdio.h>
#include <avr/io.h>
#include <stdint.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include "chip1.h"


volatile uint8_t rx_index = 0;
volatile char rx_buffer[32];

volatile int fan_value = 0;
volatile int pump_value = 0;
volatile uint8_t message_ready = 0;

void oschf_init(void)
{
    ccp_write_io((void *) & CLKCTRL.OSCHFCTRLA,
                            CLKCTRL_RUNSTDBY_bm         //Lets the clock run in stdby mode
                          | CLKCTRL_ENABLE_bm           //Enables the oscillator
                          | CLKCTRL_FRQRANGE_16M_gc);   //Sets the clock as 16 MHz
    _delay_ms(25);
    ccp_write_io((void *) & CLKCTRL.MCLKCTRLA,
                            CLKCTRL_CLKSEL_OSCHF_gc);  //Sets the OSCHF as main clock
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = 0;
}

void tca_split_init(uint8_t power)
{
    // Route TCA til PORTA
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;

    // Sett PA4 som output
    PORTA.DIRSET = PIN4_bm;
    PORTA.DIRSET = PIN3_bm;

    // Split mode + enable compare channel
    TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

    TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm |  TCA_SPLIT_HCMP0EN_bm;; // WO4 

    // Sett periode (8-bit  maks 255)
    TCA0.SPLIT.HPER = 255;

    // Duty cycle
    TCA0.SPLIT.HCMP1 = 0;
    TCA0.SPLIT.HCMP0 = 0;

    // Start timer 
    TCA0.SPLIT.CTRLA = TCA_SPLIT_ENABLE_bm
                     | TCA_SPLIT_CLKSEL_DIV256_gc;
}

void adc_init(void)
    {
        VREF.ADC0REF = VREF_REFSEL_VDD_gc;       //Sets referece voltage
        _delay_ms(25);                           //Allows voltage to stabilize
        ADC0.CTRLA = ADC_ENABLE_bm;              //Enables ADC
        ADC0.CTRLC = ADC_PRESC_DIV2_gc;        //Set desired prescaler division 
        ADC0.SAMPCTRL = 0x10;
    }

static uint16_t adc_read_channel(uint8_t muxpos)
{
    ADC0.MUXPOS = muxpos;

    ADC0.COMMAND = ADC_STCONV_bm;
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    // discard first sample after channel switch
    ADC0.COMMAND = ADC_STCONV_bm;
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
    ADC0.INTFLAGS = ADC_RESRDY_bm;

    return ADC0.RES;
}

//reads both ADC channels
adc_values_t adc_read_ain1_ain2(void)
{
    adc_values_t values;

    values.ain1 = adc_read_channel(ADC_MUXPOS_AIN1_gc);
    values.ain2 = adc_read_channel(ADC_MUXPOS_AIN2_gc);

    return values;
}

void usart_init(void)
{
    USART2.BAUD = 833;                     //Set baudrate
    USART2.CTRLB = USART_RXEN_bm            //Enable reciver pin
                 | USART_TXEN_bm            //Enable transfer pin
                 | USART_RXMODE_NORMAL_gc;  //Set mode
    USART2.CTRLA |= USART_RXCIE_bm;
    PORTF.DIRSET = PIN4_bm;                 //Set transfer pin as output
    PORTF.DIRCLR = PIN5_bm;                 //Set receive pin as input
    PORTMUX.USARTROUTEA = PORTMUX_USART2_ALT1_gc;
    
}
//Sends char over UART
void usart_transmit(char data)
{
    while (!(USART2.STATUS & USART_DREIF_bm))//Wait until data transfer register is empty
    {
        ;
    }
    USART2.TXDATAL = data;
}

// Sender en hel streng (char-array) via en USART
void usart_transmit_string(char* data)
{
    for (uint8_t i = 0; data[i] != '\0'; i++)
    {
        usart_transmit(data[i]);
    }
}

// Sender et heltall (0–65535) som tekst
void usart_transmit_uint16(USART_t *usart, uint16_t value) {
	char buffer[6]; // buffer for maks 5 siffer + null-terminering
	uint8_t i = 0; // teller for buffer
	
	if (value == 0) { // spesialtilfelle hvis tallet er 0
		while (!(usart->STATUS & USART_DREIF_bm)); // vent til senderen er klar
		usart->TXDATAL = '0'; // send tegnet '0'
		return; // avslutt funksjonen
	}
	
	// Gjør om tallet til ASCII (baklengs)
	while (value > 0 && i < sizeof(buffer) - 1) { // fortsett til tallet er ferdig
		buffer[i++] = '0' + (value % 10); // lag ASCII-tegn fra siste siffer
		value /= 10; // fjern siste siffer fra tallet
	}
	
	// Skriv ut i riktig rekkefølge
	while (i > 0) { // gå gjennom buffer baklengs
		while (!(usart->STATUS & USART_DREIF_bm)); // vent til senderen er klar
		usart->TXDATAL = buffer[--i]; // send neste tegn
	}
}

// Mottar en tekststreng via en USART til buffer
void usart_receive_string(USART_t *usart, char *buffer, uint8_t max_length)
{
	uint8_t i = 0;
	char c;

	while (i < max_length - 1) // -1 er for å ha plass til '\0'
	{
		while (!(usart->STATUS & USART_RXCIF_bm)); // vent på mottatt tegn
		c = usart->RXDATAL; // les tegnet

		if (c == '\n' || c == '\r') // stopp ved linjeskift
		{
			break;
		}

		buffer[i++] = c; // lagre tegn i buffer
	}

	buffer[i] = '\0'; // avslutt som C-streng
}
//setter TCA compare value 
void set_fan(int val)
{
    TCA0.SPLIT.HCMP1 = val;
}
//setter TCA compare value 
void set_pump(int val)
{
    TCA0.SPLIT.HCMP0 = val;
}


void RTC_init(void)
{
    // Wait for synchronization
    while (RTC.STATUS > 0);

    // Select internal 32.768 kHz oscillator
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;

    // 32768 ticks = 1 second
    RTC.PER = 32768;

    // Enable overflow interrupt
    RTC.INTCTRL = RTC_OVF_bm;

    // Enable RTC, no prescaling
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;
    
}
//parser meldingen og setter respektive verdier til set_pump og set_fan
void parse_message(char *msg)
{
    int f, p;

    if (sscanf(msg, "FAN:%d-PUMP:%d", &f, &p) == 2)
    {
        fan_value = f;
        pump_value = p;

        set_fan(fan_value);
        set_pump(pump_value);
    }
}

ISR(USART2_RXC_vect)
{
    char c = USART2.RXDATAL;

    if (c == '\n' || c == '\r')
    {
        rx_buffer[rx_index] = '\0';
        rx_index = 0;
        message_ready = 1;

        PORTA.OUTTGL = PIN2_bm;
    }
    else
    {
        if (rx_index < sizeof(rx_buffer) - 1)
        {
            rx_buffer[rx_index++] = c;
        }
        else
        {
            // overflow protection: reset buffer
            rx_index = 0;
        }
    }
}


uint16_t avg;

ISR(RTC_CNT_vect)
{
    RTC.INTFLAGS = RTC_OVF_bm;
    
    PORTA.OUTTGL = PIN2_bm;
    

    adc_values_t soil = adc_read_ain1_ain2();
    
    avg = (soil.ain1 + soil.ain2) / 2;

    char msg[32];
    sprintf(msg, "SOIL:%u\n", avg);

    usart_transmit_string(msg);
}