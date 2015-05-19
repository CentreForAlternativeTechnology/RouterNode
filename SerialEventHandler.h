#ifndef __SERIALEVENTHANDLER_H__
#define __SERIALEVENTHANDLER_H__

#include <DS1302RTC.h>
#include <Arduino.h>

/* commands */
/* layout is 1 byte command, 1 byte data length, then data */
#define C_SETCLOCK 0x07
#define C_GETCLOCK 0x06
#define C_GETMEM 0x08
#define C_GETEEPROM 0x09
#define C_SETEEPROM 0x10
#define C_DEBUG 0x1A

class SerialEventHandler {
public:
	/**
	 * @param rtc the pointer to the rtc object
	 **/
	SerialEventHandler(DS1302RTC *rtc);
	~SerialEventHandler();
	/**
	 * Called on a loop to read and parse serial data
	 **/
	void parseSerial();
	/**
	 * @return the number of free bytes of ram
	 **/
	int freeRam();
private:
	/**
	 * Reads bytes from the serial port into memory, allocates a buffer too.
	 * Free should be called on the buffer allocated here.
	 * @param size the numbre of bytes to read from the serial port
	 * @param extra_space the number of extra bytes to allocate when mallocing
	 * @return the pointer to the malloced and read data, or NULL on failure.
	 **/
	uint8_t *readBytes(uint8_t size, uint8_t extra_space = 0);
	/**
	 * Pointer to RTC object
	 **/
	DS1302RTC *rtc;
};
#endif
