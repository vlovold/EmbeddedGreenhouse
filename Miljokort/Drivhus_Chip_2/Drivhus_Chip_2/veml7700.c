/*
 * veml7700.c
 *
 * Created: 05.04.2026 13:51:28
 *  Author: Elisabeth
 */ 



#include <avr/io.h>
#include <stdint.h>
#include "i2c.h"
#include "veml7700.h"


// ---------------------- skriver 16-bit verdi til et VEML7700-register ----------------------
// VEML7700 forventer data som LSB først og deretter MSB
static void veml7700_write_register16(uint8_t reg_addr, uint16_t data)
{
	uint8_t lsb = (uint8_t)(data & 0x00FF);         // nederste byte
	uint8_t msb = (uint8_t)((data >> 8) & 0x00FF);  // øverste byte

	// velger sensoren med write
	i2c_start_write(VEML7700_ADDR);

	// sender registeradresse
	i2c_write_byte(reg_addr);

	// sender data, først LSB og så MSB
	i2c_write_byte(lsb);
	i2c_write_byte(msb);

	// avslutter I2C-transaksjonen
	i2c_stop();
}


// ---------------------- leser 16-bit verdi fra et VEML7700-register ----------------------
// VEML7700 sender data som LSB først og deretter MSB
 uint16_t veml7700_read_register16(uint8_t reg_addr)
{
	uint8_t buffer[2];      // buffer til de to byte som leses
	uint16_t value;         // ferdig sammensatt 16-bit verdi

	// leser to byte fra ønsket register
	i2c_read_registers(VEML7700_ADDR, reg_addr, buffer, 2);

	// setter sammen LSB og MSB til én 16-bit verdi
	value  = (uint16_t)buffer[0];
	value |= ((uint16_t)buffer[1] << 8);

	return value;
}


// ---------------------- starter VEML7700 med enkel konfigurasjon ----------------------
// skriver 0x0000 til config-registeret
// dette gir enkel oppstart og sørger for at shutdown-biten ikke er satt
void veml7700_init(void)
{
	veml7700_write_register16(VEML7700_REG_CONFIG, 0x0000);
}


// ---------------------- leser rå lysverdi fra ALS-registeret ----------------------
// register 0x04 inneholder 16-bit ambient light data
uint16_t veml7700_read_raw_als(void)
{
	return veml7700_read_register16(VEML7700_REG_ALS);
}