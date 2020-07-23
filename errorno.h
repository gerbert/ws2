#ifndef _ERRORNO_H_
#define _ERRORNO_H_

// General errors
#define ESUCCESS		0	// No error
#define ENULLPOINTER	1	// Address asigned to pointer is NULL

// I2C Bus errors
#define EI2CSTARTLOOP	2	// Entered endless loop while trying to start communication with i2c device
#define EI2CSTOPLOOP	3	// Entered endless loop while trying to stop communication with i2c device
#define EI2CWRITE		4	// Unsuccessfull write
#define EI2CWRITELOOP	5	// Entered endless loop while trying to write data to the device
#define EI2CREAD		6	// Unsuccessfull read

// EEPROM operation errors
#define EEEPADDRLSB		7	// Error occured while trying to send LSB address byte to i2c EEPROM device
#define EEEPADDRMSB		8	// Error occured while trying to send MSB address byte to i2c EEPROM device
#define EEEPDATAWR		9	// Error occured while trying to write data to i2c EEPROM device
#define EEEPDATAREAD	10	// Error occured while trying to read data from i2c EEPROM device

// Time errors
#define EGPSTIMENOFIX	11	// No fix, therefore we have no access to actual time

#endif /* _ERRORNO_H_ */
