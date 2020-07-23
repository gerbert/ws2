#ifndef _I2C_H_
#define _I2C_H_

#include <inttypes.h>
#include <util/twi.h>

#define TWSR_STA	(TWSR & 0xF8)

#define I2C_NOACK	0
#define I2C_ACK		1
#define I2C_READ	1

uint8_t I2CInit(void);

uint8_t I2CStart(uint8_t addr);
uint8_t I2CStop(void);

uint8_t I2CWriteByte(uint8_t data);
uint8_t I2CReadByte(uint8_t *data, uint8_t ack);
uint8_t I2CScanBus(uint8_t addr);

#endif /* _I2C_H_ */
