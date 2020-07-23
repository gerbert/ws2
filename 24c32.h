#ifndef _24C32_H_
#define _24C32_H_

/**
 * AT24C32 Internal memory organization:
 * each page is 32 Bytes long, so we have
 * 128 pages. We will try to organize data
 * to fit into one page. Reserved fields will
 * allow us to use them without taking care
 * about going out of the page block.
 * Total memory available equal to 4k
 * (32kbit for AT24C32).
 */
#pragma pack(push, 1)
struct eeprom_data {
	uint32_t    last_sync;
	uint32_t    next_sync;
	uint32_t    __reserved_1;
	uint32_t    __reserved_2;
	uint32_t    __reserved_3;
	uint32_t    __reserved_4;
	uint32_t    __reserved_5;
	uint16_t    counter;
	uint16_t    next_page;
};
#pragma pack(pop)

void AT24_write(uint8_t *data);
uint8_t AT24_read(struct eeprom_data *data);
uint8_t AT24_format(void);
uint8_t AT24_init(uint8_t addr);

#endif /* _24C32_H_ */
