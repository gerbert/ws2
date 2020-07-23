#ifndef _LCD_H_
#define _LCD_H_

#define LCD_I2C_ADDR		0x40
#define enable				1
#define disable				0

#ifdef TWO_LINE_LCD
	#define NUM_LINES		2
#else
	#define NUM_LINES		1
#endif

struct LCD {
	void (*backlight)(uint8_t sw);
	void (*clear)(void);
	void (*putCustomChar)(const char *custom, uint8_t pos);
#ifdef TWO_LINE_LCD
	uint8_t (*printChar)(char data, uint8_t line, uint8_t start);
	uint8_t (*printString)(char *data, uint8_t line, uint8_t start, uint8_t size);
#else
	uint8_t (*printChar)(char data, uint8_t start);
	uint8_t (*printString)(char *data, uint8_t start, uint8_t size);
#endif
};

struct LCD *lcdInit(void);

#endif /* _LCD_H_ */
