/*
 * BME280.h
 *
 * Created: 05.04.2026 13:51:14
 *  Author: Elisabeth
 */

#ifndef BME280_H_
#define BME280_H_

#include <stdint.h>


// ---------------------- I2C adresse ----------------------
// SDO = GND gir adresse 0x76
#define BME280_ADDR 0x76


// ---------------------- ID register ----------------------
#define BME280_REG_ID        0xD0


// ---------------------- kontrollregistre ----------------------
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5


// ---------------------- temperaturregistre ----------------------
#define BME280_REG_TEMP_MSB   0xFA
#define BME280_REG_TEMP_LSB   0xFB
#define BME280_REG_TEMP_XLSB  0xFC


// ---------------------- fuktighetsregistre ----------------------
#define BME280_REG_HUM_MSB    0xFD
#define BME280_REG_HUM_LSB    0xFE


// ---------------------- trykk ----------------------
#define BME280_REG_PRESS_MSB   0xF7
#define BME280_REG_PRESS_LSB   0xF8
#define BME280_REG_PRESS_XLSB  0xF9


// ---------------------- kalibreringsregistere ----------------------
#define BME280_REG_DIG_T1_LSB  0x88
#define BME280_REG_DIG_H1      0xA1
#define BME280_REG_DIG_H2_LSB  0xE1


// ---------------------- funksjoner ----------------------
void bme280_init(void);
uint8_t bme280_read_id(void);

uint32_t bme280_read_raw_temp(void);
uint16_t bme280_read_raw_hum(void);
void bme280_read_raw_all(uint32_t *raw_temp, uint16_t *raw_hum);

// leser kalibreringsverdier fra sensoren
void bme280_read_calibration(void);

// kompenserer rå temperatur og returnerer temperatur i 0.01 °C
// eksempel: 5123 betyr 51.23 °C
int32_t bme280_compensate_temp(uint32_t raw_temp);

// kompenserer rå fuktighet og returnerer %RH i Q22.10-format
// eksempel: 47445 betyr 47445 / 1024 = 46.33 %RH
uint32_t bme280_compensate_hum(uint16_t raw_hum);

// enkel hjelpefunksjon som leser rådata og returnerer ferdige temperatur- og fuktighetsverdier
void bme280_read_compensated(int32_t *temp_c_x100, uint32_t *hum_rh_x1024);

#endif /* BME280_H_ */