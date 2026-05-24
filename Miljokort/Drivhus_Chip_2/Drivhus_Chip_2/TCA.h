/*
 * tca.h – PWM / servo / LED-modul for AVR128DB48
 * Struktur matcher adc.h (generelle funksjoner + spesialfunksjoner)
 */

#ifndef TCA_H
#define TCA_H

#include <avr/io.h>
#include <stdint.h>



void led_pwm_init(void);
void led_set_percent(uint8_t percent);

#endif
