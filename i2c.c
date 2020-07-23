#include <avr/io.h>
#include "i2c.h"
#include "errorno.h"

uint8_t I2CInit(void)
{
	TWBR = 18; // 100 kHZ
	TWCR |= (1<<TWEN);	/* Enable TWI */
	return ESUCCESS;
}

uint8_t I2CStart(uint8_t addr)
{
	uint8_t i = 0;
	TWCR = (1 << TWINT) | (1 << TWEN) |
		   (1 << TWSTA) | (0 << TWSTO);	/* Start */
	while (!(TWCR & (1<<TWINT))) {				/* Wait for TWINT */
		if (i++ > 250)	/* Avoid endless loop */
			return EI2CSTARTLOOP;
	}

	return I2CWriteByte(addr);
}

uint8_t I2CStop(void)
{
	uint8_t i = 0;
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);	/* Stop */
	while (TWCR & (1<<TWSTO)) {					/* Wait for TWSTO */
		if (i++ > 250)	/* Avoid endless loop */
			return EI2CSTOPLOOP;
	}
	return ESUCCESS;
}

uint8_t I2CWriteByte(uint8_t data)
{
	uint8_t i = 0;
	TWDR = data;
	TWCR |= (1<<TWEA);
	TWCR = (1<<TWEN) | (1<<TWINT);	/* Start data transfer */

	while (!(TWCR & (1<<TWINT))) {	/* Wait for finish */
		if (i++ > 250)	/* Avoid endless loop */
			return EI2CWRITELOOP;
	}

	if (TWSR_STA != 0x18 && TWSR_STA != 0x28 && TWSR_STA != 0x40)
		return EI2CWRITE;
	return ESUCCESS;
}

uint8_t I2CReadByte(uint8_t *data, uint8_t ack)
{
	uint8_t i = 0;
	if (ack)
		TWCR |= (1<<TWEA);
	else
		TWCR &= ~(1<<TWEA);
	TWCR |= (1 << TWINT) | (1 << TWEN);

	while (!(TWCR & (1<<TWINT))) {	/* Wait for finish */
		if (i++ > 250)	/* Avoid endless loop */
			return TWSR;
	}

	if (TWSR_STA != 0x58 && TWSR_STA != 0x50)
		return EI2CREAD;

	*data = TWDR;
	return ESUCCESS;
}

uint8_t I2CScanBus(uint8_t addr)
{
	uint8_t status;

	status = I2CStart(addr);

	I2CStop();

	return status;
}
