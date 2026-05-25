/*
 * veml7700.h
 *
 * Created: 05.04.2026 13:50:58
 *  Author: Elisabeth
 */ 



#ifndef VEML7700_H_
#define VEML7700_H_

#include <stdint.h>

// ---------------------- I2C adresse ----------------------
#define VEML7700_ADDR 0x10


// ---------------------- registeradresser ----------------------
#define VEML7700_REG_CONFIG 0x00
#define VEML7700_REG_ALS    0x04


// ---------------------- funksjoner ----------------------
void veml7700_init(void);
uint16_t veml7700_read_raw_als(void);
uint16_t veml7700_read_register16(uint8_t reg_addr);

#endif /* VEML7700_H_ */