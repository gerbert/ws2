#include <util/delay.h>
#include <compat/twi.h>
#include <util/twi.h>
#include <avr/pgmspace.h>
#include "lcd.h"
#include "i2c.h"

#define PCF8574T_ID			0x40
#define PCF8574T_ADR		0x00

/*
 * MSB = data
 * LSB:
 * 3 - BL
 * 2 - E
 * 1 - RW
 * 0 - RS
 */
#pragma pack(push, 1)
typedef union _wdata {
	struct {
		uint8_t rs : 1;
		uint8_t rw : 1;
		uint8_t en : 1;
		uint8_t backlight : 1;
		uint8_t data : 4;
	};
	uint8_t value;
} wdata;
#pragma pack(pop)
static wdata lcd;

#ifdef TWO_LINE_LCD
static const uint8_t pos[2][16] PROGMEM = {
	{
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F
	},
	{
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
		0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
	},
};
#else
static const uint8_t pos[16] PROGMEM = {
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7
};
#endif

static const uint8_t cgram_char[8] PROGMEM = {
	0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
};

static void _strobe(void)
{
	lcd.en = enable;
	I2CWriteByte(lcd.value);
	_delay_us(10);
	lcd.en = disable;
	I2CWriteByte(lcd.value);
	_delay_us(10);
}

static void _putchar(char data)
{
	uint8_t ret = 0x00;

	lcd.rs = enable;
	lcd.rw = disable;

	ret = (data & 0xf0) >> 4;
	lcd.data = ret;
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);

	ret = ((data << 4) & 0xf0) >> 4;
	lcd.data = ret;
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);
}

#ifdef TWO_LINE_LCD
static void setPosition(uint8_t line, uint8_t index)
#else
static void setPosition(uint8_t index)
#endif
{
	lcd.rs = disable;
	lcd.rw = disable;

	// Select DDRAM address
#ifdef TWO_LINE_LCD
	lcd.data = ((pgm_read_byte(&pos[line][index])) >> 4);
#else
	lcd.data = ((pgm_read_byte(&pos[index])) >> 4);
#endif
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);

#ifdef TWO_LINE_LCD
	lcd.data = ((pgm_read_byte(&pos[line][index])) & 0x0F);
#else
	lcd.data = ((pgm_read_byte(&pos[index])) & 0x0F);
#endif
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);
}

#ifdef TWO_LINE_LCD
static uint8_t lcdPrintChar(char data, uint8_t line, uint8_t start)
#else
static uint8_t lcdPrintChar(char data, uint8_t start)
#endif
{
	if (start > 15)
		return -1;

	I2CStart(LCD_I2C_ADDR);

#ifdef TWO_LINE_LCD
	setPosition(line, start);
#else
	setPosition(start);
#endif
	_putchar(data);

	I2CStop();

	return 0;
}

#ifdef TWO_LINE_LCD
static uint8_t lcdPrintString(char *data, uint8_t line,
							  uint8_t start, uint8_t size)
#else
static uint8_t lcdPrintString(char *data, uint8_t start, uint8_t size)
#endif
{
	uint8_t ret = 0;

	if ((start + size) > 16)
		return -1;

	I2CStart(LCD_I2C_ADDR);

	for (uint8_t i = start; i < (start + size); i++) {
#ifdef TWO_LINE_LCD
		setPosition(line, i);
#else
		setPosition(i);
#endif
		_putchar(*data++);
	}

	I2CStop();

	return ret;
}

static void lcdBacklight(uint8_t sw)
{
	I2CStart(LCD_I2C_ADDR);

	lcd.rs = disable;
	lcd.rw = disable;
	lcd.backlight = sw;
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);

	I2CStop();
}

static void lcdClear(void)
{
	I2CStart(LCD_I2C_ADDR);

	lcd.rs = disable;
	lcd.rw = disable;
	lcd.data = 0x00;
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);
	lcd.data = 0x01;
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);

	I2CStop();
}

/**
 * Memory MAP for CGRAM
 * Pattern 1:	0x00-0x07, &40h
 * Pattern 2:	0x08-0x0F, &48h
 * Pattern 3:	0x10-0x17, &50h
 * Pattern 4:	0x18-0x1F, &58h
 * Pattern 5:	0x20-0x27, &60h
 * Pattern 6:	0x28-0x2F, &68h
 * Pattern 7:	0x30-0x37, &70h
 * Pattern 8:	0x38-0x3F, &78h
 */

static void writeCustomChar(const char *custom, uint8_t pos)
{
	I2CStart(LCD_I2C_ADDR);

	uint8_t cgchar = pgm_read_byte(&cgram_char[pos]);

	lcd.rs = disable;
	lcd.rw = disable;
	lcd.data = (cgchar >> 4);
	I2CWriteByte(lcd.value);
	_strobe();
	pos &= 0x07;
	lcd.data = (cgchar & 0x0f);
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_us(1);

	for (uint8_t i = 0; i < 8; i++) {
		_putchar(*(custom + i));
	}

	I2CStop();
}

static struct LCD screen = {
	.backlight = lcdBacklight,
	.clear = lcdClear,
	.putCustomChar = writeCustomChar,
	.printString = lcdPrintString,
	.printChar = lcdPrintChar
};

struct LCD *lcdInit(void)
{
	_delay_ms(50);
	lcd.value = 0x00;

	I2CStart(LCD_I2C_ADDR);

	lcd.data = 0x03;
	lcd.rs = disable;
	lcd.rw = disable;
	I2CWriteByte(lcd.value);
	_strobe();
	_delay_ms(20);
	_strobe();
	_delay_ms(5);
	_strobe();
	_delay_us(120);
	_strobe();

	// Function set
	lcd.data = 0x02;
	I2CWriteByte(lcd.value);
	_strobe();

	lcd.data = 0x02;
	I2CWriteByte(lcd.value);
	_strobe();
	lcd.data = 0x08;
	I2CWriteByte(lcd.value);
	_strobe();

	// Display on/off control
	lcd.data = 0x00;
	I2CWriteByte(lcd.value);
	_strobe();
	lcd.data = 0x0C;
	I2CWriteByte(lcd.value);
	_strobe();

	// Display clear
	lcd.data = 0x00;
	I2CWriteByte(lcd.value);
	_strobe();
	lcd.data = 0x01;
	I2CWriteByte(lcd.value);
	_strobe();

	// Entry mode set
	lcd.data = 0x00;
	I2CWriteByte(lcd.value);
	_strobe();
	lcd.data = 0x06;
	I2CWriteByte(lcd.value);
	_strobe();

	I2CStop();

	return &screen;
}
