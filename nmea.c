#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "nmea.h"

#define TIME_MASK		"hhmmss"
#define DATE_MASK		"ddmmyy"

/*
typedef enum {
    TYPE_GGA,
    TYPE_GLL,
    TYPE_GSA,
    TYPE_GSV,
    TYPE_RMC,
    TYPE_VTG,
    TYPE_TXT,
    TYPE_ERR
} NMEA_msgtype;
*/

static struct GPS gps;

static char utc[sizeof(TIME_MASK)];
static char date[sizeof(DATE_MASK)];
static char sat_num[sizeof("nn")];
static char *notAvailable = "N/A";

static void parseGGA(char *data);
static void parseRMC(char *data);

static void parseByType(char *data) {

    if (!strncmp(data, "$GPGGA", 6))
	parseGGA(data);
    if (!strncmp(data, "$GPRMC", 6))
	parseRMC(data);
/* 
    if (!strncmp(data, "$GPTXT", 6))
	parseTXT(data);
    if (!strncmp(data, "$GPGLL", 6))
	parseGLL(data);
    if (!strncmp(data, "$GPGSA", 6))
	parseGSA(data);
    if (!strncmp(data, "$GPGSV", 6))
	parseGSV(data);
    if (!strncmp(data, "$GPVTG", 6))
	parseVTG(data);
*/
}

static void parseGGA(char *data) {
    if (!strncmp(data, "$GPGGA", 6)) {
        if (!strncmp((data + 7), ",", 1)) {
			strcpy(utc, notAvailable);
			strcpy(sat_num, "0");
			gps.gpsTimeHasFix = 0;
		} else {
			memcpy(utc, (data + 7), 6);
			memcpy(sat_num, (data + 44), 2);
			gps.gpsTimeHasFix = 1;
		}
	}
}

static void parseRMC(char *data) {
    char tmp[sizeof(DATE_MASK)];
	uint8_t rmcStringSize = 0;

	if (!strncmp(data, "$GPRMC", 6)) {				// Check wether we got the correct packet
		rmcStringSize = strlen(data);				// Check packet size (in case we still don't have fix to position
		if (rmcStringSize == 38) {
			if (!strncmp((data + 26), ",", 1))
				return;
			else {
				memcpy(tmp, (data + 25), 6);
				for (uint8_t i = 0; i < 6; i++) {
					if (!isdigit(tmp[i]))
						return;
				}
				gps.gpsDateHasFix = 0;
				memcpy(date, tmp, 6);
			}
		} else if (rmcStringSize > 38) {
			if (!strncmp((data + 57), "*", 1))
				return;
			else {
				memcpy(tmp, (data + 53), 6);
				for (uint8_t i = 0; i < 6; i++) {
					if (!isdigit(tmp[i]))
						return;
				}
				gps.gpsDateHasFix = 1;
				memcpy(date, tmp, 6);
			}
		}
    }
}

static char *getUTC(void) {
    return utc;
}

static char *getDate(void) {
    return date;
}

static uint8_t getHours(void) {
    char ret[2];

    memcpy(ret, utc, 2);
    return atoi(ret);
}

static uint8_t getMinutes(void) {
    char ret[2];

    memcpy(ret, (utc + 2), 2);
    return atoi(ret);
}

static uint8_t getSeconds(void) {
    char ret[2];

    memcpy(ret, (utc + 4), 2);
    return atoi(ret);
}

static uint8_t getDay(void) {
    char ret[2];

    memcpy(ret, date, 2);
    return atoi(ret);
}

static uint8_t getMonth(void) {
    char ret[2];

    memcpy(ret, (date + 2), 2);
    return atoi(ret);
}

static uint8_t getYear(void) {
	char ret[2];

    memcpy(ret, (date + 4), 2);
    return atoi(ret);
}

static void parseGPSData(char *data) {
    parseByType(data);
}

static struct GPS gps = {
	.gpsGetHours = getHours,
	.gpsGetMinutes = getMinutes,
	.gpsGetSeconds = getSeconds,
	.gpsGetDay = getDay,
	.gpsGetMonth = getMonth,
	.gpsGetYear = getYear,
	.gpsGetDate = getDate,
	.gpsGetUTC = getUTC,
	.parse = parseGPSData,
	.gpsTimeHasFix = 0,
	.gpsDateHasFix = 0
};

struct GPS *gps_init(void)
{
	return &gps;
}
