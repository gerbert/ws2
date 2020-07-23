#include <stdlib.h>
#include <string.h>
#include <util/twi.h>
#include "i2c.h"
#include "24c32.h"
#include "errorno.h"

#define EEPROM_SIZE				0x1000				// 4096 bytes
#define EEPROM_PAGE_CNT			0x80				// 128 pages
#define EEPROM_PAGE_SIZE		0x20				// 32 bytes
#define EEPROM_MU_ADDR_STEP		EEPROM_PAGE_SIZE	// 20h
#define EEPROM_MU_WR_SIZE		EEPROM_PAGE_SIZE	// 32 bytes per write

// By-default, should be 0xAE
/*
 * By-default, read address is 0xAE,
 * and write address should be 0xAE + 1 = 0xAF
 */
static uint8_t at24_addr;

static struct eeprom_data eepd;

void AT24_write(uint8_t *data) {

}

uint8_t AT24_read(struct eeprom_data *data) {
	uint8_t ret = ESUCCESS;
	uint8_t counter = 0;
	uint8_t *p;
	struct eeprom_data *tmp = (struct eeprom_data *)malloc(sizeof(tmp));

	memset(tmp, 0, EEPROM_MU_WR_SIZE);

	// Send EEPROM's i2c address + raise read flag
	ret = I2CStart(at24_addr | TW_READ);
	if (ret > 0)
		return ret;

	// Read one page
	do {
		if (counter + 1 == EEPROM_MU_WR_SIZE)
			ret = I2CReadByte(p, I2C_NOACK);
		else
			ret = I2CReadByte(p, I2C_ACK);

		if (ret > 0)
			return EEEPDATAREAD;
		tmp += *p++;
		counter++;
	} while (counter != EEPROM_MU_WR_SIZE);
	I2CStop();

	data = tmp;
	return ESUCCESS;
}

uint8_t AT24_format(void) {
	/*
	 * Declare a pointer for 1-byte write over i2c bus.
	 * It will point to an address of our structure and
	 * we will shift it to one byte until we reach the
	 * endpoint of our structure.
	 */
	uint8_t *p;
	uint8_t counter = 0;
	uint8_t at24_waddress = 0;
	uint16_t at24_paddress = 0;
	/*
	 * If any error during write - we will return error
	 * number, otherwise - 0.
	 */
	uint8_t ret = ESUCCESS;

	memset(&eepd, 0, EEPROM_MU_WR_SIZE);

	eepd.counter = 1;
	eepd.last_sync = 2;
	eepd.next_sync = 3;
	eepd.next_page = 4;

	// Set AT24C32 i2c address and raise write flag
	at24_waddress = (at24_addr | TW_WRITE);

	for (uint8_t i = 0; i < EEPROM_PAGE_CNT; i++) {
		p = (uint8_t *)&eepd; // We set start addres
		if (p != NULL) {
			// Start transmission. Connect to the device
			ret = I2CStart(at24_waddress);
			if (ret > 0)
				return ret;

			ret = I2CWriteByte(at24_paddress >> 8);
			if (ret > 0)
				return EEEPADDRMSB;
			ret = I2CWriteByte(at24_paddress & 0xFF);
			if (ret > 0)
				return EEEPADDRLSB;

			for (uint8_t k = 0; k < sizeof(eepd); k++) {
				ret = I2CWriteByte(*p);
				if (ret > 0)
					return ret;
				p++;
			}
			// Stop transmission
			I2CStop();
			at24_paddress += EEPROM_MU_ADDR_STEP;
		} else
			return ENULLPOINTER;
	}

	return counter;
}

/**
 * @brief AT24_init - perform basic initialization
 *				of the driver
 * @param addr - address of EEPROM device on
 *				i2c bus
 */
uint8_t AT24_init(uint8_t addr) {
	/**
	 * TODO: we need to check eeprom for any real data
	 * before formatting it or reading and checking
	 * whether we need time correction or not
	 */
	memset(&eepd, 0, EEPROM_MU_WR_SIZE);
	at24_addr = addr;

	return 0;
}
