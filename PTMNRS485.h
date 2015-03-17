#ifndef __PTMNRS485_H__
#define __PTMNRS485_H__

#include "Arduino.h"
#include <SoftwareSerial.h>

#define PTMNRS485_BAUD 9600
#define READ_TIMEOUT 1000
#define MAX_RETRIES 4
const unsigned char PTM_REQUEST_PRESSURE[] = { 0xf0, 0x04, 0x0, 0x0, 0x0, 0x01 };

enum PTMNRS485_MODE {
	PTM_IDLE = 0,
	PTM_WAITING = 1
};

class PTMNRS485 {
	public:
		PTMNRS485(unsigned short txPin, unsigned short rxPin, unsigned short txEnablePin);
		~PTMNRS485();
		void update();
		bool blockingRead();
		short getReading();
		short *getReadingPtr();
	protected:
		unsigned short txEnablePin;
		SoftwareSerial *sensorSerial;
		short reading;
		PTMNRS485_MODE mode;
		unsigned short calcCRC16(unsigned char *data, unsigned short count);
		void transmitFrame(const uint8_t *frame, size_t length);
		void setMode(PTMNRS485_MODE mode);
		PTMNRS485_MODE getMode();
		void requestPressure();
		short parseRequestPressure();
		unsigned long requestTime;
		
};

#endif