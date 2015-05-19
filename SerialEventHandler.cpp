#include <Arduino.h>
#include <Time.h>
#include <DS1302RTC.h>
#include <EEPROM.h>
#include <Wire.h>
#include "SerialEventHandler.h"
#include "Definitions.h"

#ifdef DEBUG
/* creates a debug type message for the configuration program */
void sendDebug(const char *str) {
	int len = strlen(str);
	if(len <= 255) {
		uint8_t commandBytes[2];
		commandBytes[0] = C_DEBUG;
		commandBytes[1] = (uint8_t)len;
		Serial.write(commandBytes, 2);
		Serial.write((const uint8_t *)str, len);
	}
}
#endif

int16_t SerialEventHandler::freeRam() {
	extern int16_t __heap_start, *__brkval; 
	int16_t v; 
	return (int16_t) &v - (__brkval == 0 ? (int16_t) &__heap_start : (int16_t) __brkval); 
}

SerialEventHandler::SerialEventHandler(DS1302RTC *rtc) {
	this->rtc = rtc;
}

SerialEventHandler::~SerialEventHandler() {
	//nothing
}

uint8_t *SerialEventHandler::readBytes(uint8_t size, uint8_t extra_space) {
	uint8_t *data_buffer = NULL;
	if(size > 0) {
		data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (size + extra_space));
		Serial.readBytes((char *)data_buffer, (size_t)size);
	}
	return data_buffer;
}

/* covnerts a short to bytes and writes it */
void sendShort(int16_t value) {
	Serial.write(value >> 8);
	Serial.write(value & 0xFF);
}

/* converts bytes to a short */
int16_t receiveShort(uint8_t *inp) {
	int16_t ret = ((int16_t)(inp[0]) << 8) | (int16_t)(inp[1]);
	return ret;
}

void SerialEventHandler::parseSerial() {
		uint8_t commandBytes[2];
		uint8_t *data_buffer = NULL;
		
#ifdef DEBUG
		char debugBuffer[128];
#endif

		/* Read the 2 command bytes */
		if(Serial.available() > 1) {
			Serial.readBytes((char *)commandBytes, (size_t)2);
			switch(commandBytes[0]) {
			/* sets the RTC to the time received */
			case C_SETCLOCK:
				tmElements_t tm;
				data_buffer = readBytes(commandBytes[1]);
				tm.Year = y2kYearToTm(data_buffer[0]);
				tm.Month = data_buffer[1];
				tm.Day = data_buffer[2];
				tm.Hour = data_buffer[3];
				tm.Minute = data_buffer[4];
				tm.Second = data_buffer[5];
				tm.Wday = data_buffer[6];
				rtc->set(makeTime(tm));
				break;
			case C_GETCLOCK:
				/* gets the RTC and converts the time to bytes, sends it back */
				commandBytes[1] = 7;
				data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * 7);
				rtc->read(tm);
				data_buffer[0] = (uint8_t)(tmYearToCalendar(tm.Year) - 2000);
				data_buffer[1] = tm.Month;
				data_buffer[2] = tm.Day;
				data_buffer[3] = tm.Hour;
				data_buffer[4] = tm.Minute;
				data_buffer[5] = tm.Second;
				data_buffer[6] = tm.Wday;
				Serial.write(commandBytes, (size_t)2);
				Serial.write(data_buffer, 7);
				break;
			case C_GETMEM:
				/* sends the amount of free memory back */
				commandBytes[1] = (uint8_t)0x02;
				Serial.write(commandBytes, (size_t)2);
				sendShort(freeRam());
				break;
			case C_GETEEPROM:
				/* gets a section of EEPROM and writes it to serial */
				data_buffer = readBytes(commandBytes[1]);
				commandBytes[1] = receiveShort(&(data_buffer[2])) * 2 + 4;
				Serial.write(commandBytes, (size_t)2);
				Serial.write(data_buffer, 4);
				for(int16_t i = receiveShort(&(data_buffer[0])); i < receiveShort(&(data_buffer[2])) + receiveShort(&(data_buffer[0])); i++) {
					if(i <= E2END) {
						sendShort((int16_t)(EEPROM.read(i)));
					} else {
						sendShort(0);
					}
				}
				break;
			case C_SETEEPROM:
				/* sets a single value on EEPROM */
				data_buffer = readBytes(commandBytes[1]);
				EEPROM.write(receiveShort(&(data_buffer[0])), (uint8_t)(receiveShort(&(data_buffer[2]))));
				break;
			}
			/* if the serial buffer was allocated, free it */
			if(data_buffer != NULL) {
				free(data_buffer);
			}
		}
}
