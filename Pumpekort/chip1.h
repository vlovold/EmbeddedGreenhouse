/* 
 * File:   chip1.h
 * Author: viktor
 *
 * Created on April 14, 2026, 8:01 PM
 */

#ifndef CHIP1_H
#define	CHIP1_H

#include "chip1.h"
#include <avr/cpufunc.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

extern volatile char rx_buffer[32];
extern volatile uint8_t rx_index;
extern volatile uint8_t message_ready;

extern volatile int fan_value;
extern volatile int pump_value;


void oschf_init(void);

void tca_split_init(uint8_t power);

void adc_init(void);

typedef struct {
    uint16_t ain1;
    uint16_t ain2;
} adc_values_t;

adc_values_t adc_read_ain1_ain2(void);

void usart_init(void); // initialiserer en USART
void usart_transmit(char data);
void usart_transmit_string(char* data); // sender en tekststreng
void usart_transmit_uint16(USART_t *usart, uint16_t value); // sender et uint16-tall som tekst

void RTC_init(void);

void parse_message(char *msg);

void set_fan(int val);
void set_pump(int val);



#endif	/* CHIP1_H */

