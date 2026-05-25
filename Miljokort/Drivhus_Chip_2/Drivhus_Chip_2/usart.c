/*
 * usart.c
 *
 * Created: 05.03.2026 11:06:31
 *  Author: elisabeth
 */ 

#include "usart.h"   // inkluder headerfilen med deklarasjoner og registerdefinisjoner

// Generelle funksjoner

// Initierer en USART med angitt port, pinner og baudrate
void usart_init(USART_t *usart, PORT_t *port, uint8_t tx_bp, uint8_t rx_bp, uint16_t baud) {
	usart->BAUD = baud; // setter baudrate (kommunikasjonshastighet)
	usart->CTRLB = USART_RXEN_bm | USART_TXEN_bm; // aktiver mottak (RX) og sending (TX)
	port->DIR |= (1 << tx_bp); // setter TX-pinnen som output
	port->DIR &= ~(1 << rx_bp); // setter RX-pinnen som input
}

// Sender en hel streng (char-array) via en USART
uint8_t usart_transmit_string(USART_t *usart, const char *string) {
	for (uint8_t i = 0; string[i] != '\0'; i++) { // gå gjennom alle tegn i strengen
		while (!(usart->STATUS & USART_DREIF_bm)); // vent til senderegisteret er klart
		usart->TXDATAL = string[i]; // send ett tegn
	}
	return 0; // returner OK
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


// ------------------------RASBERRY PI FUNKSJONER ----------------------------------------------------
void usart_pi_init(void)
{
	// USART1 må routes til DEFAULT for å komme ut på PC0/PC1
	PORTMUX.USARTROUTEA |= PORTMUX_USART1_DEFAULT_gc;
	// Pi: TXD0/RXD0 er koblet til AVR USART1: PC0=TX2, PC1=RX2 (se skjema)
	usart_init(&USART1, &PORTC, PIN0_bp, PIN1_bp, 1667); // 9600 baud ved 4 MHz
}

// Sender en hel tekststreng via USART1 (mot Raspberry Pi)
uint8_t usart_pi_transmit_char_array(const char string[])
{
	return usart_transmit_string(&USART1, string);
}

// Sender et heltall via USART1 (mot Raspberry Pi)
void usart_pi_transmit_uint16(uint16_t value)
{
	usart_transmit_uint16(&USART1, value);
}

// Mottar ett tegn via USART1 (fra Raspberry Pi)
char usart_pi_receive_char(void)
{
	while (!(USART1.STATUS & USART_RXCIF_bm)); // vent til et tegn er mottatt
	return USART1.RXDATAL; // les og returner mottatt tegn
}

// Mottar en tekststreng via en USART til buffer
void usart_pi_receive_string(char *buffer, uint8_t max_length)
{
	usart_receive_string(&USART1, buffer, max_length);
}