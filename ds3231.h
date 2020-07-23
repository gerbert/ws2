#ifndef __ds3231_h_
#define __ds3231_h_

// i2c slave address of the DS3231 chip
#define DS3231_I2C_ADDR				0xD0
#define DS3231_INTCN				0x4

struct ts {
	uint8_t sec;		/* seconds */
	uint8_t min;		/* minutes */
	uint8_t hour;		/* hours */
	uint8_t mday;		/* day of the month */
	uint8_t mon;		/* month */
	uint16_t year;		/* year */
	uint8_t wday;		/* day of the week */
	uint8_t yday;		/* day in the year */
	uint8_t isdst;		/* daylight saving time */
	uint8_t year_s;		/* year in short notation*/
};

struct DS3231 {
	struct ts *time;

	void (*set)(struct ts t);
	void (*get)(struct ts *t);
	void (*set_aging)(const int8_t val);
	int8_t (*get_aging)(void);
	float (*get_treg)(void);
	void (*set_a1)(const uint8_t s, const uint8_t mi, const uint8_t h,
				   const uint8_t d, const uint8_t *flags);
	void (*get_a1)(char *buf, const uint8_t len);
	void (*clear_a1f)(void);
	uint8_t (*triggered_a1)(void);
	void (*set_a2)(const uint8_t mi, const uint8_t h, const uint8_t d,
				   const uint8_t *flags);
	void (*get_a2)(char *buf, const uint8_t len);
	void (*clear_a2f)(void);
	uint8_t (*triggered_a2)(void);
};

struct DS3231 *DS3231_init(const uint8_t creg);

#endif /* __ds3231_h_*/
