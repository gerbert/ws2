#ifndef _MAIN_H_
#define _MAIN_H_

#include <avr/pgmspace.h>

#define FONT_COUNT				7
#define ARRAY_SIZE(x)			((sizeof(x)) / sizeof(x[0]))
#define CPU_PRESCALE(n)			(CLKPR = 0x80, CLKPR = (n))

enum {
	ICO_TEMP_INSIDE		= 1,
	ICO_TEMP_OUTSIDE,
	ICO_CLOCK,
	ICO_PRESSURE,
	ICO_HUMIDITY,
	ICO_SAT_ONLINE,
	ICO_SAT_OFFLINE
};

const char font[FONT_COUNT][8] PROGMEM = {
	{
		0x04, 0x0E, 0x1F, 0x11, 0x15, 0x11, 0x1F, 0x00, // Inside,			1
	},
	{
		0x1F, 0x1B, 0x11, 0x11, 0x15, 0x11, 0x1F, 0x00	// Outside,			2
	},
	{
		0x00, 0x0E, 0x15, 0x17, 0x11, 0x0E, 0x00, 0x00	// Clock,			3
	},
	{
		0x00, 0x0E, 0x19, 0x09, 0x0E, 0x08, 0x08, 0x10	// Pressure,		4
	},
	{
		0x00, 0x0A, 0x15, 0x15, 0x15, 0x0E, 0x08, 0x10	// Humidity,		5
	},
	{
		0x15, 0x00, 0x10, 0x12, 0x0c, 0x04, 0x03, 0x00	// SAT on-line,		6
	},
	{
		0x0a, 0x1f, 0x0f, 0x0d, 0x13, 0x1b, 0x1c, 0x1f	// SAT off-line,	7
	}
};

#endif /* _MAIN_H_ */
