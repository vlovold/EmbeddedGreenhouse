/*
 * Drivhus_Chip_2.c
 *
 * Created: 04.04.2026 12:19:31
 * Author : Bruker
 */

#define F_CPU 4000000UL
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/interrupt.h>

#include "TCA.h"
#include "usart.h"
#include "rtc_pit.h"
#include "i2c.h"
#include "bme280.h"
#include "veml7700.h"
#include "mxc400.h"

#define RX_MSG_LEN 64


volatile char rx_msg[RX_MSG_LEN];          // buffer til innkommende melding
volatile uint8_t rx_index = 0;             // hvor i bufferen tegnet skal lagres
volatile bool rx_message_ready = false;    // flagg for ferdig melding

char tx_msg[64];                           // buffer for tekst
uint8_t led_percent = 0;                   // lagrer siste LED-verdi

volatile uint8_t send_now = 0;             // flagg for sending
volatile uint8_t seconds_count = 0;        // teller PIT-avbrudd


// ---------------------- tolker ferdig mottatt melding ----------------------
static void handle_received_message(void)
{
	char msg[RX_MSG_LEN];    // lokal kopi av melding
	char *value = 0;         // peker til verdi etter :

	// kopierer melding fra interrupt-buffer til lokal buffer
	strcpy(msg, (char *)rx_msg);

	// finner ':' og deler opp kommando og verdi
	for (uint8_t i = 0; msg[i] != '\0'; i++)
	{
		if (msg[i] == ':')
		{
			msg[i] = '\0';        // gjør om "LED:90" til "LED" og "90"
			value = &msg[i + 1];  // peker på verdien etter :
			break;
		}
	}

	// sjekker hvilken kommando som er mottatt
	if (strcmp(msg, "LED") == 0)
	{
		if (value)
		{
			int16_t led_value = atoi(value);   // gjør tekst om til heltall

			// begrenser verdien til 0-100 %
			if (led_value < 0)
			{
				led_value = 0;
			}
			if (led_value > 100)
			{
				led_value = 100;
			}

			// lagrer siste LED-verdi
			led_percent = (uint8_t)led_value;

			// skriver til terminal for feilsøking
			/*usart_usb_transmit_char_array("Led on ");
			usart_usb_transmit_uint16(led_value);
			usart_usb_transmit_char_array("% \r\n");*/
			
			/*
			usart_pi_transmit_char_array("Led on ");
			usart_pi_transmit_uint16(led_value);
			usart_pi_transmit_char_array("% \r\n");
			*/

			// oppdaterer PWM
			led_set_percent(led_percent);
		}
	}
}


// ---------------------- RX interrupt for USART 1, MÅ ENDRE OM USART3----------------------
ISR(USART1_RXC_vect)
{
	char c = USART1.RXDATAL;   // leser mottatt tegn

	// hvis forrige melding ikke er behandlet, ignorer nye tegn
	if (rx_message_ready)
	{
		return;
	}

	// når linjeskift mottas, er meldingen ferdig
	if (c == '\n' || c == '\r')
	{
		if (rx_index > 0)
		{
			rx_msg[rx_index] = '\0';   // avslutter strengen
			rx_index = 0;              // nullstiller indeks
			rx_message_ready = true;   // sier fra til main at melding er klar
		}
	}
	else
	{
		// legger tegn inn i buffer hvis det er plass
		if (rx_index < (RX_MSG_LEN - 1))
		{
			rx_msg[rx_index++] = c;
		}
		else
		{
			// hvis buffer blir full, nullstill
			rx_index = 0;
		}
	}
}


// ---------------------- RTC PIT interrupt -----2sec-----------------
/*
ISR(RTC_PIT_vect)
{
	RTC.PITINTFLAGS = RTC_PI_bm;   // nullstiller PIT-interruptflagget

	seconds_count++;               // teller PIT-avbrudd

	// når 2 PIT-avbrudd har skjedd, skal status sendes
	if (seconds_count >= 2)
	{
		seconds_count = 0;          // nullstiller sekundtelleren
		send_now = 1;               // sier fra til main at status skal sendes
	}
}
*/

//---1. sekund--------------------------
// denne kjører hver gang PIT-timeren går ut

ISR(RTC_PIT_vect)
{
	RTC.PITINTFLAGS = RTC_PI_bm;   // nullstiller PIT-interruptflagget
	send_now = 1;                  // sier fra til main at statusmelding skal sendes
}

//---------------------- TEST DEBUG IO-KORT READ SENSORS ----------------------
/*
static void read_sensors(int16_t *temp, uint16_t *hum, uint16_t *bright, uint16_t *soil, uint16_t *fan)
{
	mxc400_data_t acc;   // testdata fra akselerometer

	// leser akselerometer som midlertidig test på at I2C virker
	mxc400_read_xyz_temp(&acc);

	// midlertidige verdier til ekte sensorer kommer
	*temp = 26;                         // placeholder for BME280 temperatur
	*hum = 55;                          // placeholder for BME280 fuktighet
	*soil = 76;
	*fan = 98;
	*bright = (uint16_t)(acc.z >> 4);   // fake lysverdi fra akselerometer
}
*/

//----------------------FAKTISK READ SENSORS---------------------------------

static void read_sensors(int16_t *temp, uint16_t *hum, uint16_t *bright)
{
	int32_t temp_c_x100;
	uint32_t hum_rh_x1024;

	// les kompensert temperatur og fuktighet fra BME280
	bme280_read_compensated(&temp_c_x100, &hum_rh_x1024);

	// les rå lysverdi fra VEML7700
	//*bright = veml7700_read_raw_als();
	*bright = 500;
	
	// gjør om temperatur fra 0.01 °C til heltall i °C
	// eks: 2356 -> 23
	*temp = (int16_t)(temp_c_x100 / 100);
	//*temp  = 26;
	
	// gjør om fuktighet fra Q22.10 til heltall i %RH
	// eks: 47445 -> ca 46
	*hum = (uint16_t)(hum_rh_x1024 / 1024);
	//*hum = 69;
}


//----------------------------DEBUG READ SENSORS FUNKSJON, SAMME FUNKSJON SOM OVER, MEN LESER OG PRINTER RÅDATA FØRST-------
/*
static void read_sensors(int16_t *temp, uint16_t *hum, uint16_t *bright)
{
	uint32_t raw_temp;
	uint16_t raw_hum;
	int32_t temp_c_x100;
	uint32_t hum_rh_x1024;
	
	usart_pi_transmit_char_array("read_sensors start\r\n");

	// les rådata først
	bme280_read_raw_all(&raw_temp, &raw_hum);

	sprintf(tx_msg, "RAW TEMP:%lu RAW HUM:%u\r\n", raw_temp, raw_hum);
	usart_pi_transmit_char_array(tx_msg);

	// kompenser rådata
	temp_c_x100 = bme280_compensate_temp(raw_temp);
	hum_rh_x1024 = bme280_compensate_hum(raw_hum);

	sprintf(tx_msg, "TEMPx100:%ld HUMx1024:%lu\r\n", temp_c_x100, hum_rh_x1024);
	usart_pi_transmit_char_array(tx_msg);

	// les rå lysverdi
	*bright = veml7700_read_raw_als();

	sprintf(tx_msg, "RAW ALS:%u\r\n", *bright);
	usart_pi_transmit_char_array(tx_msg);

	// gjør om til heltall
	*temp = (int16_t)(temp_c_x100 / 100);
	*hum = (uint16_t)(hum_rh_x1024 / 1024);
}
*/

//----------------------  sender statusmelding ----------------------
static void send_status(int16_t temp, uint16_t hum, uint16_t bright)
{
	sprintf(tx_msg,
	"TEMP:%d-HUM:%u-BRIGHT:%u-LED:%u\r\n",
	temp, hum, bright, led_percent);

	//usart_usb_transmit_char_array(tx_msg);
	usart_pi_transmit_char_array(tx_msg);
}


//----------------------  debug for I2C IO kort akselerometer----------------------
/*
static void send_debug_mxc400(void)
{
	mxc400_data_t acc;
	uint8_t id;
	uint8_t status;
	uint8_t int_src1;

	id = mxc400_read_id();
	status = mxc400_read_status();
	int_src1 = mxc400_read_int_src1();
	mxc400_read_xyz_temp(&acc);

	sprintf(tx_msg,
	"DBG ID:%u STATUS:%u INT:%u X:%d Y:%d Z:%d T:%d\r\n",
	id, status, int_src1, acc.x, acc.y, acc.z, acc.temp_raw);
	//Id er identifikasjonsregister, om man prater med riktig sensor, statusregister, 
	usart_usb_transmit_char_array(tx_msg);
}
*/

int main(void)
{
	int16_t temp_value;     // temperatur som skal sendes
	uint16_t hum_value;      // fuktighet som skal sendes
	uint16_t bright_value;   // lysverdi som skal sendes
	

	led_pwm_init();
	//usart_usb_init();
	usart_pi_init();
	_delay_ms(100);
	//usart_usb_transmit_char_array("START\r\n");


	rtc_pit_init_1s();
	
	i2c_init();
	//usart_usb_transmit_char_array("I2C init done\r\n"); // debug

	bme280_init();
	//usart_usb_transmit_char_array("BME280 init done\r\n"); //debug
	//usart_usb_transmit_char_array("BME280 ID: ");
	//usart_usb_transmit_uint16(bme280_read_id()); //id er 0x60 som i desimal blir 96.
	//usart_usb_transmit_char_array("\r\n");

	veml7700_init();
	_delay_ms(200);
	//usart_usb_transmit_char_array("VEML7700 init done\r\n"); //debug
	
	//mxc400_init();
	
	
	

	//USART3.CTRLA |= USART_RXCIE_bm;
	USART1.CTRLA |= USART_RXCIE_bm;
	sei();   // globale interrupts på

	while (1)
	{
		//---------------------- sjekk om ny melding er mottatt ----------------------
		
		if (rx_message_ready)
		{

			rx_message_ready = false;      // nullstill flagg
			handle_received_message();     // behandle kommando
			
		}
		
		

		//---------------------- send status når PIT sier fra ----------------------
 
		if (send_now)
		{
			send_now = 0;   // nullstiller flagg

			// leser sensorer
			read_sensors(&temp_value, &hum_value, &bright_value);

			// sender status, 
			send_status(temp_value, hum_value, bright_value); //Kommenter ut om må debuge
			
			
			//usart_pi_transmit_char_array("SOIL:76-FAN:700");
			

		}
		
		/*
		// Leser tre 16-bit registre fra VEML7700.
		// NB: Hvis VEML ikke gir ACK, er disse verdiene ikke gyldige.
		uint16_t conf = veml7700_read_register16(0x00);  // config-register
		uint16_t als  = veml7700_read_register16(0x04);  // rå lysverdi
		uint16_t id   = veml7700_read_register16(0x07);  // ID-register

		sprintf(tx_msg, "CONF:%04X ALS:%04X ID:%04X\r\n", conf, als, id);
		usart_pi_transmit_char_array(tx_msg);

		_delay_ms(2000);

		// Tester riktig VEML7700-adresse.
		// Driveren bruker 7-bit adresse, så 0x10 er riktig.
		// Inni i2c_start_write() blir 0x10 gjort om til write-adressen 0x20.
		if (i2c_start_write(0x10))
		{
			usart_pi_transmit_char_array("ACK 0x10\r\n");
		}
		else
		{
			usart_pi_transmit_char_array("NO ACK 0x10\r\n");
		}
		i2c_stop();

		_delay_ms(500);

		// Ekstra test for å sjekke adresseforvirring.
		// Dette er egentlig IKKE riktig VEML-adresse i denne driveren.
		// 0x20 blir shiftet inni driveren og sendt som 0x40.
		if (i2c_start_write(0x20))
		{
			usart_pi_transmit_char_array("ACK 0x20\r\n");
		}
		else
		{
			usart_pi_transmit_char_array("NO ACK 0x20\r\n");
		}
		i2c_stop();

		_delay_ms(2000);

		// Scanner hele I2C-bussen etter enheter som svarer med ACK.
		usart_pi_transmit_char_array("SCAN START\r\n");

		for (uint8_t addr = 1; addr < 127; addr++)
		{
			if (i2c_start_write(addr))
			{
				sprintf(tx_msg, "FOUND: 0x%02X\r\n", addr);
				usart_pi_transmit_char_array(tx_msg);
			}

			i2c_stop();
			_delay_ms(5);
		}

		usart_pi_transmit_char_array("SCAN DONE\r\n\r\n");
		_delay_ms(3000);
		*/
	}
}