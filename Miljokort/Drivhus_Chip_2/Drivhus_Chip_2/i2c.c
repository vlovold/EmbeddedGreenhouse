/*
 * i2c.c
 *
 * Created: 05.04.2026 11:36:57
 *  Author: Elisabeth
 */

#define F_CPU 4000000UL

#include <avr/io.h>
#include <stdint.h>
#include "i2c.h"


// ---------------------- starter TWI0 som I2C-master ----------------------
void i2c_init(void)
{
	// setter SDA og SCL som utganger
	I2C_PORT.DIRSET = (1 << SDA_POS) | (1 << SCL_POS);

	// setter I2C input level på TWI-modulen
	TWI0.CTRLA |= TWI_INPUTLVL_I2C_gc;

	// slår av TWI mens vi konfigurerer
	TWI0.MCTRLA &= ~TWI_ENABLE_bm;

	// setter baudrate
	TWI0.MBAUD = I2C_BAUD;

	// slår på TWI i master/host-modus
	TWI0.MCTRLA = TWI_ENABLE_bm | TWI_SMEN_bm;

	// setter buss-tilstand til idle
	TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}


// ---------------------- sender start + slaveadresse med write ----------------------
// returnerer 1 hvis OK
// returnerer 0 hvis timeout eller manglende ACK
uint8_t i2c_start_write(uint8_t device_addr)
{
	uint16_t timeout = 0;

	// sender slaveadresse + write-bit
	TWI0.MADDR = (device_addr << 1) | I2C_WRITE;

	// venter til modulen holder klokka eller timeout skjer
	while (!(TWI0.MSTATUS & TWI_CLKHOLD_bm))
	{
		timeout++;
		if (timeout > 60000)
		{
			return 0;
		}
	}

	// hvis slaven ikke svarte med ACK, returner feil
	if (TWI0.MSTATUS & TWI_RXACK_bm)
	{
		return 0;
	}

	return 1;
}


// ---------------------- sender start + slaveadresse med read ----------------------
// returnerer 1 hvis OK
// returnerer 0 hvis timeout eller manglende ACK
uint8_t i2c_start_read(uint8_t device_addr)
{
	uint16_t timeout = 0;

	// sender slaveadresse + read-bit
	TWI0.MADDR = (device_addr << 1) | I2C_READ;

	// venter til modulen holder klokka eller timeout skjer
	while (!(TWI0.MSTATUS & TWI_CLKHOLD_bm))
	{
		timeout++;
		if (timeout > 60000)
		{
			return 0;
		}
	}

	// hvis slaven ikke svarte med ACK, returner feil
	if (TWI0.MSTATUS & TWI_RXACK_bm)
	{
		return 0;
	}

	return 1;
}


// ---------------------- skriver én byte på I2C-bussen ----------------------
// returnerer 1 hvis OK
// returnerer 0 hvis timeout eller manglende ACK
uint8_t i2c_write_byte(uint8_t data)
{
	uint16_t timeout = 0;

	// venter til modulen holder klokka og er klar til neste byte
	while (!(TWI0.MSTATUS & TWI_CLKHOLD_bm))
	{
		timeout++;
		if (timeout > 60000)
		{
			return 0;
		}
	}

	// legger data i dataregisteret
	TWI0.MDATA = data;

	// venter til modulen holder klokka igjen etter sending
	timeout = 0;
	while (!(TWI0.MSTATUS & TWI_CLKHOLD_bm))
	{
		timeout++;
		if (timeout > 60000)
		{
			return 0;
		}
	}

	// hvis slaven ikke svarte med ACK, returner feil
	if (TWI0.MSTATUS & TWI_RXACK_bm)
	{
		return 0;
	}

	return 1;
}


// ---------------------- leser én byte og sender ACK ----------------------
// brukes når det kommer flere byte etter denne
uint8_t i2c_read_byte_ack(void)
{
	uint16_t timeout = 0;

	// setter ACK fordi vi vil lese flere byte
	TWI0.MCTRLB &= ~TWI_ACKACT_bm;

	// venter til en byte er mottatt
	while (!(TWI0.MSTATUS & TWI_RIF_bm))
	{
		timeout++;
		if (timeout > 60000)
		{
			return 0;
		}
	}

	return TWI0.MDATA;
}


// ---------------------- leser én byte og sender NACK ----------------------
// brukes når dette er siste byte
uint8_t i2c_read_byte_nack(void)
{
	uint16_t timeout = 0;

	// setter NACK fordi dette er siste byte
	TWI0.MCTRLB |= TWI_ACKACT_bm;

	// venter til en byte er mottatt
	while (!(TWI0.MSTATUS & TWI_RIF_bm))
	{
		timeout++;
		if (timeout > 60000)
		{
			return 0;
		}
	}

	return TWI0.MDATA;
}


// ---------------------- avslutter I2C-transaksjonen ----------------------
void i2c_stop(void)
{
	// sender STOP på bussen
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}


// ---------------------- skriver én verdi til ett register ----------------------
void i2c_write_register(uint8_t device_addr, uint8_t reg_addr, uint8_t data)
{
	// velger enheten med write
	if (!i2c_start_write(device_addr))
	{
		i2c_stop();
		return;
	}

	// sender registeradresse
	if (!i2c_write_byte(reg_addr))
	{
		i2c_stop();
		return;
	}

	// sender data som skal skrives til registeret
	if (!i2c_write_byte(data))
	{
		i2c_stop();
		return;
	}

	// avslutter overføringen
	i2c_stop();
}


// ---------------------- leser én verdi fra ett register ----------------------
uint8_t i2c_read_register(uint8_t device_addr, uint8_t reg_addr)
{
	uint8_t data = 0;

	// velger enheten med write for å sende registeradresse
	if (!i2c_start_write(device_addr))
	{
		i2c_stop();
		return 0;
	}

	// sender registeret vi vil lese fra
	if (!i2c_write_byte(reg_addr))
	{
		i2c_stop();
		return 0;
	}

	// gjør en ny start og velger enheten med read
	if (!i2c_start_read(device_addr))
	{
		i2c_stop();
		return 0;
	}

	// leser siste og eneste byte med NACK
	data = i2c_read_byte_nack();

	// avslutter overføringen
	i2c_stop();

	return data;
}


// ---------------------- leser flere byte fra sammenhengende registre ----------------------
void i2c_read_registers(uint8_t device_addr, uint8_t start_reg, uint8_t *buffer, uint8_t length)
{
	if (length == 0)
	{
		return;
	}

	// velger enheten med write for å sende startregister
	if (!i2c_start_write(device_addr))
	{
		i2c_stop();
		return;
	}

	// sender første registeradresse
	if (!i2c_write_byte(start_reg))
	{
		i2c_stop();
		return;
	}

	// gjør en ny start og velger enheten med read
	if (!i2c_start_read(device_addr))
	{
		i2c_stop();
		return;
	}

	// leser alle byte
	for (uint8_t i = 0; i < length; i++)
	{
		if (i < (length - 1))
		{
			buffer[i] = i2c_read_byte_ack();
		}
		else
		{
			buffer[i] = i2c_read_byte_nack();
		}
	}

	// avslutter overføringen
	i2c_stop();
}