/****************************************************************************
* BMP085.cpp - BMP085/I2C (Digital Pressure Sensor) library for Arduino     *
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
#include <util/delay.h>
#include "i2c.h"
#include "bmp085.h"

#define BUFFER_SIZE					3
#define AUTO_UPDATE_TEMPERATURE		1	//default is true
		// when true, temperature is measured everytime pressure is measured (Auto).
		// when false, user chooses when to measure temperature (just call calcTrueTemperature()).
		// used for dynamic measurement to increase sample rate (see BMP085 modes below).

/* ---- Registers ---- */
#define CAL_AC1						0xAA	// R   Calibration data (16 bits)
#define CAL_AC2						0xAC	// R   Calibration data (16 bits)
#define CAL_AC3						0xAE	// R   Calibration data (16 bits)
#define CAL_AC4						0xB0	// R   Calibration data (16 bits)
#define CAL_AC5						0xB2	// R   Calibration data (16 bits)
#define CAL_AC6						0xB4	// R   Calibration data (16 bits)
#define CAL_B1						0xB6	// R   Calibration data (16 bits)
#define CAL_B2						0xB8	// R   Calibration data (16 bits)
#define CAL_MB						0xBA	// R   Calibration data (16 bits)
#define CAL_MC						0xBC	// R   Calibration data (16 bits)
#define CAL_MD						0xBE	// R   Calibration data (16 bits)
#define CONTROL						0xF4	// W   Control register
#define CONTROL_OUTPUT				0xF6	// R   Output registers 0xF6=MSB, 0xF7=LSB, 0xF8=XLSB

// unused registers
#define SOFTRESET					0xE0
#define VERSION						0xD1	// ML_VERSION  pos=0 len=4 msk=0F  AL_VERSION pos=4 len=4 msk=f0
#define CHIPID						0xD0	// pos=0 mask=FF len=8
											// BMP085_CHIP_ID=0x55

/************************************/
/*    REGISTERS PARAMETERS          */
/************************************/
// Control register
#define READ_TEMPERATURE			0x2E
#define READ_PRESSURE				0x34
//Other
#define MSLP						101325	// Mean Sea Level Pressure = 1013.25 hPA (1hPa = 100Pa = 1mbar)

static int16_t ac1,ac2,ac3,b1,b2,mb,mc,md;			// cal data
static uint16_t ac4,ac5,ac6;						// cal data
static long b5, oldEMA;								// temperature data
static uint8_t _dev_address;
static uint8_t _buff[BUFFER_SIZE];					// buffer  MSB LSB XLSB
static uint8_t _oss;								// OverSamplingSetting

int32_t _cm_Offset, _Pa_Offset;
int32_t _param_datum, _param_centimeters;

static void getCalData(void);

static void writemem(uint8_t _addr, uint8_t _val)
{
	I2CStart(_dev_address);   // start transmission to device
	I2CWriteByte(_addr); // send register address
	I2CWriteByte(_val); // send value to write
	I2CStop(); // end transmission
}

static void readmem(uint8_t _addr, uint8_t _nbytes, uint8_t __buff[])
{
	I2CStart(_dev_address); // start transmission to device
	I2CWriteByte(_addr); // sends register address to read from
	I2CStop(); // end transmission

	I2CStart(_dev_address | I2C_READ); // start transmission to device
	for (uint8_t i = 0; i < _nbytes; i++) {
		if (i == (_nbytes - 1))
			I2CReadByte(&__buff[i], I2C_NOACK);
		else
			I2CReadByte(&__buff[i], I2C_ACK);
	}
	I2CStop();
}

static uint8_t getDevAddr(void)
{
	return _dev_address;
}

static uint8_t getMode(void)
{
	return _oss;
}

static void setMode(uint8_t _BMPMode)
{
	_oss = _BMPMode;
}

static void calcTrueTemperature()
{
	long ut,x1,x2;

	//read Raw Temperature
	writemem(CONTROL, READ_TEMPERATURE);
	_delay_ms(5);                                         // min. 4.5ms read Temp delay
	readmem(CONTROL_OUTPUT, 2, _buff);
	ut = (long)((_buff[0] << 8) + _buff[1]);    // uncompensated temperature value

	// calculate temperature
	x1 = ((ut - ac6) * ac5 >> 15);
	x2 = ((long)mc << 11) / (x1 + md);
	b5 = x1 + x2;
}

static void calcTruePressure(long *_TruePressure)
{
	long up,x1,x2,x3,b3,b6,p;
	unsigned long b4,b7;
	int32_t tmp;

#if AUTO_UPDATE_TEMPERATURE
	calcTrueTemperature();        // b5 update
#endif

	//read Raw Pressure
	writemem(CONTROL, READ_PRESSURE+(_oss << 6));

	switch (_oss) {
		case 0:
			_delay_ms(5);
			break;
		case 1:
			_delay_ms(8);
			break;
		case 2:
			_delay_ms(14);
			break;
		case 3:
			_delay_ms(26);
			break;
	}

	readmem(CONTROL_OUTPUT, 3, _buff);
	up = ((((long)_buff[0] <<16) | ((long)_buff[1] <<8) | ((long)_buff[2])) >> (8-_oss)); // uncompensated pressure value

	// calculate true pressure
	b6 = b5 - 4000;             // b5 is updated by calcTrueTemperature().
	x1 = (b2* (b6 * b6 >> 12)) >> 11;
	x2 = ac2 * b6 >> 11;
	x3 = x1 + x2;
	tmp = ac1;
	tmp = (tmp * 4 + x3) << _oss;
	b3 = (tmp + 2) >> 2;
	x1 = ac3 * b6 >> 13;
	x2 = (b1 * (b6 * b6 >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (ac4 * (uint32_t) (x3 + 32768)) >> 15;
	b7 = ((uint32_t)up - b3) * (50000 >> _oss);
	p = b7 < 0x80000000 ? (b7 << 1) / b4 : (b7 / b4) << 1;
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	*_TruePressure = p + ((x1 + x2 + 3791) >> 4);
}

static void getPressure(int32_t *_Pa)
{
	long TruePressure;

	calcTruePressure(&TruePressure);
	*_Pa = TruePressure / pow((1 - (float)_param_centimeters / 4433000), 5.255) + _Pa_Offset;
	// converting from float to int32_t truncates toward zero, 1010.999985 becomes 1010 resulting in 1 Pa error (max).
	// Note that BMP085 abs accuracy from 700...1100hPa and 0..+65C is +-100Pa (typ.)
}

static void getTemperature(long *_Temperature)
{
	getCalData();
	calcTrueTemperature();                            // force b5 update
	*_Temperature = (b5 + 8) >> 4;
}

static void getAltitude(int32_t *_centimeters)
{
	long TruePressure;

	calcTruePressure(&TruePressure);
	*_centimeters =  4433000 * (1 - pow((TruePressure / (float)_param_datum), 0.1903)) + _cm_Offset;
	// converting from float to int32_t truncates toward zero, 100.999985 becomes 100 resulting in 1 cm error (max).
}

static void setLocalPressure(int32_t _Pa)
{
	int32_t tmp_alt;

	_param_datum = _Pa;
	getAltitude(&tmp_alt);    // calc altitude based on current pressure
	_param_centimeters = tmp_alt;
}

static void setLocalAbsAlt(int32_t _centimeters)
{
	int32_t tmp_Pa;

	_param_centimeters = _centimeters;
	getPressure(&tmp_Pa);    // calc pressure based on current altitude
	_param_datum = tmp_Pa;
}

static void setAltOffset(int32_t _centimeters)
{
	_cm_Offset = _centimeters;
}

static void setPaOffset(int32_t _Pa)
{
	_Pa_Offset = _Pa;
}

static void zeroCal(int32_t _Pa, int32_t _centimeters)
{
	setAltOffset(_centimeters - _param_centimeters);
	setPaOffset(_Pa - _param_datum);
}

static void getCalData(void) {
	readmem(CAL_AC1, 2, _buff);
	ac1 = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_AC2, 2, _buff);
	ac2 = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_AC3, 2, _buff);
	ac3 = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_AC4, 2, _buff);
	ac4 = ((uint16_t)_buff[0] << 8 | ((uint16_t)_buff[1]));
	readmem(CAL_AC5, 2, _buff);
	ac5 = ((uint16_t)_buff[0] << 8 | ((uint16_t)_buff[1]));
	readmem(CAL_AC6, 2, _buff);
	ac6 = ((uint16_t)_buff[0] << 8 | ((uint16_t)_buff[1]));
	readmem(CAL_B1, 2, _buff);
	b1 = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_B2, 2, _buff);
	b2 = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_MB, 2, _buff);
	mb = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_MC, 2, _buff);
	mc = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
	readmem(CAL_MD, 2, _buff);
	md = ((int16_t)_buff[0] << 8 | ((int16_t)_buff[1]));
}

static struct BMP085 sensor = {
	.getAddress = getDevAddr,
	.getMode = getMode,
	.setMode = setMode,
	.setLocalPressure = setLocalPressure,
	.setLocalAbsAlt = setLocalAbsAlt,
	.setAltOffset = setAltOffset,
	.zeroCal = zeroCal,
	.getPressure = getPressure,
	.getAltitude = getAltitude,
	.getTemperature = getTemperature,
	.calcTrueTemperature = calcTrueTemperature,
	.calcTruePressure = calcTruePressure,
	.writeMem = writemem,
	.readMem = readmem,
};

struct BMP085 *BMP085_init(uint8_t _BMPMode, int32_t _initVal, uint8_t _Unitmeters)
{

	_dev_address = BMP085_ADDR;
	_cm_Offset = 0;
	_Pa_Offset = 0;						// 1hPa = 100Pa = 1mbar

	oldEMA = 0;
	getCalData();						// initialize cal data
	calcTrueTemperature();				// initialize b5
	setMode(_BMPMode);
	_Unitmeters ? setLocalAbsAlt(_initVal) : setLocalPressure(_initVal);

	return &sensor;
}
