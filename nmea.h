#ifndef _NMEA_H_
#define _NMEA_H_

struct GPS {
	uint8_t gpsTimeHasFix;
	uint8_t gpsDateHasFix;
	uint8_t (*gpsGetHours)(void);
	uint8_t (*gpsGetMinutes)(void);
	uint8_t (*gpsGetSeconds)(void);
	uint8_t (*gpsGetDay)(void);
	uint8_t (*gpsGetMonth)(void);
	uint8_t (*gpsGetYear)(void);
	char *(*gpsGetDate)(void);
	char *(*gpsGetUTC)(void);
	void (*parse)(char *data);
};

struct GPS *gps_init(void);

#endif /* _NMEA_H_ */
