#ifndef __SERIALEVENTHANDLER_H__
#define __SERIALEVENTHANDLER_H__

#include "DS1302.h"
#include "Arduino.h"

/* commands */
/* layout is 1 byte command, 1 byte data length, then data */
#define C_SETCLOCK 0x07
#define C_GETMEM 0x08

class SerialEventHandler {
public:
	SerialEventHandler(DS1302 *rtc);
	~SerialEventHandler();
	void parseSerial();
	int freeRam();
private:
	uint8_t *readBytes(uint8_t size);
	DS1302 *rtc;
};
#endif
