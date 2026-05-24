/*
 * usart.h
 *
 * Created: 05.03.2026 11:06:50
 *  Author: elisa
 */ 


#ifndef USART_H_
#define USART_H_

#include <avr/io.h>   // gir tilgang til registerdefinisjoner (USART, PORT osv.)
#include <stdint.h>   // gir tilgang til uint8_t, uint16_t osv.

// Generelle USART-funksjoner
void usart_init(USART_t *usart, PORT_t *port, uint8_t tx_bp, uint8_t rx_bp, uint16_t baud); // initialiserer en USART
uint8_t usart_transmit_string(USART_t *usart, const char *string); // sender en tekststreng
void usart_transmit_uint16(USART_t *usart, uint16_t value); // sender et uint16-tall som tekst

// Spesifikke funksjoner for USART3 (USB på kortet)
void usart_usb_init(void); // initialiserer USART3
uint8_t usart_usb_transmit_char_array(const char string[]); // sender tekst via USART3
void usart_usb_transmit_uint16(uint16_t value); // sender heltall via USART3
char usart_usb_receive_char(void); // mottar ett tegn via USART3
void usart_usb_receive_string(char *buffer, uint8_t max_length); // mottar en tekststreng via USART3

// Raspberry Pi / USART1
void usart_pi_init(void); // initialiserer USART1 for kommunikasjon med Raspberry Pi
uint8_t usart_pi_transmit_char_array(const char string[]); // sender tekst via USART1 til Raspberry Pi
void usart_pi_transmit_uint16(uint16_t value); // sender heltall via USART1 til Raspberry Pi
char usart_pi_receive_char(void); // mottar ett tegn via USART1 fra Raspberry Pi
void usart_pi_receive_string(char *buffer, uint8_t max_length); // mottar en tekststreng via USART1 fra Raspberry Pi

#endif /* USART_H_ */