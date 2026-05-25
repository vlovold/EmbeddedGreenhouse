

/*
 * i2c.h
 *
 * Enkel generell I2C-driver for AVR128DB32
 * Brukes av alle sensorer (VEML7700, BME280 osv.)
 */

#ifndef I2C_H_
#define I2C_H_

#include <avr/io.h>
#include <stdint.h>

// ---------------------- valg av I2C-pinner ----------------------
#define I2C_PORT PORTA
#define SDA_POS  2
#define SCL_POS  3

// ---------------------- retning på I2C-kommunikasjon ----------------------
#define I2C_WRITE 0      // skrive til slave
#define I2C_READ  1      // lese fra slave


// ---------------------- I2C hastighet ----------------------
// standard 100 kHz
#define I2C_FREQ 100000UL

// enkel fast verdi (kan justeres senere)
#define I2C_BAUD 75


// ---------------------- init ----------------------
void i2c_init(void);


// ---------------------- lavnivå funksjoner ----------------------
uint8_t i2c_start_write(uint8_t device_addr);   // start + adresse + write
uint8_t i2c_start_read(uint8_t device_addr);    // start + adresse + read

uint8_t i2c_write_byte(uint8_t data);           // send én byte

uint8_t i2c_read_byte_ack(void);             // les byte + ACK (flere kommer)
uint8_t i2c_read_byte_nack(void);            // les byte + NACK (siste byte)

void i2c_stop(void);                         // STOP på bussen


// ---------------------- høyere nivå funksjoner ----------------------
void i2c_write_register(uint8_t device_addr, uint8_t reg_addr, uint8_t data);

uint8_t i2c_read_register(uint8_t device_addr, uint8_t reg_addr);

void i2c_read_registers(uint8_t device_addr,
                        uint8_t start_reg,
                        uint8_t *buffer,
                        uint8_t length);

#endif /* I2C_H_ */