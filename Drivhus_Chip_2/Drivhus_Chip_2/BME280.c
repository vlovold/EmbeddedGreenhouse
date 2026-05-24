/*
 * BME280.c
 *
 * Created: 05.04.2026 13:51:41
 *  Author: Bruker
 */

#include <avr/io.h>
#include <stdint.h>
#include "i2c.h"
#include "bme280.h"


// ---------------------- kalibreringsverdier fra BME280 ----------------------
// disse leses én gang fra sensoren og brukes i kompensasjonsformlene
static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;

static uint8_t  dig_H1;
static int16_t  dig_H2;
static uint8_t  dig_H3;
static int16_t  dig_H4;
static int16_t  dig_H5;
static int8_t   dig_H6;

// ---------------------- mellomverdi brukt i kompensasjon ----------------------
// databladet sier at t_fine fra temperaturkompensasjonen brukes videre i fuktighet
static int32_t t_fine;


// ---------------------- starter BME280 med enkel konfigurasjon ----------------------
// osrs_h = 001  -> humidity oversampling x1
// osrs_t = 001  -> temperature oversampling x1
// osrs_p = 000  -> pressure hoppes over
// mode   = 11   -> normal mode
// config = 0x00 -> ingen filter, standard oppsett
void bme280_init(void)
{
	// setter humidity oversampling til x1
	// viktig: denne blir først aktiv etter skriving til ctrl_meas
	i2c_write_register(BME280_ADDR, BME280_REG_CTRL_HUM, 0x01);

	// setter config-register til 0
	// ingen filter, ingen spesielle valg
	i2c_write_register(BME280_ADDR, BME280_REG_CONFIG, 0x00);

	// setter temperatur oversampling x1, pressure av, normal mode
	// osrs_t = 001, osrs_p = 000, mode = 11
	// blir: 0010 0011 = 0x23
	i2c_write_register(BME280_ADDR, BME280_REG_CTRL_MEAS, 0x23);

	// leser kalibreringsverdier etter init
	bme280_read_calibration();
}


// ---------------------- leser chip-id fra BME280 ----------------------
// skal være 0x60 hvis kommunikasjonen virker
uint8_t bme280_read_id(void)
{
	return i2c_read_register(BME280_ADDR, BME280_REG_ID);
}


// ---------------------- leser rå temperaturverdi fra BME280 ----------------------
// temperaturdata ligger i 0xFA, 0xFB og 0xFC
// disse 3 bytene settes sammen til én 20-bits verdi
uint32_t bme280_read_raw_temp(void)
{
	uint8_t buffer[3];      // buffer til temperatur-byte
	uint32_t raw_temp;      // sammensatt rå temperaturverdi

	// leser 3 byte fra temperaturregistrene
	i2c_read_registers(BME280_ADDR, BME280_REG_TEMP_MSB, buffer, 3);

	// setter sammen til 20-bit verdi
	raw_temp  = ((uint32_t)buffer[0] << 12);
	raw_temp |= ((uint32_t)buffer[1] << 4);
	raw_temp |= ((uint32_t)buffer[2] >> 4);

	return raw_temp;
}


// ---------------------- leser rå fuktighetsverdi fra BME280 ----------------------
// humiditydata ligger i 0xFD og 0xFE
// disse 2 bytene settes sammen til én 16-bits verdi
uint16_t bme280_read_raw_hum(void)
{
	uint8_t buffer[2];      // buffer til fuktighets-byte
	uint16_t raw_hum;       // sammensatt rå fuktighetsverdi

	// leser 2 byte fra fuktighetsregistrene
	i2c_read_registers(BME280_ADDR, BME280_REG_HUM_MSB, buffer, 2);

	// setter sammen til 16-bit verdi
	raw_hum  = ((uint16_t)buffer[0] << 8);
	raw_hum |= ((uint16_t)buffer[1]);

	return raw_hum;
}


// ---------------------- leser rå temperatur og fuktighet med én stor burst read ----------------------
// leser fra 0xF7 til 0xFE (8 byte totalt)
// dette inkluderer også trykkdata, men de ignoreres her
void bme280_read_raw_all(uint32_t *raw_temp, uint16_t *raw_hum)
{
	uint8_t buffer[8];   // buffer for alle 8 byte (pressure + temp + hum)

	// leser hele måleblokken i én operasjon
	i2c_read_registers(BME280_ADDR, BME280_REG_PRESS_MSB, buffer, 8);

	// buffer[3] = TEMP_MSB (0xFA)
	// buffer[4] = TEMP_LSB (0xFB)
	// buffer[5] = TEMP_XLSB (0xFC)
	*raw_temp  = ((uint32_t)buffer[3] << 12);
	*raw_temp |= ((uint32_t)buffer[4] << 4);
	*raw_temp |= ((uint32_t)buffer[5] >> 4);

	// buffer[6] = HUM_MSB (0xFD)
	// buffer[7] = HUM_LSB (0xFE)
	*raw_hum  = ((uint16_t)buffer[6] << 8);
	*raw_hum |= ((uint16_t)buffer[7]);
}


// ---------------------- leser kalibreringsverdier fra BME280 ----------------------
// temperaturkalibrering ligger fra 0x88 til 0x8D
// fuktighetskalibrering ligger i 0xA1 og 0xE1 til 0xE7
void bme280_read_calibration(void)
{
	uint8_t temp_calib[6];   // buffer for dig_T1, dig_T2, dig_T3
	uint8_t hum_calib[7];    // buffer for dig_H2 til dig_H6

	// leser 6 byte fra temperaturkalibreringen
	i2c_read_registers(BME280_ADDR, BME280_REG_DIG_T1_LSB, temp_calib, 6);

	// setter sammen dig_T1, dig_T2 og dig_T3
	dig_T1 = (uint16_t)(temp_calib[0] | (temp_calib[1] << 8));
	dig_T2 = (int16_t)(temp_calib[2] | (temp_calib[3] << 8));
	dig_T3 = (int16_t)(temp_calib[4] | (temp_calib[5] << 8));

	// leser dig_H1 fra eget register
	dig_H1 = i2c_read_register(BME280_ADDR, BME280_REG_DIG_H1);

	// leser 7 byte fra fuktighetskalibreringen
	i2c_read_registers(BME280_ADDR, BME280_REG_DIG_H2_LSB, hum_calib, 7);

	// setter sammen dig_H2 og dig_H3
	dig_H2 = (int16_t)(hum_calib[0] | (hum_calib[1] << 8));
	dig_H3 = hum_calib[2];

	// dig_H4 består av 0xE4 og nederste nibble av 0xE5
	dig_H4 = (int16_t)((hum_calib[3] << 4) | (hum_calib[4] & 0x0F));
	if (dig_H4 & 0x0800)                  // hvis bit 11 er satt, er tallet negativt
	{
		dig_H4 |= 0xF000;                 // fyller inn 1-ere i de øverste 4 bitene
	}

	// dig_H5 består av øverste nibble av 0xE5 og 0xE6
	dig_H5 = (int16_t)((hum_calib[5] << 4) | (hum_calib[4] >> 4));
	if (dig_H5 & 0x0800)                  // hvis bit 11 er satt, er tallet negativt
	{
		dig_H5 |= 0xF000;                 // fyller inn 1-ere i de øverste 4 bitene
	}

	// dig_H6 er signed 8-bit
	dig_H6 = (int8_t)hum_calib[6];
}


// ---------------------- kompenserer temperatur ----------------------
// returnerer temperatur i 0.01 °C
// eksempel: 5123 betyr 51.23 °C
int32_t bme280_compensate_temp(uint32_t raw_temp)
{
	int32_t var1;
	int32_t var2;
	int32_t T;

	var1 = ((((int32_t)raw_temp >> 3) - ((int32_t)dig_T1 << 1)) * ((int32_t)dig_T2)) >> 11;

	var2 = ((((((int32_t)raw_temp >> 4) - ((int32_t)dig_T1)) *
	          (((int32_t)raw_temp >> 4) - ((int32_t)dig_T1))) >> 12) *
	          ((int32_t)dig_T3)) >> 14;

	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;

	return T;
}


// ---------------------- kompenserer fuktighet ----------------------
// returnerer fuktighet i Q22.10-format
// eksempel: 47445 betyr 47445 / 1024 = 46.33 %RH
uint32_t bme280_compensate_hum(uint16_t raw_hum)
{
	int32_t v_x1_u32r;

	v_x1_u32r = (t_fine - ((int32_t)76800));

	v_x1_u32r = (((((int32_t)raw_hum << 14) - (((int32_t)dig_H4) << 20) -
	               (((int32_t)dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
	             (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
	                  (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
	                  ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14);

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
	               ((int32_t)dig_H1)) >> 4));

	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (uint32_t)(v_x1_u32r >> 12);
}


// ---------------------- leser rådata og returnerer kompenserte verdier ----------------------
// temperatur returneres i 0.01 °C
// fuktighet returneres i Q22.10-format
void bme280_read_compensated(int32_t *temp_c_x100, uint32_t *hum_rh_x1024)
{
	uint32_t raw_temp;
	uint16_t raw_hum;

	// leser rå temperatur og rå fuktighet
	bme280_read_raw_all(&raw_temp, &raw_hum);

	// kompenserer temperatur først
	// denne setter også t_fine som brukes i fuktighet
	*temp_c_x100 = bme280_compensate_temp(raw_temp);

	// kompenserer fuktighet etterpå
	*hum_rh_x1024 = bme280_compensate_hum(raw_hum);
}

