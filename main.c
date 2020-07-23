#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "timer.h"
#include "errorno.h"
#include "main.h"
#include "i2c.h"
#include "usb/usb_serial.h"
#include "usart.h"
#include "lcd.h"
#include "bmp085.h"
#include "ds3231.h"
#include "nmea.h"
#include "dht22.h"

#define BUFFER_SIZE			128
#define SCREEN_BUFF			16
#define LCD_EMPTY_LINE		"                "

// Buffer with actual data received from UART
static char wbuf[BUFFER_SIZE];
// Buffer for temporary data
static char tbuf[BUFFER_SIZE];
// Buffer for USB output
static char ubuf[BUFFER_SIZE];
/*
 * USB output mask:
 *	- \r\n - carriage return, new line
 *	- $DATA - beginning of data section
 *	- %02d:%02d:%02d - hh:mm:ss, time
 *	- %03d - pressure
 *	- %02d - humidity
 *	- %02d.%1d - internal temperature
 *	- %02d.%1d - external temperature
 *	- \r\n - carriage return, new line
 *	- GPS - beginning of GPS output section
 *	- %s - GPS data from receiver
 * GPS data always ends with \r\n, so we don't need
 * to add ending chars to the line.
 */
#define USB_OUTPUT_MASK		"\r\n$DATA;%02d:%02d:%02d;%03d;%02d;%02d.%1d;%02d.%1d\r\n$GPS;%s"

static uint8_t intCounter = 0;
// Screen buffer
#ifdef TWO_LINE_LCD
	char pbuf[NUM_LINES][SCREEN_BUFF];
#else
	char pbuf[SCREEN_BUFF];
#endif

static long slPressure = 0;
static long slTemp = 0;

struct ts rtc_time;

typedef enum {
	ACTION_WRITE_SCREEN,	// Write data from buffer to the screen
	ACTION_ERASE_SCREEN		// Erase both, screen and buffer
} ScreenAction;

static void writeScreen(struct LCD *screen, ScreenAction flag) {
#ifdef TWO_LINE_LCD
	for (uint8_t i = 0; i < NUM_LINES; i++) {
		if (flag == ACTION_WRITE_SCREEN) {
			screen->printString(pbuf[i], i, 0, strlen(pbuf[i]));
			//memset(&pbuf[i], 0x20, SCREEN_BUFF);
		} else if (flag == ACTION_ERASE_SCREEN) {
			//memset(&pbuf[i], 0x20, SCREEN_BUFF);
			memcpy(&pbuf[i], LCD_EMPTY_LINE, SCREEN_BUFF);
			screen->printString(pbuf[i], i, 0, strlen(pbuf[i]));
		}
	}
#else
	if (flag == ACTION_WRITE_SCREEN) {
		screen->printString(pbuf, 0, 0, strlen(pbuf));
		//memset(&pbuf, 0x20, SCREEN_BUFF);
	} else if (flag == ACTION_ERASE_SCREEN) {
		for (uint8_t i = 0; i < SCREEN_BUFF; i++) {
			pbuf[i] = " ";
		}
		screen->printString(pbuf, 0, 0, strlen(pbuf));
    }
#endif
}

ISR(USART1_RX_vect) {
	char tmp = UDR1;

	if (intCounter < BUFFER_SIZE) {
		if (tmp != '\n') {
			tbuf[intCounter++] = tmp;
		} else {
			tbuf[intCounter] = '\0';
			intCounter = 0;
			PORTB &= ~_BV(PB0);
		}
	}
}

/*
 * This function reads custom chars from
 * CPU's Flash memory and returns it to
 * LCD screen
 */
static void initPgmFont(struct LCD *screen)
{
	for (uint8_t i = 0; i < FONT_COUNT; i++) {
		char ch[8];
		for (uint8_t k = 0; k < 8; k++) {
			ch[k] = pgm_read_byte(&font[i][k]);
		}
		screen->putCustomChar((const char *)&ch, i + 1);
	}
}

static uint8_t timeCorrection(struct DS3231 *rtc, struct GPS *gps)
{
	struct ts new_time;
	uint8_t gHours, gMinutes, gSeconds;
	uint8_t rtcHours, rtcMinutes, rtcSeconds;
	static uint16_t delay_counter = 0;

	if (!gps->gpsTimeHasFix)
		return EGPSTIMENOFIX;

	memset(&new_time, 0, sizeof(struct ts));
	memcpy(&new_time, &rtc_time, sizeof(struct ts));

	rtcHours = rtc_time.hour;
	rtcMinutes = rtc_time.min;
	rtcSeconds = rtc_time.sec;

	gHours = gps->gpsGetHours();
	gMinutes = gps->gpsGetMinutes();
	gSeconds = gps->gpsGetSeconds();

	/*
	 * TODO: add DST time correction
	 * for now it is hardcoded.
	 */
	if (++delay_counter > 16000) {
		if ((gHours + 3) != rtcHours) {
			if ((gHours + 3) >= 24)
				new_time.hour = (gHours + 3) - 24;
			else
				new_time.hour = gHours + 3;
		}

		if (gMinutes != rtcMinutes)
			new_time.min = gMinutes;
		if (gSeconds != rtcSeconds)
			new_time.sec = gSeconds;

		if (gps->gpsDateHasFix) {
			if (rtc_time.mday != gps->gpsGetDay())
				new_time.mday = gps->gpsGetDay();
			if (rtc_time.mon != gps->gpsGetMonth())
				new_time.mon = gps->gpsGetMonth();
			if (rtc_time.year_s != gps->gpsGetYear())
				new_time.year_s = gps->gpsGetYear();
		}
		rtc->set(new_time);

		delay_counter = 0;
	}

	return ESUCCESS;
}

static void send_usb(char *str)
{
	while (*str) {
		usb_serial_putchar(*str++);
	}
}

int main(void)
{
	struct LCD *screen;
	struct DS3231 *rtc;
	struct GPS *gps;
	struct BMP085 *pressSensor = (struct BMP085 *)malloc(sizeof(struct BMP085));
	float dht22_humidity = 0L;
	float dht22_temp = 0L;

	// WatchDog configuration
	wdt_enable(WDTO_2S);
	wdt_reset();
	// Init timers
	tmr_init();
	/*
	 * Init LED which will show activity
	 * on USART's RX line
	 */
	DDRB |= _BV(PB0);
	/*
	 * Init LED which will show activity
	 * on 1-wire data line
	 */
	DDRD |= _BV(PD5);
	/*
	 * Init 1-wire power line
	 */
	DDRE |= _BV(PE6);
	PORTE &= ~_BV(PE6); // GND
	DDRD |= _BV(PD7);
	PORTD |= _BV(PD7);	// PWR

	// Init i2c bus first, as screen, some sensors, use it to communicate
	I2CInit();
	// Init pressure/temperature sensor
	pressSensor = BMP085_init(MODE_STANDARD, 0, 1);
	pressSensor->setLocalAbsAlt(22000);
	pressSensor->setLocalPressure(740);
	// Init RTC
	rtc = DS3231_init(DS3231_INTCN);
	memset(&rtc_time, 0, sizeof(struct ts));
	// Init GPS
	gps = gps_init();
	// Init USART
	USART_init(9600); // TODO: *unhardcode speed select in usart.c
	// Enable interrupts
	sei();
	// Init USB
	CPU_PRESCALE(0); // Just routine, we already running @16MHz
	usb_init();
	usb_serial_flush_input();
	usb_serial_flush_output();
	// Init LCD and fill struct LCD with initial data
	screen = lcdInit();
	// Enable LCD backligt
	screen->backlight(enable);
	// Clear LCD screen and its buffer
	writeScreen(screen, ACTION_ERASE_SCREEN);
	// Init custom font chars from flash
	initPgmFont(screen);
	// Start forever loop
	while (1) {
		// Process usb configuration (not being in the while loop).
		// while (!usb_configured()) /* wait */
		usb_configured();
		// Reset Watchdog timer
		wdt_reset();
		// Turn off UART's led
		PORTB |= _BV(PB0);
		// Turn off 1-wire's led
		PORTD |= _BV(PD5);
		// Fill receive buffer
		strcpy(wbuf, tbuf);
		// Call to process GPS data received from UART
		gps->parse(wbuf);
		// Update current time from RTC module
		memset(&rtc_time, 0, sizeof(struct ts));
		rtc->get(&rtc_time);
		// Update pressure/temperature from BMP085 sensor
		pressSensor->getPressure(&slPressure);
		pressSensor->getTemperature(&slTemp);
		// Update time in RTC clock
		timeCorrection(rtc, gps);
		/*
		 * Read data from DHT22 sensor.
		 * We don't process errors here. Just show
		 * correct data ignoring the rest.
		 */
		dht22_read(&dht22_temp, &dht22_humidity);
		/*
		 * Clear screen/buffer before writing data
		 * so we make sure we always write to an
		 * empty screen buffer.
		 */
		writeScreen(screen, ACTION_ERASE_SCREEN);
		snprintf(pbuf[0], SCREEN_BUFF, "%c%02d:%02d %c  %c%3dC",
				ICO_CLOCK, rtc_time.hour, rtc_time.min,
				(gps->gpsTimeHasFix) ? ICO_SAT_ONLINE : ICO_SAT_OFFLINE,
				ICO_TEMP_INSIDE, (int16_t)(slTemp * 0.1));
		snprintf(pbuf[1], SCREEN_BUFF, "%c%d %c%2d%% %c%3dC",
				ICO_PRESSURE, (uint16_t)slPressure,
				ICO_HUMIDITY, (uint16_t)dht22_humidity,
				ICO_TEMP_OUTSIDE, (int16_t)dht22_temp);

		// Write actual data to LCD screen
		writeScreen(screen, ACTION_WRITE_SCREEN);

		memset(&ubuf, 0x00, BUFFER_SIZE);
		
		//send_usb(ubuf);
		snprintf(ubuf, BUFFER_SIZE, USB_OUTPUT_MASK,
				 rtc_time.hour, rtc_time.min, rtc_time.sec,
				 (uint16_t)slPressure, (uint16_t)dht22_humidity,
				 (int16_t)(slTemp * 0.1), (uint16_t)(slTemp % 10),
				 (int16_t)dht22_temp, (uint16_t)(dht22_temp) % 10,
				 wbuf);
		send_usb(ubuf);

		// Show activity on TX LED
		PORTD &= ~_BV(PD5);
	}

	return 0;
}
