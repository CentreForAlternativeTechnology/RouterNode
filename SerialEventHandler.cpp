#include "Arduino.h"
#include "SerialEventHandler.h"
#include "DS1302.h"

int SerialEventHandler::freeRam() {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

SerialEventHandler::SerialEventHandler(DS1302 *rtc) {
	this->rtc = rtc;
	Serial.setTimeout(40);
}

SerialEventHandler::~SerialEventHandler() {
	//nothing
}

uint8_t *SerialEventHandler::readBytes(uint8_t size) {
	uint8_t *data_buffer = NULL;
	if(size > 0) {
		data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * size);
		Serial.readBytes((char *)data_buffer, (size_t)size);
	}
	return data_buffer;
}

void sendInt(int value) {
	Serial.write(value >> 8);
	Serial.write(value & 0xFF);
}

void SerialEventHandler::parseSerial() {
		uint8_t commandBytes[2];
		uint8_t *data_buffer = NULL;
		int tmp_int; 
		uint8_t tmp_uchar;
		if(Serial.available() > 1) {
			Serial.readBytes((char *)commandBytes, (size_t)2);
		}
		switch(commandBytes[0]) {
		case C_SETCLOCK:
			/* buffer should be 7 bytes */
			if(commandBytes[1] != 7) { 
				break; 
			}
			data_buffer = readBytes(commandBytes[1]);
			if(data_buffer[6] < 1 || data_buffer[6] > 7) { 
				break; 
			}
			rtc->year((uint16_t)data_buffer[0] + 2000);
			rtc->month(data_buffer[1]);
			rtc->date(data_buffer[2]);
			rtc->hour(data_buffer[3]);
			rtc->minutes(data_buffer[4]);
			rtc->seconds(data_buffer[5]);
			rtc->day((Time::Day)data_buffer[6]);
			break;
		case C_GETMEM:
			commandBytes[1] = (uint8_t)0x02;
			Serial.write(commandBytes, (size_t)2);
			tmp_int = freeRam();
			sendInt(tmp_int);
			break;
		}
		if(data_buffer != NULL) {
			free(data_buffer);
		}
}
