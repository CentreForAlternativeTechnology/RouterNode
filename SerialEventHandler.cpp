#include "Arduino.h"
#include "SerialEventHandler.h"
#include "DS1302.h"
#include <EEPROM.h>
#include <Wire.h>
#include "EMonCMS.h"

#define ASHORT(x) (((uint8_t *)(&(x)))[1] | (((uint8_t *)(&(x)))[0] << 8))

extern int pressureAttributeReader(AttributeIdentifier *attr, DataItem *item);
extern float getDepth();
bool sensorEnabled = false;

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

uint8_t *SerialEventHandler::readBytes(uint8_t size, uint8_t extra_space) {
	uint8_t *data_buffer = NULL;
	if(size > 0) {
		data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (size + extra_space));
		Serial.readBytes((char *)data_buffer, (size_t)size);
	}
	return data_buffer;
}

void sendShort(short value) {
	Serial.write(value >> 8);
	Serial.write(value & 0xFF);
}

void sendFloat(float value) {
	floatBytes b;
	b.value = value;
	Serial.write(b.bytes, 4);
}

float receiveFloat(uint8_t *inp) {
	floatBytes b;
	for(int i = 0; i < 4; i++) {
		b.bytes[i] = inp[i];
	}
	return b.value;
}

void enableSensor() {
	if(!sensorEnabled) {
		Wire.begin();
		digitalWrite(EN_PIN1, HIGH);
		delay(300);
		sensorEnabled = true;
	}
}

void SerialEventHandler::parseSerial() {
		/* Declare everything we might use in here */
		char debugBuffer[64];
		
		uint8_t commandBytes[2];
		uint8_t *data_buffer = NULL;
		short tmp_int; 
		uint8_t tmp_uchar;
		DataItem item;
		floatBytes flb;

		/* Read the 2 command bytes */
		if(Serial.available() > 1) {
			Serial.readBytes((char *)commandBytes, (size_t)2);
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
				rtc->halt(false);
				break;
			case C_GETCLOCK:
				commandBytes[1] = 7;
				data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * 7);
				data_buffer[0] = (uint8_t)(rtc->year() - 2000);
				data_buffer[1] = rtc->month();
				data_buffer[2] = rtc->date();
				data_buffer[3] = rtc->hour();
				data_buffer[4] = rtc->minutes();
				data_buffer[5] = rtc->seconds();
				data_buffer[6] = rtc->day();
				Serial.write(commandBytes, (size_t)2);
				Serial.write(data_buffer, 7);
				break;
			case C_GETMEM:
				commandBytes[1] = (uint8_t)0x02;
				Serial.write(commandBytes, (size_t)2);
				tmp_int = freeRam();
				sendShort(tmp_int);
				break;
			case C_GETPRESSURE:
				commandBytes[1] = (uint8_t)0x02;
				enableSensor();
				if(pressureAttributeReader(NULL, &item)) {
					Serial.write(commandBytes, (size_t)2);
					sendShort(*(short *)(item.item));
				}
				break;
			case C_GETEEPROM:
				if(commandBytes[1] != 4) {
					break;
				}
				data_buffer = readBytes(commandBytes[1]);
				commandBytes[1] = ASHORT(data_buffer[2]) * 2 + 4;
				Serial.write(commandBytes, (size_t)2);
				Serial.write(data_buffer, 4);
				for(int i = ASHORT(data_buffer[0]); i < ASHORT(data_buffer[2]) + ASHORT(data_buffer[0]); i++) {
					if(i < 1024) {
						sendShort((short)(EEPROM.read(i)));
					} else {
						sendShort(0);
					}
				}
				break;
			case C_SETEEPROM:
				if(commandBytes[1] != 4) {
					break;
				}
				data_buffer = readBytes(commandBytes[1]);
				EEPROM.write(ASHORT(data_buffer[0]), (uint8_t)(ASHORT(data_buffer[2])));
				break;
			case C_SETPRESSUREGRADIENT:
				if(commandBytes[1] != 4) {
					break;
				}
				data_buffer = readBytes(commandBytes[1]);
				flb.value = receiveFloat(data_buffer);
				for(int i = 0; i < 4; i++) {
					EEPROM.write(EEPROM_CALIB_GRAD + i, flb.bytes[i]);
				}
				break;
			case C_SETPRESSURECONSTANT:
				if(commandBytes[1] != 4) {
					break;
				}
				data_buffer = readBytes(commandBytes[1]);
				flb.value = receiveFloat(data_buffer);
				for(int i = 0; i < 4; i++) {
					EEPROM.write(EEPROM_CALIB_CONST + i, flb.bytes[i]);
				}
				break;
			case C_SETPRESSUREBASE:
				enableSensor();
				if(pressureAttributeReader(NULL, &item)) {
					EEPROM.write(EEPROM_CALIB_BASE, ((uint8_t *)(item.item))[0]);
					EEPROM.write(EEPROM_CALIB_BASE + 1, ((uint8_t *)(item.item))[1]);
				}
				break;
			case C_GETDEPTH:
				enableSensor();
				if(pressureAttributeReader(NULL, &item)) {
					commandBytes[1] = 4;
					Serial.write(commandBytes, 2);
					sendFloat(getDepth());
				}
				break;
			}
			if(data_buffer != NULL) {
				free(data_buffer);
			}
		}
}
