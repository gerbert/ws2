/****************************************************************************
* BMP085.h - BMP085/I2C (Digital Pressure Sensor) library for Arduino       *
* Copyright 2010-2012 Filipe Vieira & various contributors                  *
*                                                                           *
* This file is part of BMP085 Arduino library.                              *
*                                                                           *
* This library is free software: you can redistribute it and/or modify      *
* it under the terms of the GNU Lesser General Public License as published  *
* by the Free Software Foundation, either version 3 of the License, or      *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU Lesser General Public License for more details.                       *
*                                                                           *
* You should have received a copy of the GNU Lesser General Public License  *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
****************************************************************************/
/****************************************************************************
* Tested on Arduino Mega with BMP085 Breakout                               *
* SDA   -> pin 20   (no pull up resistors)                                  *
* SCL   -> pin 21   (no pull up resistors)                                  *
* XCLR  -> not connected                                                    *
* EOC   -> not connected                                                    *
* GND   -> pin GND                                                          *
* VCC   -> pin 3.3V                                                         *
* NOTE: SCL and SDA needs pull-up resistors for each I2C bus.               *
*  2.2kOhm..10kOhm, typ. 4.7kOhm                                            *
*****************************************************************************/
#ifndef _BMP085_H_
#define _BMP085_H_

#define BMP085_ADDR					0xEE	//0x77 default I2C address

// BMP085 Modes
#define MODE_ULTRA_LOW_POWER		0 //oversampling=0, internalsamples=1, maxconvtimepressure=4.5ms, avgcurrent=3uA, RMSnoise_hPA=0.06, RMSnoise_m=0.5
#define MODE_STANDARD				1 //oversampling=1, internalsamples=2, maxconvtimepressure=7.5ms, avgcurrent=5uA, RMSnoise_hPA=0.05, RMSnoise_m=0.4
#define MODE_HIGHRES				2 //oversampling=2, internalsamples=4, maxconvtimepressure=13.5ms, avgcurrent=7uA, RMSnoise_hPA=0.04, RMSnoise_m=0.3
#define MODE_ULTRA_HIGHRES			3 //oversampling=3, internalsamples=8, maxconvtimepressure=25.5ms, avgcurrent=12uA, RMSnoise_hPA=0.03, RMSnoise_m=0.25
				// "Sampling rate can be increased to 128 samples per second (standard mode) for
				// dynamic measurement.In this case it is sufficient to measure temperature only
				// once per second and to use this value for all pressure measurements during period."
				// (from BMP085 datasheet Rev1.2 page 10).
				// To use dynamic measurement set AUTO_UPDATE_TEMPERATURE to false and
				// call calcTrueTemperature() from your code.

struct BMP085 {
	uint8_t (*getAddress)(void);
	// BMP mode
	uint8_t (*getMode)(void);
	void (*setMode)(uint8_t _BMPMode);						// BMP085 mode
	// Initialization
	void (*setLocalPressure)(int32_t _Pa);					// set known barometric pressure as reference Ex. QNH
	void (*setLocalAbsAlt)(int32_t _centimeters);			// set known altitude as reference
	void (*setAltOffset)(int32_t _centimeters);				// altitude offset
	void (*setPaOffset)(int32_t _Pa);						// pressure offset
	void (*zeroCal)(int32_t _Pa, int32_t _centimeters);		// zero Calibrate output to a specific Pa/altitude
	// BMP sensors
	void (*getPressure)(int32_t *_Pa);						// pressure in Pa + offset
	void (*getAltitude)(int32_t *_centimeters);				// altitude in centimeters + offset
	void (*getTemperature)(long *_Temperature);			// temperature in C
	void (*calcTrueTemperature)(void);						// calc temperature data b5 (only needed if AUTO_UPDATE_TEMPERATURE is false)
	void (*calcTruePressure)(long *_TruePressure);			// calc Pressure in Pa
	// Dummy staff
	void (*writeMem)(uint8_t _addr, uint8_t _val);
	void (*readMem)(uint8_t _addr, uint8_t _nbytes, uint8_t __buff[]);
};

struct BMP085 *BMP085_init(uint8_t _BMPMode, int32_t _initVal, uint8_t _centimeters);

#endif /* _BMP085_H_ */
